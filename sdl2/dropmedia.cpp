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

#include "sdlapi.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <locale.h>
#include <random>
#include <string>
#include <system_error>
#include <vector>

#if defined(VAEG_HAVE_LIBARCHIVE)
#include <archive.h>
#include <archive_entry.h>
#endif

#include "compiler.h"
#include "diskdrv.h"
#include "dosio.h"
#include "dropmedia.h"
#include "np2.h"
#include "sysmng.h"

namespace {

namespace fs = std::filesystem;

constexpr std::uintmax_t kMaxImageBytes = 64u * 1024u * 1024u;
constexpr std::uintmax_t kMaxExtractedBytes = 128u * 1024u * 1024u;
constexpr std::size_t kMaxArchiveEntries = 4096;
constexpr std::size_t kMaxImageCandidates = 256;
constexpr const char kArchiveSourceDirectoryFile[] = ".source-directory";

struct DiskCandidate {
	std::string path;
	std::string basename;
	std::string source_directory;
};

std::vector<std::string> g_dropped_paths;
std::string g_status;
std::string g_session_references[2];
std::string g_mounted_archive_paths[2];
std::string g_archive_source_directories[2];

static std::string ascii_lower(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

static std::string path_extension(const std::string &path) {
	return ascii_lower(fs::u8path(path).extension().u8string());
}

static std::string display_file_name(const std::string &path) {
	std::string name = fs::u8path(path).filename().u8string();
	const std::size_t separator = name.find_last_of("/\\");
	if (separator != std::string::npos) {
		name.erase(0, separator + 1);
	}
	return name;
}

static const char *const kImageExtensions[] = {".d88", ".88d", ".d98", ".98d", ".fdi", ".xdf",
                                               ".hdm", ".dup", ".2hd", ".tfd", ".img"};

static bool image_extension_supported(const std::string &path) {
	const std::string extension = path_extension(path);

	return std::any_of(std::begin(kImageExtensions), std::end(kImageExtensions),
	                   [&extension](const char *candidate) {
		                   return extension == candidate;
	                   });
}

static std::string compressed_image_basename(const std::string &path) {
	const std::string name = fs::u8path(path).filename().u8string();
	const std::string lower_name = ascii_lower(name);

	for (const char *image_extension : kImageExtensions) {
		const std::string suffix = std::string(image_extension) + ".xz";
		if ((lower_name.size() >= suffix.size()) &&
		    (lower_name.compare(lower_name.size() - suffix.size(), suffix.size(), suffix) == 0)) {
			return name.substr(0, name.size() - 3);
		}
	}
	return std::string();
}

static bool compressed_image_extension_supported(const std::string &path) {
	return !compressed_image_basename(path).empty();
}

static bool archive_extension_supported(const std::string &path) {
	const std::string extension = path_extension(path);

	return (extension == ".zip") || (extension == ".7z") || (extension == ".lzh") ||
	       compressed_image_extension_supported(path);
}

static bool candidate_less(const DiskCandidate &left, const DiskCandidate &right) {
	const std::string left_name = ascii_lower(left.basename);
	const std::string right_name = ascii_lower(right.basename);

	if (left_name != right_name) {
		return left_name < right_name;
	}
	return left.path < right.path;
}

static bool archive_path_safe(const char *entry_path) {
	std::string normalized;
	fs::path path;

	if ((entry_path == nullptr) || (entry_path[0] == '\0')) {
		return false;
	}
	normalized = entry_path;
	std::replace(normalized.begin(), normalized.end(), '\\', '/');
	if ((normalized.size() >= 3) && std::isalpha(static_cast<unsigned char>(normalized[0])) &&
	    (normalized[1] == ':') && (normalized[2] == '/')) {
		return false;
	}
	path = fs::u8path(normalized);
	if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
		return false;
	}
	for (const fs::path &component : path) {
		const std::string name = component.u8string();
		if ((name == "..") || name.empty()) {
			return false;
		}
	}
	return true;
}

#if defined(VAEG_HAVE_LIBARCHIVE)
#if !defined(_WIN32)
class ScopedArchiveLocale {
  public:
	ScopedArchiveLocale() : locale_(create_utf8_locale()), previous_(nullptr) {
		if (locale_ != nullptr) {
			previous_ = uselocale(locale_);
		}
	}

	~ScopedArchiveLocale() {
		if (previous_ != nullptr) {
			uselocale(previous_);
		}
		if (locale_ != nullptr) {
			freelocale(locale_);
		}
	}

	ScopedArchiveLocale(const ScopedArchiveLocale &) = delete;
	ScopedArchiveLocale &operator=(const ScopedArchiveLocale &) = delete;

