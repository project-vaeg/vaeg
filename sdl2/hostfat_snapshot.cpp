/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#include "compiler.h"
#include "hostfat_path.h"
#include "hostfat_snapshot.h"
#include "hostfat.h"

struct hostfat_snapshot_candidate {
	std::vector<unsigned char> image;
	HOSTFAT_SNAPSHOT_INFO info{};
	HOSTFAT_PREPARED_IMAGE *prepared = nullptr;
};

namespace fs = std::filesystem;

namespace {

constexpr std::size_t kSectorSize = HOSTFAT_SECTOR_SIZE;
constexpr std::size_t kTotalSectors = HOSTFAT_TOTAL_SECTORS;
constexpr std::size_t kSectorsPerCluster = HOSTFAT_SECTORS_PER_CLUSTER;
constexpr std::size_t kClusterSize = kSectorSize * kSectorsPerCluster;
constexpr std::size_t kFatSectors = 7;
constexpr std::size_t kFatCopies = 2;
constexpr std::size_t kRootEntries = 128;
constexpr std::size_t kRootSectors = (kRootEntries * 32 + kSectorSize - 1) / kSectorSize;
constexpr std::size_t kDataStartSector = kFatSectors * kFatCopies + kRootSectors;
constexpr std::size_t kDataClusters = (kTotalSectors - kDataStartSector) / kSectorsPerCluster;
constexpr std::size_t kFat12ClusterLimit = 4085;
constexpr std::size_t kFatEntries = 4096;
constexpr std::size_t kFirstReservedFat12Cluster = 0x0ff0;
constexpr unsigned kMaximumDepth = 8;
constexpr unsigned kMaximumEntries = 1024;
static_assert(kDataClusters < kFat12ClusterLimit, "HOSTFAT DOS-visible geometry must remain FAT12");
static_assert(kDataClusters == 4084,
              "HOSTFAT must remain at the maximum FAT12 cluster-count boundary");
static_assert(2 + kDataClusters == 0x0ff6,
              "HOSTFAT FAT12-max geometry must expose clusters 002H through 0FF5H");
static_assert(kClusterSize == 16384, "PC-Engine HOSTFAT clusters must remain 16 KiB");
static_assert(HOSTFAT_TOTAL_SECTORS <= UINT16_MAX,
              "PC-Engine HOSTFAT requests contain a 16-bit starting sector");
static_assert(HOSTFAT_BACKING_SECTORS >= HOSTFAT_TOTAL_SECTORS,
              "HOSTFAT backing must contain every DOS-visible sector");
constexpr std::array<unsigned char, 11> kVolumeLabel = {'H', 'O', 'S', 'T', 'F', 'A',
                                                        'T', ' ', ' ', ' ', ' '};

struct FatTimestamp {
	std::uint16_t time = 0;
	std::uint16_t date = 0x0021;
};

struct FileIdentity {
	std::uint64_t first = 0;
	std::uint64_t second = 0;