  private:
	static locale_t create_utf8_locale() {
		static const char *names[] = {"C.UTF-8", "C.utf8", "en_US.UTF-8", ""};

		for (const char *name : names) {
			locale_t locale = newlocale(LC_CTYPE_MASK, name, nullptr);
			if (locale != nullptr) {
				return locale;
			}
		}
		return nullptr;
	}

	locale_t locale_;
	locale_t previous_;
};
#endif

static fs::path archive_storage_root(void) {
	char path[MAX_PATH];

	file_getstatepath(path, sizeof(path), "archive-drop");
	return fs::u8path(path);
}

static bool path_is_within(const fs::path &path, const fs::path &directory) {
	const fs::path normalized_path = path.lexically_normal();
	const fs::path normalized_directory = directory.lexically_normal();
	auto path_part = normalized_path.begin();
	auto directory_part = normalized_directory.begin();

	for (; directory_part != normalized_directory.end(); directory_part++, path_part++) {
		if ((path_part == normalized_path.end()) || (*path_part != *directory_part)) {
			return false;
		}
	}
	return true;
}

static std::string archive_source_directory(const std::string &archive_path) {
	std::error_code ec;
	fs::path path = fs::absolute(fs::u8path(archive_path), ec);

	if (ec) {
		path = fs::u8path(archive_path);
	}
	path = path.parent_path();
	return path.empty() ? std::string() : path.u8string();
}

static void write_archive_source_directory(const fs::path &archive_directory,
                                           const std::string &source_directory) {
	if (source_directory.empty() || (source_directory.size() >= MAX_PATH)) {
		return;
	}
	std::ofstream output(archive_directory / kArchiveSourceDirectoryFile,
	                     std::ios::binary | std::ios::trunc);
	if (output) {
		output.write(source_directory.data(),
		             static_cast<std::streamsize>(source_directory.size()));
	}
}

static fs::path managed_archive_directory(const std::string &mounted_path,
                                          const fs::path &storage_root) {
	std::error_code ec;
	const fs::path root = storage_root.lexically_normal();
	fs::path path = fs::absolute(fs::u8path(mounted_path), ec);

	if (ec) {
		path = fs::u8path(mounted_path);
	}
	path = path.lexically_normal();
	if (!path_is_within(path, root)) {
		return fs::path();
	}
	const fs::path relative = path.lexically_relative(root);
	if (relative.empty()) {
		return fs::path();
	}
	auto component = relative.begin();
	if (component == relative.end()) {
		return fs::path();
	}
	const fs::path batch_name = *component++;
	if (batch_name.u8string().rfind("drop-", 0) != 0) {
		return fs::path();
	}
	if (component == relative.end()) {
		return fs::path();
	}
	const fs::path image_name = *component;
	const std::string image_name_string = image_name.u8string();
	if ((image_name_string.size() != 4) ||
	    (!std::all_of(image_name_string.begin(), image_name_string.end(), [](unsigned char ch) {
		    return std::isdigit(ch) != 0;
	    }))) {
		return fs::path();
	}
	return root / batch_name / image_name;
}

static std::string read_archive_source_directory(const std::string &mounted_path,
                                                 const fs::path &storage_root) {
	const fs::path archive_directory = managed_archive_directory(mounted_path, storage_root);
	char buffer[MAX_PATH];

	if (archive_directory.empty()) {
		return std::string();
	}
	std::ifstream input(archive_directory / kArchiveSourceDirectoryFile, std::ios::binary);
	if (!input) {
		return std::string();
	}
	input.read(buffer, sizeof(buffer));
	const std::streamsize size = input.gcount();
	if ((size <= 0) || (size >= static_cast<std::streamsize>(sizeof(buffer)))) {
		return std::string();
	}
	const std::string source_directory(buffer, static_cast<std::size_t>(size));
	std::error_code ec;
	if ((source_directory.find('\0') != std::string::npos) ||
	    (!fs::is_directory(fs::u8path(source_directory), ec)) || ec) {
		return std::string();
	}
	return source_directory;
}

static std::string read_archive_source_directory(const std::string &mounted_path) {
	return read_archive_source_directory(mounted_path, archive_storage_root());
}

static fs::path create_archive_directory(const fs::path &root, std::string *error) {
	std::error_code ec;
	std::random_device random;

	if ((!fs::create_directories(root, ec)) && ec) {
		*error = "archive storage directory is unavailable";
		return fs::path();
	}
	for (unsigned int attempt = 0; attempt < 64; attempt++) {
		char name[64];
		std::snprintf(name, sizeof(name), "drop-%08x-%02u", random(), attempt);
		fs::path candidate = root / name;
		if (fs::create_directory(candidate, ec)) {
			return candidate;
		}
		if (ec && (ec != std::errc::file_exists)) {
			break;
		}
		ec.clear();
	}
	*error = "could not create an archive storage directory";
	return fs::path();
}

static bool write_test_archive(const fs::path &path, bool seven_zip, const char *entry_path) {
#if !defined(_WIN32)
	ScopedArchiveLocale archive_locale;
#endif
	struct archive *writer = archive_write_new();
	struct archive_entry *entry;
	static const char payload[] = "D88";
	int result;

	if (writer == nullptr) {
		return false;
	}
	result =
	    seven_zip ? archive_write_set_format_7zip(writer) : archive_write_set_format_zip(writer);
	if ((!seven_zip) && (result == ARCHIVE_OK)) {
		result = archive_write_set_options(writer, "zip:compression=deflate");
	}
	if (result == ARCHIVE_OK) {
		result = archive_write_open_filename(writer, path.u8string().c_str());
	}
	entry = archive_entry_new();
	if ((result != ARCHIVE_OK) || (entry == nullptr)) {
		if (entry != nullptr) {
			archive_entry_free(entry);
		}
		archive_write_free(writer);
		return false;
	}
	archive_entry_set_pathname_utf8(entry, entry_path);
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0600);
	archive_entry_set_size(entry, sizeof(payload) - 1);
	result = archive_write_header(writer, entry);
	if (result == ARCHIVE_OK) {
		result = (archive_write_data(writer, payload, sizeof(payload) - 1) ==
		          static_cast<la_ssize_t>(sizeof(payload) - 1))
		             ? ARCHIVE_OK
		             : ARCHIVE_FATAL;
	}
	archive_entry_free(entry);
	if (archive_write_close(writer) != ARCHIVE_OK) {
		result = ARCHIVE_FATAL;
	}
	archive_write_free(writer);
	return result == ARCHIVE_OK;
}

static bool write_test_xz_image(const fs::path &path) {
#if !defined(_WIN32)
	ScopedArchiveLocale archive_locale;
#endif
	struct archive *writer = archive_write_new();
	struct archive_entry *entry;
	static const char payload[] = "D88";
	int result;

	if (writer == nullptr) {
		return false;
	}
	result = archive_write_set_format_raw(writer);
	if (result == ARCHIVE_OK) {
		result = archive_write_add_filter_xz(writer);
	}
	if (result == ARCHIVE_OK) {
		result = archive_write_open_filename(writer, path.u8string().c_str());
	}
	entry = archive_entry_new();
	if ((result != ARCHIVE_OK) || (entry == nullptr)) {
		if (entry != nullptr) {
			archive_entry_free(entry);
		}
		archive_write_free(writer);
		return false;
	}
	archive_entry_set_pathname_utf8(entry, "test.d88");
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0600);
	archive_entry_set_size(entry, sizeof(payload) - 1);
	result = archive_write_header(writer, entry);
	if (result == ARCHIVE_OK) {
		result = (archive_write_data(writer, payload, sizeof(payload) - 1) ==
		          static_cast<la_ssize_t>(sizeof(payload) - 1))
		             ? ARCHIVE_OK
		             : ARCHIVE_FATAL;
	}
	archive_entry_free(entry);
	if (archive_write_close(writer) != ARCHIVE_OK) {
		result = ARCHIVE_FATAL;
	}
	archive_write_free(writer);
	return result == ARCHIVE_OK;
}