	bool operator==(const FileIdentity &other) const {
		return (first == other.first) && (second == other.second);
	}
};

struct Node {
	fs::path path;
	std::string source_name;
	std::array<unsigned char, 11> dos_name{};
	bool dos_name_assigned = false;
	bool directory = false;
	std::uint32_t size = 0;
	fs::file_time_type modified_time{};
	FatTimestamp fat_timestamp{};
	std::uintmax_t hard_links = 0;
	FileIdentity identity{};
	std::uint16_t first_cluster = 0;
	std::uint16_t cluster_count = 0;
	Node *parent = nullptr;
	std::vector<std::unique_ptr<Node>> children;
};

struct BuildState {
	unsigned entries = 0;
	unsigned files = 0;
	unsigned directories = 0;
	std::uint64_t source_bytes = 0;
	std::uint64_t copied_bytes = 0;
	fs::path canonical_root;
	std::vector<FileIdentity> regular_files;
	HOSTFAT_SNAPSHOT_PROGRESS progress = nullptr;
	void *progress_context = nullptr;
	std::string error;
};

void report_progress(BuildState &state, const char *phase, std::uint64_t completed,
                     std::uint64_t total) {
	if (state.progress != nullptr) {
		state.progress(state.progress_context, phase, completed, total);
	}
}

void set_error(char *destination, UINT size, const std::string &message) {
	if ((destination != nullptr) && (size != 0)) {
		std::snprintf(destination, size, "%s", message.c_str());
	}
}

void store_word(unsigned char *destination, std::uint16_t value) {
	destination[0] = static_cast<unsigned char>(value);
	destination[1] = static_cast<unsigned char>(value >> 8);
}

void store_dword(unsigned char *destination, std::uint32_t value) {
	destination[0] = static_cast<unsigned char>(value);
	destination[1] = static_cast<unsigned char>(value >> 8);
	destination[2] = static_cast<unsigned char>(value >> 16);
	destination[3] = static_cast<unsigned char>(value >> 24);
}

std::uint16_t load_word(const unsigned char *source) {
	return static_cast<std::uint16_t>(source[0] | (static_cast<std::uint16_t>(source[1]) << 8));
}

FatTimestamp pack_fat_timestamp(int year, int month, int day, int hour, int minute, int second) {
	// FAT stores local civil time without a timezone and with two-second
	// resolution. Clamp metadata outside the representable 1980--2107 range.
	if (year < 1980) {
		return FatTimestamp{};
	}
	if (year > 2107) {
		return FatTimestamp{0xbf7d, 0xff9f};
	}
	month = std::clamp(month, 1, 12);
	day = std::clamp(day, 1, 31);
	hour = std::clamp(hour, 0, 23);
	minute = std::clamp(minute, 0, 59);
	second = std::clamp(second, 0, 59);
	FatTimestamp result;
	result.time = static_cast<std::uint16_t>((hour << 11) | (minute << 5) | (second / 2));
	result.date = static_cast<std::uint16_t>(((year - 1980) << 9) | (month << 5) | day);
	return result;
}

bool read_fat_timestamp(const fs::path &path, FatTimestamp &timestamp, std::error_code &error) {
#if defined(_WIN32)
	const HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
	                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                                  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (handle == INVALID_HANDLE_VALUE) {
		error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
		return false;
	}
	FILETIME modified{};
	const BOOL got_time = GetFileTime(handle, nullptr, nullptr, &modified);
	const DWORD get_time_error = got_time ? ERROR_SUCCESS : GetLastError();
	CloseHandle(handle);
	if (!got_time) {
		error = std::error_code(static_cast<int>(get_time_error), std::system_category());
		return false;
	}
	SYSTEMTIME utc{};
	SYSTEMTIME local{};
	if (!FileTimeToSystemTime(&modified, &utc) ||
	    !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) {
		error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
		return false;
	}
	timestamp = pack_fat_timestamp(local.wYear, local.wMonth, local.wDay, local.wHour,
	                               local.wMinute, local.wSecond);
#else
	struct stat information{};
	if (::stat(path.c_str(), &information) != 0) {
		error = std::error_code(errno, std::generic_category());
		return false;
	}
	std::tm local{};
	errno = 0;
	if (localtime_r(&information.st_mtime, &local) == nullptr) {
		error = std::error_code(errno != 0 ? errno : EOVERFLOW, std::generic_category());
		return false;
	}
	timestamp = pack_fat_timestamp(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
	                               local.tm_hour, local.tm_min, local.tm_sec);
#endif
	error.clear();
	return true;
}

bool dos_character(unsigned char value) {
	return ((value >= 'A') && (value <= 'Z')) || ((value >= '0') && (value <= '9')) ||
	       value == '_' || value == '-';
}

bool make_dos_name(const std::string &source, std::array<unsigned char, 11> &destination) {
	std::size_t dot;
	std::string base;
	std::string extension;

	if (source.empty() || source == "." || source == "..") {
		return false;
	}
	dot = source.find('.');
	if ((dot != std::string::npos) && (source.find('.', dot + 1) != std::string::npos)) {
		return false;
	}
	base = (dot == std::string::npos) ? source : source.substr(0, dot);
	extension = (dot == std::string::npos) ? std::string() : source.substr(dot + 1);
	if (base.empty() || (base.size() > 8) || (extension.size() > 3) ||
	    ((dot != std::string::npos) && extension.empty())) {
		return false;
	}
	destination.fill(' ');
	for (std::size_t position = 0; position < base.size(); position++) {
		unsigned char value = static_cast<unsigned char>(base[position]);
		if ((value >= 'a') && (value <= 'z')) {
			value = static_cast<unsigned char>(value - 'a' + 'A');
		}
		if (!dos_character(value)) {
			return false;
		}
		destination[position] = value;
	}
	for (std::size_t position = 0; position < extension.size(); position++) {
		if (position >= 3) {
			return false;
		}
		unsigned char value = static_cast<unsigned char>(extension[position]);
		if ((value >= 'a') && (value <= 'z')) {
			value = static_cast<unsigned char>(value - 'a' + 'A');
		}
		if (!dos_character(value)) {
			return false;
		}
		destination[8 + position] = value;
	}
	return true;
}

bool valid_utf8_name(const std::string &source) {
	std::size_t position = 0;
	while (position < source.size()) {
		const unsigned char first = static_cast<unsigned char>(source[position]);
		std::uint32_t codepoint;
		std::size_t continuation;
		if (first < 0x80) {
			if ((first < 0x20) || (first == 0x7f)) {
				return false;
			}
			position++;
			continue;
		}
		if ((first >= 0xc2) && (first <= 0xdf)) {
			codepoint = first & 0x1f;
			continuation = 1;
		} else if ((first >= 0xe0) && (first <= 0xef)) {
			codepoint = first & 0x0f;
			continuation = 2;
		} else if ((first >= 0xf0) && (first <= 0xf4)) {
			codepoint = first & 0x07;
			continuation = 3;
		} else {
			return false;
		}
		if (position + continuation >= source.size()) {
			return false;
		}
		for (std::size_t index = 1; index <= continuation; index++) {
			const unsigned char next = static_cast<unsigned char>(source[position + index]);
			if ((next & 0xc0) != 0x80) {
				return false;
			}
			codepoint = (codepoint << 6) | (next & 0x3f);
		}
		if (((continuation == 2) && (codepoint < 0x800)) ||
		    ((continuation == 3) && (codepoint < 0x10000)) || (codepoint > 0x10ffff) ||
		    ((codepoint >= 0xd800) && (codepoint <= 0xdfff))) {
			return false;
		}
		position += continuation + 1;
	}
	return !source.empty();
}

bool dos_device_name(const std::array<unsigned char, 11> &name) {
	std::string base;
	for (std::size_t index = 0; index < 8 && name[index] != ' '; index++) {
		base.push_back(static_cast<char>(name[index]));
	}
	if ((base == "CON") || (base == "PRN") || (base == "AUX") || (base == "NUL")) {
		return true;
	}
	return (base.size() == 4) &&
	       (((base.compare(0, 3, "COM") == 0) || (base.compare(0, 3, "LPT") == 0)) &&
	        (base[3] >= '1') && (base[3] <= '9'));
}

std::uint32_t alias_hash(const std::string &source) {
	std::uint32_t hash = 2166136261U;
	for (const unsigned char value : source) {
		hash ^= value;
		hash *= 16777619U;
	}
	return hash;
}

void append_alias_characters(const std::string &source, std::string &output, std::size_t limit) {
	for (const unsigned char source_value : source) {
		unsigned char value = source_value;
		if ((value >= 'a') && (value <= 'z')) {
			value = static_cast<unsigned char>(value - 'a' + 'A');
		}
		if (dos_character(value)) {
			output.push_back(static_cast<char>(value));
			if (output.size() == limit) {
				break;
			}
		}
	}
}

void make_dos_alias(const std::string &source, unsigned attempt,
                    std::array<unsigned char, 11> &destination) {
	static constexpr char hex[] = "0123456789ABCDEF";
	const std::size_t dot = source.rfind('.');
	const std::string base =
	    ((dot == std::string::npos) || (dot == 0)) ? source : source.substr(0, dot);
	const std::string extension = ((dot == std::string::npos) || (dot + 1 == source.size()))
	                                  ? std::string()
	                                  : source.substr(dot + 1);
	std::string prefix;
	std::string suffix;
	append_alias_characters(base, prefix, 4);
	append_alias_characters(extension, suffix, 3);
	if (prefix.empty()) {
		prefix = "FILE";
	}
	const std::uint32_t hash = (alias_hash(source) + attempt * 0x9e3779b9U) & 0x0fffU;
	destination.fill(' ');
	std::copy(prefix.begin(), prefix.end(), destination.begin());
	destination[prefix.size()] = '~';
	destination[prefix.size() + 1] = hex[(hash >> 8) & 0x0f];
	destination[prefix.size() + 2] = hex[(hash >> 4) & 0x0f];
	destination[prefix.size() + 3] = hex[hash & 0x0f];
	std::copy(suffix.begin(), suffix.end(), destination.begin() + 8);
}

bool assign_dos_names(Node &node, BuildState &state) {
	std::sort(node.children.begin(), node.children.end(),
	          [](const std::unique_ptr<Node> &left, const std::unique_ptr<Node> &right) {
		          return left->source_name < right->source_name;
	          });
	std::vector<std::array<unsigned char, 11>> used;
	for (auto &child : node.children) {
		if (make_dos_name(child->source_name, child->dos_name) &&
		    !dos_device_name(child->dos_name) &&
		    (std::find(used.begin(), used.end(), child->dos_name) == used.end())) {
			child->dos_name_assigned = true;
			used.push_back(child->dos_name);
		}
	}
	for (auto &child : node.children) {
		if (child->dos_name_assigned) {
			continue;
		}
		bool assigned = false;
		for (unsigned attempt = 0; attempt < 4096; attempt++) {
			make_dos_alias(child->source_name, attempt, child->dos_name);
			if (std::find(used.begin(), used.end(), child->dos_name) == used.end()) {
				child->dos_name_assigned = true;
				used.push_back(child->dos_name);
				assigned = true;
				break;
			}
		}
		if (!assigned) {
			state.error = "cannot generate a unique DOS 8.3 alias";
			return false;
		}
	}
	std::sort(node.children.begin(), node.children.end(),
	          [](const std::unique_ptr<Node> &left, const std::unique_ptr<Node> &right) {
		          return left->dos_name < right->dos_name;
	          });
	return true;
}

bool inspect_file_identity(const fs::path &path, bool directory, FileIdentity &identity,
                           std::uintmax_t &links, std::error_code &error) {
#if defined(_WIN32)
	const HANDLE handle = CreateFileW(
	    path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	    nullptr, OPEN_EXISTING,
	    FILE_FLAG_OPEN_REPARSE_POINT | (directory ? FILE_FLAG_BACKUP_SEMANTICS : 0), nullptr);
	if (handle == INVALID_HANDLE_VALUE) {
		error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
		return false;
	}
	BY_HANDLE_FILE_INFORMATION information{};
	if (!GetFileInformationByHandle(handle, &information)) {
		error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
		CloseHandle(handle);
		return false;
	}
	if ((information.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
		error = std::make_error_code(std::errc::too_many_symbolic_link_levels);
		CloseHandle(handle);
		return false;
	}
	CloseHandle(handle);
	links = information.nNumberOfLinks;
	identity.first = information.dwVolumeSerialNumber;
	identity.second =
	    (static_cast<std::uint64_t>(information.nFileIndexHigh) << 32) | information.nFileIndexLow;
#else
	struct stat information{};
	if (::lstat(path.c_str(), &information) != 0) {
		error = std::error_code(errno, std::generic_category());
		return false;
	}
	if (S_ISLNK(information.st_mode) ||
	    (directory ? !S_ISDIR(information.st_mode) : !S_ISREG(information.st_mode))) {
		error = std::make_error_code(std::errc::invalid_argument);
		return false;
	}
	links = static_cast<std::uintmax_t>(information.st_nlink);
	identity.first = static_cast<std::uint64_t>(information.st_dev);
	identity.second = static_cast<std::uint64_t>(information.st_ino);
#endif
	error.clear();
	return true;
}

bool capture_timestamp(Node &node, const std::string &name, BuildState &state) {
	std::error_code error;
	node.modified_time = fs::last_write_time(node.path, error);
	if (error || !read_fat_timestamp(node.path, node.fat_timestamp, error)) {
		state.error = "cannot inspect host timestamp: " + name;
		if (error) {
			state.error += ": " + error.message();
		}
		return false;
	}
	return true;
}

bool path_is_within(const fs::path &root, const fs::path &candidate) {
	auto root_part = root.begin();
	auto candidate_part = candidate.begin();
	for (; root_part != root.end(); ++root_part, ++candidate_part) {
		if ((candidate_part == candidate.end()) || (*candidate_part != *root_part)) {
			return false;
		}
	}
	return true;
}

bool capture_identity(Node &node, const std::string &name, BuildState &state) {
	std::error_code error;
	if (!inspect_file_identity(node.path, node.directory, node.identity, node.hard_links, error)) {
		state.error = "cannot inspect host entry identity: " + name;
		if (error) {
			state.error += ": " + error.message();
		}
		return false;
	}
	if (!node.directory && (node.hard_links != 1)) {
		state.error = "hard-linked files are not supported: " + name;
		return false;
	}
	return true;
}

bool scan_directory(Node &node, unsigned depth, BuildState &state) {
	std::error_code error;
	fs::directory_iterator iterator(node.path, error);
	fs::directory_iterator end;

	if (error) {
		state.error = "cannot enumerate host directory: " + error.message();
		return false;
	}
	for (; iterator != end; iterator.increment(error)) {
		if (error) {
			state.error = "cannot continue host-directory scan: " + error.message();
			return false;
		}
		const fs::directory_entry &item = *iterator;
		const fs::file_status status = item.symlink_status(error);
		if (error) {
			state.error = "cannot inspect host entry: " + error.message();
			return false;
		}
		const bool linked = fs::is_symlink(status);
#if !defined(_WIN32)
		if (linked) {
			state.error = "symbolic links are not supported: " + item.path().filename().u8string();
			return false;
		}
#endif
		if (!linked && !fs::is_directory(status) && !fs::is_regular_file(status)) {
			state.error = "special files are not supported: " + item.path().filename().u8string();
			return false;
		}
		if (++state.entries > kMaximumEntries) {
			state.error = "host directory exceeds the HOSTFAT entry limit";
			return false;
		}
		report_progress(state, "Scanning host directory", state.entries, kMaximumEntries);
		const std::string name = item.path().filename().u8string();
		if (!valid_utf8_name(name)) {
			state.error = "host entry name is not valid UTF-8";
			return false;
		}
		const fs::path canonical = fs::canonical(item.path(), error);
		if (error || !path_is_within(state.canonical_root, canonical)) {
			state.error = "host entry escapes the canonical HOSTFAT root: " + name;
			return false;
		}
		const fs::file_status canonical_status = fs::status(canonical, error);
		if (error ||
		    (!fs::is_directory(canonical_status) && !fs::is_regular_file(canonical_status))) {
			state.error = "special files are not supported: " + name;
			return false;
		}
		auto child = std::make_unique<Node>();
		child->path = canonical;
		child->source_name = name;
		child->parent = &node;
		child->directory = fs::is_directory(canonical_status);
		if (!capture_timestamp(*child, name, state) || !capture_identity(*child, name, state)) {
			return false;
		}
		if (child->directory) {
			if (depth >= kMaximumDepth) {
				state.error = "host directory exceeds the HOSTFAT depth limit";
				return false;
			}
			state.directories++;
			if (!scan_directory(*child, depth + 1, state)) {
				return false;
			}
		} else {
			const std::uintmax_t size = fs::file_size(child->path, error);
			if (error || (size > (std::numeric_limits<std::uint32_t>::max)())) {
				state.error = "cannot represent host file size: " + name;
				return false;
			}
			child->size = static_cast<std::uint32_t>(size);
			for (const FileIdentity &previous : state.regular_files) {
				if (previous == child->identity) {
					state.error = "hard-linked files are not supported: " + name;
					return false;
				}
			}
			state.regular_files.push_back(child->identity);
			state.files++;
			state.source_bytes += size;
		}
		node.children.push_back(std::move(child));
	}
	if (error) {
		state.error = "cannot finish host-directory scan: " + error.message();
		return false;
	}
	return assign_dos_names(node, state);
}

bool assign_clusters(Node &node, std::size_t &next_cluster, BuildState &state) {
	if (node.parent != nullptr) {
		const std::size_t bytes = node.directory ? (node.children.size() + 2) * 32 : node.size;
		std::size_t clusters = (bytes + kClusterSize - 1) / kClusterSize;
		if (node.directory && (clusters == 0)) {
			clusters = 1;
		}
		if ((clusters > UINT16_MAX) || (next_cluster + clusters > 2 + kDataClusters) ||
		    (next_cluster + clusters > kFirstReservedFat12Cluster)) {
			state.error = "host directory does not fit the FAT12-max snapshot";
			return false;
		}
		node.cluster_count = static_cast<std::uint16_t>(clusters);
		if (clusters != 0) {
			node.first_cluster = static_cast<std::uint16_t>(next_cluster);
			next_cluster += clusters;
		}
	}
	for (auto &child : node.children) {
		if (!assign_clusters(*child, next_cluster, state)) {
			return false;
		}
	}
	return true;
}

void add_chain(const Node &node, std::vector<std::uint16_t> &fat) {
	for (std::size_t index = 0; index < node.cluster_count; index++) {
		const std::size_t cluster = node.first_cluster + index;
		fat[cluster] =
		    static_cast<std::uint16_t>((index + 1 == node.cluster_count) ? 0x0fff : cluster + 1);
	}
	for (const auto &child : node.children) {
		add_chain(*child, fat);
	}
}

void write_directory_entry(unsigned char *destination, const std::array<unsigned char, 11> &name,
                           unsigned char attributes, std::uint16_t cluster, std::uint32_t size,
                           const FatTimestamp &timestamp) {
	std::copy(name.begin(), name.end(), destination);
	destination[11] = attributes;
	store_word(destination + 22, timestamp.time);
	store_word(destination + 24, timestamp.date);
	store_word(destination + 26, cluster);
	store_dword(destination + 28, size);
}

std::size_t cluster_offset(std::uint16_t cluster) {
	return kDataStartSector * kSectorSize + (static_cast<std::size_t>(cluster) - 2) * kClusterSize;
}

bool file_identity_is_unchanged(const Node &node, BuildState &state) {
	std::error_code error;
	FileIdentity current_identity{};
	std::uintmax_t current_hard_links = 0;
	const fs::file_status status = fs::symlink_status(node.path, error);
	if (error || fs::is_symlink(status) || !fs::is_regular_file(status)) {
		state.error = "host file type changed while creating snapshot";
		return false;
	}
	const std::uintmax_t size = fs::file_size(node.path, error);
	if (error || (size != node.size)) {
		state.error = "host file size changed while creating snapshot";
		return false;
	}
	const fs::file_time_type modified_time = fs::last_write_time(node.path, error);
	if (error || (modified_time != node.modified_time)) {
		state.error = "host file timestamp changed while creating snapshot";
		return false;
	}
	if (!inspect_file_identity(node.path, false, current_identity, current_hard_links, error) ||
	    !(current_identity == node.identity) || (current_hard_links != node.hard_links) ||
	    (current_hard_links != 1)) {
		state.error = "host file link identity changed while creating snapshot";
		return false;
	}
	return true;
}

bool directory_identity_is_unchanged(const Node &node, BuildState &state) {
	std::error_code error;
	FileIdentity current_identity{};
	std::uintmax_t current_hard_links = 0;
	const fs::file_status status = fs::symlink_status(node.path, error);
	if (error || fs::is_symlink(status) || !fs::is_directory(status)) {
		state.error = "host directory type changed while creating snapshot";
		return false;
	}
	const fs::file_time_type modified_time = fs::last_write_time(node.path, error);
	if (error || (modified_time != node.modified_time)) {
		state.error = "host directory timestamp changed while creating snapshot";
		return false;
	}
	if (!inspect_file_identity(node.path, true, current_identity, current_hard_links, error) ||
	    !(current_identity == node.identity)) {
		state.error = "host directory identity changed while creating snapshot";
		return false;
	}
	return true;
}

bool write_stable_file(const Node &node, unsigned char *destination, BuildState &state) {
	if (!file_identity_is_unchanged(node, state)) {
		return false;
	}
	std::ifstream input(node.path, std::ios::binary);
	if (!input) {
		state.error = "cannot open host file while creating snapshot";
		return false;
	}
	if (node.size != 0) {
		input.read(reinterpret_cast<char *>(destination), static_cast<std::streamsize>(node.size));
	}
	if ((static_cast<std::uint32_t>(input.gcount()) != node.size) ||
	    (input.peek() != std::char_traits<char>::eof()) ||
	    !file_identity_is_unchanged(node, state)) {
		if (state.error.empty()) {
			state.error = "host file changed while creating snapshot";
		}
		return false;
	}

	std::ifstream verification(node.path, std::ios::binary);
	if (!verification) {
		state.error = "cannot reopen host file while verifying snapshot";
		return false;
	}
	std::array<unsigned char, 65536> buffer{};
	std::size_t position = 0;
	while (position < node.size) {
		const std::size_t amount =
		    std::min<std::size_t>(buffer.size(), static_cast<std::size_t>(node.size) - position);
		verification.read(reinterpret_cast<char *>(buffer.data()),
		                  static_cast<std::streamsize>(amount));
		if ((static_cast<std::size_t>(verification.gcount()) != amount) ||
		    !std::equal(buffer.begin(), buffer.begin() + amount, destination + position)) {
			state.error = "host file content changed while creating snapshot";
			return false;
		}
		position += amount;
	}
	if ((verification.peek() != std::char_traits<char>::eof()) ||
	    !file_identity_is_unchanged(node, state)) {
		if (state.error.empty()) {
			state.error = "host file changed while verifying snapshot";
		}
		return false;
	}
	state.copied_bytes += node.size;
	report_progress(state, "Copying immutable file data", state.copied_bytes, state.source_bytes);
	return true;
}

bool write_node(Node &node, std::vector<unsigned char> &image, BuildState &state) {
	if (node.directory && !directory_identity_is_unchanged(node, state)) {
		return false;
	}
	if (node.parent != nullptr) {
		unsigned char *destination = image.data() + cluster_offset(node.first_cluster);
		if (node.directory) {
			std::array<unsigned char, 11> dot{};
			std::array<unsigned char, 11> dotdot{};
			dot.fill(' ');
			dotdot.fill(' ');
			dot[0] = '.';
			dotdot[0] = '.';
			dotdot[1] = '.';
			write_directory_entry(destination, dot, 0x10, node.first_cluster, 0,
			                      node.fat_timestamp);
			write_directory_entry(destination + 32, dotdot, 0x10,
			                      (node.parent->parent == nullptr) ? 0 : node.parent->first_cluster,
			                      0, node.parent->fat_timestamp);
			for (std::size_t index = 0; index < node.children.size(); index++) {
				const Node &child = *node.children[index];
				write_directory_entry(destination + (index + 2) * 32, child.dos_name,
				                      child.directory ? 0x10 : 0x21, child.first_cluster,
				                      child.size, child.fat_timestamp);
			}
		} else if (!write_stable_file(node, destination, state)) {
			return false;
		}
	}
	for (auto &child : node.children) {
		if (!write_node(*child, image, state)) {
			return false;
		}
	}
	return !node.directory || directory_identity_is_unchanged(node, state);
}

bool build_image(const fs::path &root_path, std::vector<unsigned char> &image, BuildState &state) {
	std::error_code error;
	const fs::path canonical = fs::canonical(root_path, error);
	if (error || !fs::is_directory(canonical, error) || error) {
		state.error = "HOSTFAT root is not a readable directory";
		return false;
	}
#if !defined(_WIN32)
	const fs::file_status root_status = fs::symlink_status(root_path, error);
	if (error || fs::is_symlink(root_status)) {
		state.error = "HOSTFAT root must not be a symbolic link";
		return false;
	}
#endif
	state.canonical_root = canonical;
	Node root;
	root.path = canonical;
	root.directory = true;
	if (!capture_timestamp(root, canonical.u8string(), state) ||
	    !capture_identity(root, canonical.u8string(), state)) {
		return false;
	}
	if (!scan_directory(root, 0, state)) {
		return false;
	}
	if (root.children.size() + 1 > kRootEntries) {
		state.error = "HOSTFAT root exceeds 127 entries";
		return false;
	}
	std::size_t next_cluster = 2;
	if (!assign_clusters(root, next_cluster, state)) {
		return false;
	}
	report_progress(state, "Allocating FAT12-max image", 0, HOSTFAT_IMAGE_SIZE);
	image.assign(HOSTFAT_IMAGE_SIZE, 0);
	report_progress(state, "Creating FAT and directories", 1, 1);
	std::vector<std::uint16_t> fat(kFatEntries, 0);
	fat[0] = 0x0ff0;
	fat[1] = 0x0fff;
	for (std::size_t cluster = kFirstReservedFat12Cluster; cluster < 2 + kDataClusters; cluster++) {
		fat[cluster] = 0x0ff0;
	}
	add_chain(root, fat);
	std::vector<unsigned char> packed_fat(kFatSectors * kSectorSize, 0);
	for (std::size_t entry = 0; entry < kFatEntries; entry += 2) {
		const std::uint16_t first = fat[entry];
		const std::uint16_t second = fat[entry + 1];
		const std::size_t output = (entry / 2) * 3;
		packed_fat[output] = static_cast<unsigned char>(first);
		packed_fat[output + 1] =
		    static_cast<unsigned char>((first >> 8) | ((second & 0x000f) << 4));
		packed_fat[output + 2] = static_cast<unsigned char>(second >> 4);
	}
	for (std::size_t copy = 0; copy < kFatCopies; copy++) {
		std::copy(packed_fat.begin(), packed_fat.end(), image.begin() + copy * packed_fat.size());
	}
	unsigned char *root_directory = image.data() + kFatCopies * kFatSectors * kSectorSize;
	write_directory_entry(root_directory, kVolumeLabel, 0x08, 0, 0, root.fat_timestamp);
	for (std::size_t index = 0; index < root.children.size(); index++) {
		const Node &child = *root.children[index];
		write_directory_entry(root_directory + (index + 1) * 32, child.dos_name,
		                      child.directory ? 0x10 : 0x21, child.first_cluster, child.size,
		                      child.fat_timestamp);
	}
	if (!write_node(root, image, state)) {
		return false;
	}
	report_progress(state, "Snapshot ready to commit", 1, 1);
	return true;
}

bool verify_test_image(const fs::path &source_root) {
	std::error_code timestamp_error;
	FatTimestamp root_timestamp{};
	FatTimestamp docs_timestamp{};
	FatTimestamp hello_timestamp{};
	FatTimestamp readme_timestamp{};
	if (!read_fat_timestamp(source_root, root_timestamp, timestamp_error) ||
	    !read_fat_timestamp(source_root / "docs", docs_timestamp, timestamp_error) ||
	    !read_fat_timestamp(source_root / "hello.txt", hello_timestamp, timestamp_error) ||
	    !read_fat_timestamp(source_root / "docs" / "readme.txt", readme_timestamp,
	                        timestamp_error)) {
		return false;
	}
	auto timestamp_matches = [](const unsigned char *entry, const FatTimestamp &timestamp) {
		return (load_word(entry + 22) == timestamp.time) &&
		       (load_word(entry + 24) == timestamp.date);
	};

	std::array<unsigned char, kFatSectors * kSectorSize> first_fat{};
	std::array<unsigned char, kFatSectors * kSectorSize> second_fat{};
	std::array<unsigned char, HOSTFAT_SECTOR_SIZE> root{};
	for (std::size_t sector = 0; sector < kFatSectors; sector++) {
		if ((hostfat_read_sector(static_cast<UINT32>(sector),
		                         first_fat.data() + sector * kSectorSize) != SUCCESS) ||
		    (hostfat_read_sector(static_cast<UINT32>(kFatSectors + sector),
		                         second_fat.data() + sector * kSectorSize) != SUCCESS)) {
			return false;
		}
	}
	if ((first_fat != second_fat) ||
	    (hostfat_read_sector(kFatSectors * kFatCopies, root.data()) != SUCCESS)) {
		return false;
	}
	if (!std::equal(kVolumeLabel.begin(), kVolumeLabel.end(), root.begin()) || (root[11] != 0x08) ||
	    !timestamp_matches(root.data(), root_timestamp)) {
		return false;
	}
	const unsigned char *docs = root.data() + 32;
	const unsigned char *hello = root.data() + 64;
	const std::array<unsigned char, 11> docs_name = {'D', 'O', 'C', 'S', ' ', ' ',
	                                                 ' ', ' ', ' ', ' ', ' '};
	const std::array<unsigned char, 11> hello_name = {'H', 'E', 'L', 'L', 'O', ' ',
	                                                  ' ', ' ', 'T', 'X', 'T'};
	if (!std::equal(docs_name.begin(), docs_name.end(), docs) ||
	    !std::equal(hello_name.begin(), hello_name.end(), hello) || (docs[11] != 0x10) ||
	    (hello[11] != 0x21) || !timestamp_matches(docs, docs_timestamp) ||
	    !timestamp_matches(hello, hello_timestamp)) {
		return false;
	}
	const std::uint16_t docs_cluster = load_word(docs + 26);
	const std::uint16_t hello_cluster = load_word(hello + 26);
	auto fat_entry = [&first_fat](std::uint16_t cluster) {
		const std::size_t offset = cluster + cluster / 2;
		std::uint16_t value = static_cast<std::uint16_t>(
		    first_fat[offset] | (static_cast<std::uint16_t>(first_fat[offset + 1]) << 8));
		if ((cluster & 1) != 0) {
			value >>= 4;
		}
		return static_cast<std::uint16_t>(value & 0x0fff);
	};
	for (std::uint16_t cluster = kFirstReservedFat12Cluster; cluster < 2 + kDataClusters;
	     cluster++) {
		if (fat_entry(cluster) != 0x0ff0) {
			return false;
		}
	}
	if ((docs_cluster < 2) || (hello_cluster < 2) || (fat_entry(docs_cluster) != 0x0fff) ||
	    (fat_entry(hello_cluster) != 0x0fff)) {
		return false;
	}
	std::array<unsigned char, HOSTFAT_SECTOR_SIZE> sector{};
	const std::size_t hello_sector =
	    kDataStartSector + (static_cast<std::size_t>(hello_cluster) - 2) * kSectorsPerCluster;
	if ((hostfat_read_sector(static_cast<UINT32>(hello_sector), sector.data()) != SUCCESS) ||
	    !std::equal(sector.begin(), sector.begin() + 6, "hello\n")) {
		return false;
	}
	const std::size_t docs_sector =
	    kDataStartSector + (static_cast<std::size_t>(docs_cluster) - 2) * kSectorsPerCluster;
	if (hostfat_read_sector(static_cast<UINT32>(docs_sector), sector.data()) != SUCCESS) {
		return false;
	}
	const std::array<unsigned char, 11> readme_name = {'R', 'E', 'A', 'D', 'M', 'E',
	                                                   ' ', ' ', 'T', 'X', 'T'};
	const std::uint16_t readme_cluster = load_word(sector.data() + 64 + 26);
	return (sector[0] == '.') && (sector[32] == '.') && (sector[33] == '.') &&
	       timestamp_matches(sector.data(), docs_timestamp) &&
	       timestamp_matches(sector.data() + 32, root_timestamp) &&
	       std::equal(readme_name.begin(), readme_name.end(), sector.begin() + 64) &&
	       timestamp_matches(sector.data() + 64, readme_timestamp) && (readme_cluster >= 2) &&
	       (fat_entry(readme_cluster) == 0x0fff);
}

bool verify_internal_limits() {
	const FatTimestamp before_fat = pack_fat_timestamp(1979, 12, 31, 23, 59, 59);
	const FatTimestamp known = pack_fat_timestamp(2026, 7, 22, 12, 34, 57);
	const FatTimestamp after_fat = pack_fat_timestamp(2108, 1, 1, 0, 0, 0);
	if ((before_fat.time != 0) || (before_fat.date != 0x0021) || (known.time != 0x645c) ||
	    (known.date != 0x5cf6) || (after_fat.time != 0xbf7d) || (after_fat.date != 0xff9f)) {
		return false;
	}

	std::array<unsigned char, 11> lowercase{};
	std::array<unsigned char, 11> uppercase{};
	if (!make_dos_name("case.txt", lowercase) || !make_dos_name("CASE.TXT", uppercase) ||
	    (lowercase != uppercase) || make_dos_name("bad name.txt", uppercase) ||
	    !valid_utf8_name("\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88.txt") ||
	    valid_utf8_name("\xc0\xaf")) {
		return false;
	}
	Node collision_root;
	for (const char *name : {"case.txt", "CASE.TXT"}) {
		auto child = std::make_unique<Node>();
		child->source_name = name;
		collision_root.children.push_back(std::move(child));
	}
	BuildState collision_state;
	if (!assign_dos_names(collision_root, collision_state) ||
	    (collision_root.children[0]->dos_name == collision_root.children[1]->dos_name)) {
		return false;
	}
	std::array<unsigned char, 11> alias_first{};
	std::array<unsigned char, 11> alias_second{};
	make_dos_alias("Long Unicode \xe5\x90\x8d.txt", 0, alias_first);
	make_dos_alias("Long Unicode \xe5\x90\x8d.txt", 0, alias_second);
	if ((alias_first != alias_second) || (alias_first[4] != '~') || (alias_first[8] != 'T') ||
	    (alias_first[9] != 'X') || (alias_first[10] != 'T')) {
		return false;
	}
	Node root;
	auto boundary_file = std::make_unique<Node>();
	boundary_file->parent = &root;
	boundary_file->size = 96 * 1024;
	root.children.push_back(std::move(boundary_file));
	std::size_t next_cluster = 2;
	BuildState state;
	if (!assign_clusters(root, next_cluster, state) || (root.children[0]->cluster_count != 6)) {
		return false;
	}
	root.children.clear();
	auto far_filler = std::make_unique<Node>();
	far_filler->parent = &root;
	far_filler->size = 60 * 1024 * 1024;
	root.children.push_back(std::move(far_filler));
	auto far_marker = std::make_unique<Node>();
	far_marker->parent = &root;
	far_marker->size = 4096;
	root.children.push_back(std::move(far_marker));
	next_cluster = 2;
	state = BuildState{};
	if (!assign_clusters(root, next_cluster, state) || (root.children[0]->cluster_count != 3840) ||
	    (root.children[1]->first_cluster != 3842)) {
		return false;
	}
	root.children.clear();
	auto oversized = std::make_unique<Node>();
	oversized->parent = &root;
	oversized->size = HOSTFAT_IMAGE_SIZE;
	root.children.push_back(std::move(oversized));
	next_cluster = 2;
	state = BuildState{};
	if (assign_clusters(root, next_cluster, state)) {
		return false;
	}
	state = BuildState{};
	state.entries = kMaximumEntries;
	if (++state.entries <= kMaximumEntries) {
		return false;
	}
	return true;
}

} // namespace

extern "C" BOOL hostfat_snapshot_build_directory(const char *path,
                                                 HOSTFAT_SNAPSHOT_CANDIDATE **candidate,
                                                 HOSTFAT_SNAPSHOT_PROGRESS progress,
                                                 void *progress_context, char *error,
                                                 UINT error_size) {
	if ((error != nullptr) && (error_size != 0)) {
		error[0] = '\0';
	}
	if (candidate != nullptr) {
		*candidate = nullptr;
	}
	const std::string normalized_path =
	    (path != nullptr) ? vaeg_hostfat::normalize_path(path) : std::string();
	if ((candidate == nullptr) || normalized_path.empty()) {
		set_error(error, error_size, "HOSTFAT directory path is empty");
		return FAILURE;
	}
	try {
		BuildState state;
		state.progress = progress;
		state.progress_context = progress_context;
		auto result = std::unique_ptr<HOSTFAT_SNAPSHOT_CANDIDATE>(new (std::nothrow)
		                                                              HOSTFAT_SNAPSHOT_CANDIDATE());
		if (!result) {
			set_error(error, error_size, "cannot allocate HOSTFAT candidate");
			return FAILURE;
		}
		if (!build_image(vaeg_hostfat::path_from_utf8(normalized_path), result->image, state)) {
			set_error(error, error_size, state.error);
			return FAILURE;
		}
		result->info.files = state.files;
		result->info.directories = state.directories;
		result->info.source_bytes = state.source_bytes;
		report_progress(state, "Preparing immutable snapshot", 0, HOSTFAT_IMAGE_SIZE);
		if (hostfat_prepare_image(result->image.data(), static_cast<UINT32>(result->image.size()),
		                          &result->prepared, &result->info.digest) != SUCCESS) {
			set_error(error, error_size, "cannot prepare HOSTFAT snapshot");
			return FAILURE;
		}
		std::vector<unsigned char>().swap(result->image);
		report_progress(state, "Snapshot ready to commit", 1, 1);
		*candidate = result.release();
		return SUCCESS;
	} catch (const std::exception &exception) {
		set_error(error, error_size, std::string("HOSTFAT snapshot failed: ") + exception.what());
		return FAILURE;
	}
}

extern "C" BOOL hostfat_snapshot_candidate_mount(HOSTFAT_SNAPSHOT_CANDIDATE *candidate,
                                                 HOSTFAT_SNAPSHOT_INFO *info, char *error,
                                                 UINT error_size) {
	if ((error != nullptr) && (error_size != 0)) {
		error[0] = '\0';
	}
	if ((candidate == nullptr) || (candidate->prepared == nullptr) ||
	    (hostfat_commit_prepared_image(candidate->prepared) != SUCCESS)) {
		set_error(error, error_size, "cannot commit HOSTFAT snapshot");
		return FAILURE;
	}
	if (info != nullptr) {
		*info = candidate->info;
	}
	return SUCCESS;
}

extern "C" void hostfat_snapshot_candidate_destroy(HOSTFAT_SNAPSHOT_CANDIDATE *candidate) {
	if (candidate != nullptr) {
		hostfat_destroy_prepared_image(candidate->prepared);
	}
	delete candidate;
}

extern "C" BOOL hostfat_snapshot_mount_directory(const char *path, HOSTFAT_SNAPSHOT_INFO *info,
                                                 char *error, UINT error_size) {
	HOSTFAT_SNAPSHOT_CANDIDATE *candidate = nullptr;
	if (hostfat_snapshot_build_directory(path, &candidate, nullptr, nullptr, error, error_size) !=
	    SUCCESS) {
		return FAILURE;
	}
	const BOOL result = hostfat_snapshot_candidate_mount(candidate, info, error, error_size);
	hostfat_snapshot_candidate_destroy(candidate);
	return result;
}

extern "C" void hostfat_snapshot_unmount(void) {
	hostfat_unmount();
}

extern "C" BOOL hostfat_snapshot_selftest(void) {
	if (!verify_internal_limits()) {
		std::fprintf(stderr, "HOSTFAT selftest detail: internal limits failed\\n");
		return FAILURE;
	}
	std::error_code error;
	const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
	const fs::path temp = fs::temp_directory_path(error);
	fs::path root;
	bool root_created = false;
	for (unsigned attempt = 0; !root_created && (attempt < 100); attempt++) {
		root = temp /
		       ("vaeg-hostfat-selftest-" + std::to_string(nonce) + "-" + std::to_string(attempt));
		error.clear();
		if (fs::create_directory(root, error)) {
			error.clear();
			if (fs::create_directory(root / "docs", error)) {
				root_created = true;
			} else {
				fs::remove_all(root);
			}
		} else if (error == std::errc::file_exists) {
			error.clear();
		}
	}
	if (error || !root_created) {
		std::fprintf(stderr,
		             "HOSTFAT selftest detail: temporary root setup failed path=%s "
		             "error=%s\\n",
		             temp.u8string().c_str(), error.message().c_str());
		return FAILURE;
	}
	bool result = false;
	try {
		{
			std::ofstream hello(root / "hello.txt", std::ios::binary);
			hello << "hello\n";
		}
		{
			std::ofstream readme(root / "docs" / "readme.txt", std::ios::binary);
			readme << "nested\n";
		}
		HOSTFAT_SNAPSHOT_INFO first{};
		HOSTFAT_SNAPSHOT_INFO second{};
		char message[256];
		if (hostfat_snapshot_mount_directory(root.u8string().c_str(), &first, message,
		                                     sizeof(message)) != SUCCESS) {
			throw std::runtime_error(std::string("valid snapshot mount failed: ") + message);
		}
		if ((first.files != 2) || (first.directories != 1) || (first.source_bytes != 13) ||
		    !verify_test_image(root)) {
			throw std::runtime_error("valid snapshot check failed");
		}
		const std::string quoted_root = "\"" + root.u8string() + "\"";
		HOSTFAT_SNAPSHOT_INFO quoted{};
		if ((hostfat_snapshot_mount_directory(quoted_root.c_str(), &quoted, message,
		                                      sizeof(message)) != SUCCESS) ||
		    (quoted.digest != first.digest)) {
			throw std::runtime_error("quoted snapshot path was not accepted");
		}
		// Compare before the Windows reparse tests mutate directory metadata.
		if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), &second, message,
		                                      sizeof(message)) != SUCCESS) ||
		    (first.digest != second.digest)) {
			throw std::runtime_error("snapshot regeneration was not deterministic");
		}