static bool extract_xz_image(const std::string &image_path, std::vector<DiskCandidate> *images,
                             std::string *error, const fs::path &storage_root) {
	const std::string source_directory = archive_source_directory(image_path);
	const std::string basename = compressed_image_basename(image_path);
	struct archive *reader;
	struct archive_entry *entry;
	fs::path temp_dir;
	fs::path output_dir;
	fs::path output_path;
	std::uintmax_t image_bytes = 0;
	std::uintmax_t total_bytes = 0;
	char buffer[16384];
	int result;

	if (basename.empty()) {
		*error = "compressed image has an unsupported name";
		return false;
	}
	temp_dir = create_archive_directory(storage_root, error);
	if (temp_dir.empty()) {
		return false;
	}
	reader = archive_read_new();
	if (reader == nullptr) {
		fs::remove_all(temp_dir);
		*error = "libarchive initialization failed";
		return false;
	}
	archive_read_support_filter_all(reader);
	result = archive_read_support_format_raw(reader);
	if (result != ARCHIVE_OK) {
		archive_read_free(reader);
		fs::remove_all(temp_dir);
		*error = "raw archive support is unavailable";
		return false;
	}
#if defined(_WIN32)
	result = archive_read_open_filename_w(reader, fs::u8path(image_path).c_str(), 10240);
#else
	result = archive_read_open_filename(reader, image_path.c_str(), 10240);
#endif
	if (result != ARCHIVE_OK) {
		*error = archive_error_string(reader) ? archive_error_string(reader) : "image open failed";
		archive_read_free(reader);
		fs::remove_all(temp_dir);
		return false;
	}
	result = archive_read_next_header(reader, &entry);
	if ((result != ARCHIVE_OK) || (archive_entry_filetype(entry) != AE_IFREG) ||
	    (archive_entry_symlink(entry) != nullptr) || (archive_entry_hardlink(entry) != nullptr)) {
		*error = archive_error_string(reader) ? archive_error_string(reader)
		                                      : "compressed image payload is invalid";
		archive_read_free(reader);
		fs::remove_all(temp_dir);
		return false;
	}
	output_dir = temp_dir / "0000";
	std::error_code ec;
	if ((!fs::create_directory(output_dir, ec)) || ec) {
		*error = "could not create an archive extraction directory";
		archive_read_free(reader);
		fs::remove_all(temp_dir);
		return false;
	}
	output_path = output_dir / fs::u8path(basename);
	{
		std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
		if (!output) {
			*error = "could not create an extracted disk image";
			archive_read_free(reader);
			fs::remove_all(temp_dir);
			return false;
		}
		for (;;) {
			const la_ssize_t count = archive_read_data(reader, buffer, sizeof(buffer));
			if (count == 0) {
				break;
			}
			if (count < 0) {
				*error = archive_error_string(reader) ? archive_error_string(reader)
				                                      : "compressed image extraction failed";
				archive_read_free(reader);
				fs::remove_all(temp_dir);
				return false;
			}
			if ((static_cast<std::uintmax_t>(count) > (kMaxImageBytes - image_bytes)) ||
			    (static_cast<std::uintmax_t>(count) > (kMaxExtractedBytes - total_bytes))) {
				*error = "compressed image extraction limit exceeded";
				archive_read_free(reader);
				fs::remove_all(temp_dir);
				return false;
			}
			output.write(buffer, count);
			if (!output) {
				*error = "could not write an extracted disk image";
				archive_read_free(reader);
				fs::remove_all(temp_dir);
				return false;
			}
			image_bytes += static_cast<std::uintmax_t>(count);
			total_bytes += static_cast<std::uintmax_t>(count);
		}
	}
	result = archive_read_next_header(reader, &entry);
	if (result != ARCHIVE_EOF) {
		*error = "compressed image contains more than one payload";
		archive_read_free(reader);
		fs::remove_all(temp_dir);
		return false;
	}
	archive_read_free(reader);
	write_archive_source_directory(output_dir, source_directory);
	if ((output_path.u8string().size() >= MAX_PATH) || (image_bytes == 0)) {
		*error = (image_bytes == 0) ? "compressed image is empty"
		                            : "extracted disk image path is too long";
		fs::remove_all(temp_dir);
		return false;
	}
	images->push_back({output_path.u8string(), basename, source_directory});
	return true;
}

static bool extract_archive_images(const std::string &archive_path,
                                   std::vector<DiskCandidate> *images, std::string *error,
                                   const fs::path &storage_root) {
	struct archive *reader;
	struct archive_entry *entry;
	fs::path temp_dir;
	std::uintmax_t total_bytes;
	std::size_t entry_count;
	std::size_t image_count;
	const std::size_t initial_image_count = images->size();
	const std::string source_directory = archive_source_directory(archive_path);
#if !defined(_WIN32)
	ScopedArchiveLocale archive_locale;
#endif
	int result;

	if (compressed_image_extension_supported(archive_path)) {
		return extract_xz_image(archive_path, images, error, storage_root);
	}

	temp_dir = create_archive_directory(storage_root, error);
	if (temp_dir.empty()) {
		return false;
	}
	reader = archive_read_new();
	if (reader == nullptr) {
		fs::remove_all(temp_dir);
		*error = "libarchive initialization failed";
		return false;
	}
	archive_read_support_filter_all(reader);
	archive_read_support_format_all(reader);
#if defined(_WIN32)
	result = archive_read_open_filename_w(reader, fs::u8path(archive_path).c_str(), 10240);
#else
	result = archive_read_open_filename(reader, archive_path.c_str(), 10240);
#endif
	if (result != ARCHIVE_OK) {
		*error =
		    archive_error_string(reader) ? archive_error_string(reader) : "archive open failed";
		archive_read_free(reader);
		fs::remove_all(temp_dir);
		return false;
	}
	total_bytes = 0;
	entry_count = 0;
	image_count = 0;
	while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK) {
		const char *entry_path = archive_entry_pathname_utf8(entry);
		if (entry_path == nullptr) {
			entry_path = archive_entry_pathname(entry);
		}
		entry_count++;
		if (entry_count > kMaxArchiveEntries) {
			*error = "archive contains too many entries";
			goto extract_error;
		}
		if (!archive_path_safe(entry_path)) {
			*error = "archive contains an unsafe path";
			goto extract_error;
		}
		if ((archive_entry_symlink(entry) != nullptr) ||
		    (archive_entry_hardlink(entry) != nullptr)) {
			*error = "archive links are not allowed";
			goto extract_error;
		}
		if ((archive_entry_filetype(entry) != AE_IFREG) ||
		    (!image_extension_supported(entry_path))) {
			archive_read_data_skip(reader);
			continue;
		}
		if (image_count >= kMaxImageCandidates) {
			*error = "archive contains too many disk images";
			goto extract_error;
		}
		const la_int64_t declared_size = archive_entry_size(entry);
		if ((declared_size < 0) || (static_cast<std::uintmax_t>(declared_size) > kMaxImageBytes) ||
		    (static_cast<std::uintmax_t>(declared_size) > (kMaxExtractedBytes - total_bytes))) {
			*error = "archive disk image exceeds the extraction limit";
			goto extract_error;
		}
		fs::path basename = fs::u8path(entry_path).filename();
		char subdir_name[16];
		std::snprintf(subdir_name, sizeof(subdir_name), "%04zu", image_count);
		fs::path output_dir = temp_dir / subdir_name;
		fs::path output_path = output_dir / basename;
		std::error_code ec;
		if ((!fs::create_directory(output_dir, ec)) || ec) {
			*error = "could not create an archive extraction directory";
			goto extract_error;
		}
		std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
		if (!output) {
			*error = "could not create an extracted disk image";
			goto extract_error;
		}
		std::uintmax_t image_bytes = 0;
		char buffer[16384];
		for (;;) {
			la_ssize_t count = archive_read_data(reader, buffer, sizeof(buffer));
			if (count == 0) {
				break;
			}
			if (count < 0) {
				*error = archive_error_string(reader) ? archive_error_string(reader)
				                                      : "archive extraction failed";
				goto extract_error;
			}
			if ((static_cast<std::uintmax_t>(count) > (kMaxImageBytes - image_bytes)) ||
			    (static_cast<std::uintmax_t>(count) > (kMaxExtractedBytes - total_bytes))) {
				*error = "archive extraction limit exceeded";
				goto extract_error;
			}
			output.write(buffer, count);
			if (!output) {
				*error = "could not write an extracted disk image";
				goto extract_error;
			}
			image_bytes += static_cast<std::uintmax_t>(count);
			total_bytes += static_cast<std::uintmax_t>(count);
		}
		output.close();
		if (!output) {
			*error = "could not finish an extracted disk image";
			goto extract_error;
		}
		write_archive_source_directory(output_dir, source_directory);
		if (output_path.u8string().size() >= MAX_PATH) {
			*error = "extracted disk image path is too long";
			goto extract_error;
		}
		images->push_back({output_path.u8string(), basename.u8string(), source_directory});
		image_count++;
	}
	if (result != ARCHIVE_EOF) {
		*error =
		    archive_error_string(reader) ? archive_error_string(reader) : "archive read failed";
		goto extract_error;
	}
	archive_read_free(reader);
	if (image_count == 0) {
		fs::remove_all(temp_dir);
		*error = "archive contains no supported disk image";
		return false;
	}
	return true;

extract_error:
	archive_read_free(reader);
	fs::remove_all(temp_dir);
	images->resize(initial_image_count);
	return false;
}
#endif

static void remember_archive_source(std::size_t drive, const std::string &mounted_path,
                                    const std::string &source_directory) {
	if (drive >= 2) {
		return;
	}
	if (mounted_path.empty() || source_directory.empty()) {
		g_mounted_archive_paths[drive].clear();
		g_archive_source_directories[drive].clear();
		return;
	}
	g_mounted_archive_paths[drive] = mounted_path;
	g_archive_source_directories[drive] = source_directory;
}

static void append_status_line(const std::string &line) {
	if (!g_status.empty()) {
		g_status += '\n';
	}
	g_status += line;
}

static void mount_candidates(std::vector<DiskCandidate> *images, std::size_t first_drive) {
	if ((images == nullptr) || (first_drive >= 2)) {
		return;
	}
	const std::size_t capacity = 2 - first_drive;
	std::sort(images->begin(), images->end(), candidate_less);
	for (std::size_t index = 0; (index < images->size()) && (index < capacity); index++) {
		const DiskCandidate &image = (*images)[index];
		const std::size_t drive = first_drive + index;

		diskdrv_setfdd(static_cast<REG8>(drive), image.path.c_str(), 0);
		file_cpyname(np2oscfg.fdd_image[drive], image.path.c_str(),
		             sizeof(np2oscfg.fdd_image[drive]));
		remember_archive_source(drive, image.path, image.source_directory);
		sysmng_update(SYS_UPDATEOSCFG);
	}
	if (images->size() > capacity) {
		append_status_line("Ignored " + std::to_string(images->size() - capacity) +
		                   " additional disk image(s).");
	}
	dropmedia_prune_storage();
}