#if defined(_WIN32)
		const fs::path root_link =
		    root.parent_path() / ("vaeg-hostfat-selftest-link-" + std::to_string(nonce));
		error.clear();
		fs::create_directory_symlink(root, root_link, error);
		if (!error) {
			HOSTFAT_SNAPSHOT_INFO linked{};
			if ((hostfat_snapshot_mount_directory(root_link.u8string().c_str(), &linked, message,
			                                      sizeof(message)) != SUCCESS) ||
			    (linked.digest != first.digest)) {
				throw std::runtime_error("Windows root reparse path was rejected");
			}
			error.clear();
			fs::remove(root_link, error);
			if (error) {
				throw std::runtime_error("temporary root-link cleanup failed");
			}
		}
		const fs::path nested_target = root / "empty";
		const fs::path nested_link = root / "empty-link";
		error.clear();
		fs::create_directory(nested_target, error);
		if (!error) {
			error.clear();
			fs::create_directory_symlink(nested_target, nested_link, error);
		}
		if (!error) {
			HOSTFAT_SNAPSHOT_INFO nested{};
			if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), &nested, message,
			                                      sizeof(message)) != SUCCESS) ||
			    (nested.digest == first.digest)) {
				throw std::runtime_error("Windows contained reparse path was not included");
			}
			error.clear();
			fs::remove(nested_link, error);
			if (error) {
				throw std::runtime_error("temporary nested-link cleanup failed");
			}
		}
		error.clear();
		fs::remove(nested_target, error);
		if (error) {
			throw std::runtime_error("temporary nested-target cleanup failed");
		}