static void process_drop_batch(void) {
	std::vector<DiskCandidate> images;

	g_status.clear();
	for (const std::string &path : g_dropped_paths) {
		std::error_code ec;
		const fs::path fs_path = fs::u8path(path);
		if ((!fs::is_regular_file(fs_path, ec)) || ec) {
			append_status_line("Drop ignored: file not found: " + display_file_name(path));
			continue;
		}
		if (image_extension_supported(path)) {
			if (path.size() >= MAX_PATH) {
				append_status_line("Drop ignored: path is too long: " + display_file_name(path));
				continue;
			}
			images.push_back({path, fs_path.filename().u8string(), std::string()});
			continue;
		}
		if (archive_extension_supported(path)) {
#if defined(VAEG_HAVE_LIBARCHIVE)
			std::string error;
			if (!extract_archive_images(path, &images, &error, archive_storage_root())) {
				append_status_line("Archive drop failed: " + error + ": " +
				                   fs_path.filename().u8string());
			}
#else
			append_status_line("Archive drop unavailable: this build has no LibArchive support.");
#endif
			continue;
		}
		append_status_line("Unsupported dropped file: " + fs_path.filename().u8string());
	}
	if (!images.empty()) {
		mount_candidates(&images, 0);
	} else if (g_status.empty()) {
		g_status = "Drop contained no supported disk image.";
	}
	g_dropped_paths.clear();
}

} // namespace

extern "C" BOOL dropmedia_process_event(const void *event) {
	const SDL_Event *sdl_event;

	if (event == nullptr) {
		return FALSE;
	}
	sdl_event = static_cast<const SDL_Event *>(event);
	switch (sdl_event->type) {
	case SDL_DROPBEGIN:
		g_dropped_paths.clear();
		return TRUE;

	case SDL_DROPFILE:
		if (sdl_event->drop.file != nullptr) {
			g_dropped_paths.emplace_back(sdl_event->drop.file);
			SDL_free(sdl_event->drop.file);
		}
		return TRUE;

	case SDL_DROPCOMPLETE:
		process_drop_batch();
		return TRUE;

	default:
		return FALSE;
	}
}

extern "C" BOOL dropmedia_path_is_archive(const char *path) {
	return ((path != nullptr) && archive_extension_supported(path)) ? TRUE : FALSE;
}

extern "C" BOOL dropmedia_mount_archive(const char *path, UINT first_drive) {
	g_status.clear();
	if ((path == nullptr) || (path[0] == '\0')) {
		g_status = "Archive open failed: path is empty.";
		return FALSE;
	}
	if (first_drive >= 2) {
		g_status = "Archive open failed: invalid FDD drive.";
		return FALSE;
	}
	if (!archive_extension_supported(path)) {
		g_status = "Archive open failed: unsupported archive extension.";
		return FALSE;
	}
	std::error_code ec;
	if ((!fs::is_regular_file(fs::u8path(path), ec)) || ec) {
		g_status = "Archive open failed: file not found.";
		return FALSE;
	}
#if defined(VAEG_HAVE_LIBARCHIVE)
	std::vector<DiskCandidate> images;
	std::string error;

	if (!extract_archive_images(path, &images, &error, archive_storage_root())) {
		g_status = "Archive open failed: " + error + ": " + fs::u8path(path).filename().u8string();
		return FALSE;
	}
	mount_candidates(&images, first_drive);
	return TRUE;
#else
	g_status = "Archive open unavailable: this build has no LibArchive support.";
	return FALSE;
#endif
}

extern "C" BOOL dropmedia_fdd_source_directory(UINT drive, const char *mounted_path,
                                               char *directory, UINT directory_size) {
	if ((directory != nullptr) && (directory_size != 0)) {
		directory[0] = '\0';
	}
	if ((drive >= 2) || (mounted_path == nullptr) || (mounted_path[0] == '\0') ||
	    (directory == nullptr) || (directory_size == 0) ||
	    (g_mounted_archive_paths[drive] != mounted_path) ||
	    g_archive_source_directories[drive].empty() ||
	    (g_archive_source_directories[drive].size() >= directory_size)) {
		return FALSE;
	}
	std::error_code ec;
	if ((!fs::is_directory(fs::u8path(g_archive_source_directories[drive]), ec)) || ec) {
		return FALSE;
	}
	file_cpyname(directory, g_archive_source_directories[drive].c_str(),
	             static_cast<int>(directory_size));
	return TRUE;
}

extern "C" const char *dropmedia_status(void) {
	return g_status.c_str();
}

#if defined(VAEG_HAVE_LIBARCHIVE)
static bool storage_directory_referenced(const fs::path &directory, const char *const *references,
                                         std::size_t reference_count) {
	for (std::size_t index = 0; index < reference_count; index++) {
		if ((references[index] != nullptr) && (references[index][0] != '\0') &&
		    path_is_within(fs::u8path(references[index]), directory)) {
			return true;
		}
	}
	return false;
}