#endif
		UINT32 accepted_digest = second.digest;
		{
			std::ofstream aliased(root / "long name.txt", std::ios::binary);
			aliased << "aliased\n";
		}
		HOSTFAT_SNAPSHOT_INFO aliased{};
		if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), &aliased, message,
		                                      sizeof(message)) != SUCCESS) ||
		    (aliased.files != 3) || (aliased.digest == accepted_digest)) {
			throw std::runtime_error("deterministic 8.3 alias was not generated");
		}
		accepted_digest = aliased.digest;
		fs::remove(root / "long name.txt", error);
		if (error) {
			throw std::runtime_error("temporary aliased-name cleanup failed");
		}
		error.clear();
		fs::create_symlink(root / "hello.txt", root / "link.txt", error);
		if (!error) {
			if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), nullptr, message,
			                                      sizeof(message)) == SUCCESS) ||
			    (hostfat_image_digest() != accepted_digest)) {
				throw std::runtime_error("symbolic link was accepted");
			}
			fs::remove(root / "link.txt", error);
			if (error) {
				throw std::runtime_error("temporary symbolic-link cleanup failed");
			}
		}
		error.clear();
		fs::create_hard_link(root / "hello.txt", root / "hard.txt", error);
		if (!error) {
			if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), nullptr, message,
			                                      sizeof(message)) == SUCCESS) ||
			    (hostfat_image_digest() != accepted_digest)) {
				throw std::runtime_error("hard link was accepted");
			}
			fs::remove(root / "hard.txt", error);
			if (error) {
				throw std::runtime_error("temporary hard-link cleanup failed");
			}
		}
		error.clear();
		fs::path nested = root;
		for (unsigned depth = 0; depth <= kMaximumDepth; depth++) {
			nested /= "deep" + std::to_string(depth);
			if (!fs::create_directory(nested, error) || error) {
				throw std::runtime_error("temporary depth-tree creation failed");
			}
		}
		if ((hostfat_snapshot_mount_directory(root.u8string().c_str(), nullptr, message,
		                                      sizeof(message)) == SUCCESS) ||
		    (hostfat_image_digest() != accepted_digest)) {
			throw std::runtime_error("excessive directory depth was accepted");
		}
		result = true;
	} catch (const std::exception &exception) {
		std::fprintf(stderr, "HOSTFAT selftest detail: %s\n", exception.what());
		result = false;
	} catch (...) {
		std::fprintf(stderr, "HOSTFAT selftest detail: unknown exception\n");
		result = false;
	}
	hostfat_snapshot_unmount();
	error.clear();
	fs::remove_all(root, error);
	return result ? SUCCESS : FAILURE;
}