static void prune_storage_root(const fs::path &root, const char *const *references,
                               std::size_t reference_count) {
	std::error_code ec;

	if ((!fs::exists(root, ec)) || ec) {
		return;
	}
	for (const fs::directory_entry &batch : fs::directory_iterator(root, ec)) {
		if (ec) {
			return;
		}
		const std::string batch_name = batch.path().filename().u8string();
		if ((batch_name.rfind("drop-", 0) != 0) || (!batch.is_directory(ec)) || ec) {
			ec.clear();
			continue;
		}
		for (const fs::directory_entry &image : fs::directory_iterator(batch.path(), ec)) {
			if (ec) {
				return;
			}
			const std::string image_name = image.path().filename().u8string();
			if ((image_name.size() != 4) ||
			    (!std::all_of(image_name.begin(), image_name.end(), [](unsigned char ch) {
				    return std::isdigit(ch) != 0;
			    }))) {
				continue;
			}
			if (!storage_directory_referenced(image.path(), references, reference_count)) {
				fs::remove_all(image.path(), ec);
				ec.clear();
			}
		}
		if (fs::is_empty(batch.path(), ec) && !ec) {
			fs::remove(batch.path(), ec);
		}
		ec.clear();
	}
}
#endif

extern "C" void dropmedia_prune_storage(void) {
#if defined(VAEG_HAVE_LIBARCHIVE)
	const char *references[4] = {np2oscfg.fdd_image[0], np2oscfg.fdd_image[1],
	                             g_session_references[0].c_str(), g_session_references[1].c_str()};

	prune_storage_root(archive_storage_root(), references, std::size(references));
#endif
}

extern "C" void dropmedia_initialize(void) {
	g_session_references[0].clear();
	g_session_references[1].clear();
	for (std::size_t drive = 0; drive < 2; drive++) {
		g_mounted_archive_paths[drive].clear();
		g_archive_source_directories[drive].clear();
#if defined(VAEG_HAVE_LIBARCHIVE)
		const std::string source_directory =
		    read_archive_source_directory(np2oscfg.fdd_image[drive]);
		remember_archive_source(drive, np2oscfg.fdd_image[drive], source_directory);
#endif
	}
	dropmedia_prune_storage();
}

extern "C" void dropmedia_set_session_fdd_references(const char *first, const char *second) {
	g_session_references[0] = (first != nullptr) ? first : "";
	g_session_references[1] = (second != nullptr) ? second : "";
}

extern "C" void dropmedia_shutdown(void) {
	g_dropped_paths.clear();
	g_status.clear();
	g_session_references[0].clear();
	g_session_references[1].clear();
	g_mounted_archive_paths[0].clear();
	g_mounted_archive_paths[1].clear();
	g_archive_source_directories[0].clear();
	g_archive_source_directories[1].clear();
}

extern "C" BOOL dropmedia_selftest(void) {
	std::vector<DiskCandidate> candidates = {{"/tmp/b.D88", "b.D88", std::string()},
	                                         {"/tmp/A.d88", "A.d88", std::string()},
	                                         {"/tmp/c.xdf", "c.xdf", std::string()}};

	if ((!image_extension_supported("disk.D88")) || (!image_extension_supported("disk.2HD")) ||
	    (!image_extension_supported("disk.IMG")) || image_extension_supported("disk.zip") ||
	    (!dropmedia_path_is_archive("set.ZIP")) || (!dropmedia_path_is_archive("set.7Z")) ||
	    (!dropmedia_path_is_archive("set.LZH")) || (!dropmedia_path_is_archive("disk.d88.xz")) ||
	    dropmedia_path_is_archive("disk.xz") || dropmedia_path_is_archive("disk.d88") ||
	    dropmedia_path_is_archive(NULL)) {
		return FAILURE;
	}
	if ((!archive_path_safe("folder/disk.d88")) || archive_path_safe("../disk.d88") ||
	    archive_path_safe("folder/../../disk.d88") || archive_path_safe("/absolute/disk.d88") ||
	    archive_path_safe("C:\\absolute\\disk.d88")) {
		return FAILURE;
	}
	std::sort(candidates.begin(), candidates.end(), candidate_less);
	if ((candidates[0].basename != "A.d88") || (candidates[1].basename != "b.D88") ||
	    (candidates[2].basename != "c.xdf")) {
		return FAILURE;
	}
#if defined(VAEG_HAVE_LIBARCHIVE)
	std::string error;
	std::error_code ec;
	fs::path test_dir = fs::temp_directory_path(ec) / "vaeg-dropmedia-selftest";
	fs::remove_all(test_dir, ec);
	if (ec || (!fs::create_directory(test_dir, ec)) || ec) {
		return FAILURE;
	}
	for (bool seven_zip : {false, true}) {
		fs::path archive_path = test_dir / (seven_zip ? "dropmedia-test.7z" : "dropmedia-test.zip");
		const fs::path storage_root = test_dir / "storage";
		std::vector<DiskCandidate> extracted;
		char source_directory[MAX_PATH];
		char short_directory[2];
		const char *entry_path = "nested/test.d88";
		const char *expected_basename = "test.d88";
#if !defined(_WIN32)
		if (seven_zip) {
			entry_path = u8"nested/\u30c6\u30b9\u30c8.d88";
			expected_basename = u8"\u30c6\u30b9\u30c8.d88";
		}
#endif
		if ((!write_test_archive(archive_path, seven_zip, entry_path)) ||
		    (!extract_archive_images(archive_path.u8string(), &extracted, &error, storage_root)) ||
		    (extracted.size() != 1) || (extracted[0].basename != expected_basename) ||
		    (extracted[0].source_directory != test_dir.u8string()) ||
		    (read_archive_source_directory(extracted[0].path, storage_root) !=
		     test_dir.u8string())) {
			fs::remove_all(test_dir);
			dropmedia_shutdown();
			return FAILURE;
		}
		remember_archive_source(0, extracted[0].path, extracted[0].source_directory);
		if ((!dropmedia_fdd_source_directory(0, extracted[0].path.c_str(), source_directory,
		                                     sizeof(source_directory))) ||
		    strcmp(source_directory, test_dir.u8string().c_str()) ||
		    dropmedia_fdd_source_directory(0, extracted[0].path.c_str(), short_directory,
		                                   sizeof(short_directory)) ||
		    dropmedia_fdd_source_directory(0, "different.d88", source_directory,
		                                   sizeof(source_directory))) {
			fs::remove_all(test_dir);
			dropmedia_shutdown();
			return FAILURE;
		}
	}
	{
		const fs::path image_path = test_dir / "sample.d88.xz";
		const fs::path storage_root = test_dir / "storage-xz";
		std::vector<DiskCandidate> extracted;
		if ((!write_test_xz_image(image_path)) ||
		    (!extract_archive_images(image_path.u8string(), &extracted, &error, storage_root)) ||
		    (extracted.size() != 1) || (extracted[0].basename != "sample.d88") ||
		    (extracted[0].source_directory != test_dir.u8string())) {
			fs::remove_all(test_dir);
			dropmedia_shutdown();
			return FAILURE;
		}
		std::ifstream extracted_file(extracted[0].path, std::ios::binary);
		const std::string contents((std::istreambuf_iterator<char>(extracted_file)),
		                           std::istreambuf_iterator<char>());
		if (contents != "D88") {
			fs::remove_all(test_dir);
			dropmedia_shutdown();
			return FAILURE;
		}
	}
	fs::path unsafe_path = test_dir / "dropmedia-unsafe.zip";
	std::vector<DiskCandidate> unsafe_images;
	if ((!write_test_archive(unsafe_path, false, "../escape.d88")) ||
	    extract_archive_images(unsafe_path.u8string(), &unsafe_images, &error,
	                           test_dir / "storage")) {
		fs::remove_all(test_dir);
		dropmedia_shutdown();
		return FAILURE;
	}
	fs::path prune_root = test_dir / "prune";
	fs::path keep_first = prune_root / "drop-one" / "0000" / "first.d88";
	fs::path remove_image = prune_root / "drop-one" / "0001" / "unused.d88";
	fs::path keep_second = prune_root / "drop-two" / "0000" / "second.d88";
	fs::create_directories(keep_first.parent_path(), ec);
	fs::create_directories(remove_image.parent_path(), ec);
	fs::create_directories(keep_second.parent_path(), ec);
	std::ofstream(keep_first).put('\0');
	std::ofstream(remove_image).put('\0');
	std::ofstream(keep_second).put('\0');
	const std::string first_reference = keep_first.u8string();
	const std::string second_reference = keep_second.u8string();
	const char *references[2] = {first_reference.c_str(), second_reference.c_str()};
	prune_storage_root(prune_root, references, std::size(references));
	if ((!fs::exists(keep_first)) || fs::exists(remove_image) || (!fs::exists(keep_second))) {
		fs::remove_all(test_dir);
		dropmedia_shutdown();
		return FAILURE;
	}
	references[0] = "";
	prune_storage_root(prune_root, references, std::size(references));
	if (fs::exists(keep_first) || (!fs::exists(keep_second))) {
		fs::remove_all(test_dir);
		dropmedia_shutdown();
		return FAILURE;
	}
	fs::remove_all(test_dir);
	dropmedia_shutdown();
#endif
	return SUCCESS;
}
