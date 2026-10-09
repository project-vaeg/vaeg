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

#include <cstdio>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "hostfat.h"
#include "hostfat_manager.h"

namespace {

namespace fs = std::filesystem;

struct ManagerState {
	SDL_mutex *mutex = nullptr;
	SDL_Thread *thread = nullptr;
	std::string path;
	HOSTFAT_SNAPSHOT_CANDIDATE *candidate = nullptr;
	HOSTFAT_SNAPSHOT_INFO candidate_info{};
	HOSTFAT_MANAGER_STATUS status{};
	bool worker_done = false;
	bool worker_success = false;
};

ManagerState g_manager;

void set_text(char *destination, std::size_t size, const char *source) {
	if (size != 0) {
		std::snprintf(destination, size, "%s", (source != nullptr) ? source : "");
	}
}

void set_error(char *destination, UINT size, const char *source) {
	if ((destination != nullptr) && (size != 0)) {
		set_text(destination, size, source);
	}
}

void progress_callback(void *, const char *phase, UINT64 completed, UINT64 total) {
	SDL_LockMutex(g_manager.mutex);
	set_text(g_manager.status.phase, sizeof(g_manager.status.phase), phase);
	g_manager.status.completed = completed;
	g_manager.status.total = total;
	SDL_UnlockMutex(g_manager.mutex);
}

int build_thread(void *) {
	HOSTFAT_SNAPSHOT_CANDIDATE *candidate = nullptr;
	char error[256]{};
	const BOOL success = hostfat_snapshot_build_directory(
	    g_manager.path.c_str(), &candidate, progress_callback, nullptr, error, sizeof(error));
	SDL_LockMutex(g_manager.mutex);
	g_manager.candidate = candidate;
	g_manager.worker_success = success == SUCCESS;
	g_manager.worker_done = true;
	if (success != SUCCESS) {
		set_text(g_manager.status.message, sizeof(g_manager.status.message), error);
	}
	SDL_UnlockMutex(g_manager.mutex);
	return success == SUCCESS ? 0 : 1;
}

void set_mounted_status(const HOSTFAT_SNAPSHOT_INFO &info) {
	g_manager.status.state = HOSTFAT_MANAGER_MOUNTED;
	g_manager.status.mounted = TRUE;
	g_manager.status.completed = 1;
	g_manager.status.total = 1;
	g_manager.status.info = info;
	set_text(g_manager.status.phase, sizeof(g_manager.status.phase), "Mounted");
	std::snprintf(g_manager.status.message, sizeof(g_manager.status.message),
	              "Read-only snapshot mounted: %u files, %u directories, %llu bytes, "
	              "digest %08x",
	              info.files, info.directories, static_cast<unsigned long long>(info.source_bytes),
	              info.digest);
}

} // namespace

extern "C" BOOL hostfat_manager_initialize(void) {
	if (g_manager.mutex != nullptr) {
		return SUCCESS;
	}
	g_manager.mutex = SDL_CreateMutex();
	if (g_manager.mutex == nullptr) {
		return FAILURE;
	}
	g_manager.status.state = HOSTFAT_MANAGER_UNMOUNTED;
	set_text(g_manager.status.phase, sizeof(g_manager.status.phase), "Unmounted");
	return SUCCESS;
}

extern "C" void hostfat_manager_shutdown(void) {
	if (g_manager.mutex == nullptr) {
		hostfat_snapshot_unmount();
		return;
	}
	if (g_manager.thread != nullptr) {
		SDL_WaitThread(g_manager.thread, nullptr);
		g_manager.thread = nullptr;
	}
	if (g_manager.candidate != nullptr) {
		hostfat_snapshot_candidate_destroy(g_manager.candidate);
		g_manager.candidate = nullptr;
	}
	hostfat_snapshot_unmount();
	SDL_DestroyMutex(g_manager.mutex);
	g_manager = ManagerState{};
}

extern "C" BOOL hostfat_manager_mount_startup(const char *path, HOSTFAT_SNAPSHOT_INFO *info,
                                              char *error, UINT error_size) {
	if (g_manager.mutex == nullptr) {
		set_error(error, error_size, "HOSTFAT manager is not initialized");
		return FAILURE;
	}
	HOSTFAT_SNAPSHOT_INFO mounted{};
	if (hostfat_snapshot_mount_directory(path, &mounted, error, error_size) != SUCCESS) {
		return FAILURE;
	}
	SDL_LockMutex(g_manager.mutex);
	set_mounted_status(mounted);
	SDL_UnlockMutex(g_manager.mutex);
	if (info != nullptr) {
		*info = mounted;
	}
	return SUCCESS;
}

extern "C" BOOL hostfat_manager_rebuild_async(const char *path, char *error, UINT error_size) {
	if ((g_manager.mutex == nullptr) || (path == nullptr) || (path[0] == '\0')) {
		set_error(error, error_size, "HOSTFAT directory path is empty");
		return FAILURE;
	}
	SDL_LockMutex(g_manager.mutex);
	if (g_manager.thread != nullptr) {
		SDL_UnlockMutex(g_manager.mutex);
		set_error(error, error_size, "HOSTFAT snapshot build is already running");
		return FAILURE;
	}
	g_manager.path = path;
	g_manager.worker_done = false;
	g_manager.worker_success = false;
	g_manager.status.state = HOSTFAT_MANAGER_BUILDING;
	g_manager.status.mounted = hostfat_is_mounted();
	g_manager.status.completed = 0;
	g_manager.status.total = 0;
	set_text(g_manager.status.phase, sizeof(g_manager.status.phase), "Starting snapshot build");
	g_manager.status.message[0] = '\0';
	SDL_UnlockMutex(g_manager.mutex);

	SDL_Thread *thread = SDL_CreateThread(build_thread, "hostfat-build", nullptr);
	if (thread == nullptr) {
		SDL_LockMutex(g_manager.mutex);
		g_manager.status.state = HOSTFAT_MANAGER_ERROR;
		set_text(g_manager.status.message, sizeof(g_manager.status.message), SDL_GetError());
		SDL_UnlockMutex(g_manager.mutex);
		set_error(error, error_size, SDL_GetError());
		return FAILURE;
	}
	SDL_LockMutex(g_manager.mutex);
	g_manager.thread = thread;
	SDL_UnlockMutex(g_manager.mutex);
	return SUCCESS;
}

extern "C" UINT hostfat_manager_poll(void) {
	if (g_manager.mutex == nullptr) {
		return HOSTFAT_MANAGER_EVENT_NONE;
	}
	SDL_LockMutex(g_manager.mutex);
	if ((g_manager.thread == nullptr) || !g_manager.worker_done) {
		SDL_UnlockMutex(g_manager.mutex);
		return HOSTFAT_MANAGER_EVENT_NONE;
	}
	SDL_Thread *thread = g_manager.thread;
	HOSTFAT_SNAPSHOT_CANDIDATE *candidate = g_manager.candidate;
	const bool success = g_manager.worker_success;
	g_manager.thread = nullptr;
	g_manager.candidate = nullptr;
	g_manager.worker_done = false;
	SDL_UnlockMutex(g_manager.mutex);
	SDL_WaitThread(thread, nullptr);

	UINT event = HOSTFAT_MANAGER_EVENT_FAILED;
	HOSTFAT_SNAPSHOT_INFO info{};
	char error[256]{};
	if (success && (candidate != nullptr) &&
	    (hostfat_snapshot_candidate_mount(candidate, &info, error, sizeof(error)) == SUCCESS)) {
		event = HOSTFAT_MANAGER_EVENT_MOUNTED;
	}
	hostfat_snapshot_candidate_destroy(candidate);

	SDL_LockMutex(g_manager.mutex);
	if (event == HOSTFAT_MANAGER_EVENT_MOUNTED) {
		set_mounted_status(info);
	} else {
		g_manager.status.state = HOSTFAT_MANAGER_ERROR;
		g_manager.status.mounted = hostfat_is_mounted();
		if (error[0] != '\0') {
			set_text(g_manager.status.message, sizeof(g_manager.status.message), error);
		}
		set_text(g_manager.status.phase, sizeof(g_manager.status.phase), "Snapshot build failed");
	}
	SDL_UnlockMutex(g_manager.mutex);
	return event;
}

extern "C" BOOL hostfat_manager_unmount(char *error, UINT error_size) {
	if (g_manager.mutex == nullptr) {
		set_error(error, error_size, "HOSTFAT manager is not initialized");
		return FAILURE;
	}
	SDL_LockMutex(g_manager.mutex);
	if (g_manager.thread != nullptr) {
		SDL_UnlockMutex(g_manager.mutex);
		set_error(error, error_size, "wait for the HOSTFAT snapshot build before unmounting");
		return FAILURE;
	}
	hostfat_snapshot_unmount();
	ZeroMemory(&g_manager.status, sizeof(g_manager.status));
	g_manager.status.state = HOSTFAT_MANAGER_UNMOUNTED;
	set_text(g_manager.status.phase, sizeof(g_manager.status.phase), "Unmounted");
	set_text(g_manager.status.message, sizeof(g_manager.status.message), "HOSTFAT drive unmounted");
	SDL_UnlockMutex(g_manager.mutex);
	return SUCCESS;
}

extern "C" void hostfat_manager_get_status(HOSTFAT_MANAGER_STATUS *status) {
	if (status == nullptr) {
		return;
	}
	ZeroMemory(status, sizeof(*status));
	if (g_manager.mutex == nullptr) {
		return;
	}
	SDL_LockMutex(g_manager.mutex);
	*status = g_manager.status;
	SDL_UnlockMutex(g_manager.mutex);
}

extern "C" BOOL hostfat_manager_selftest(void) {
	std::error_code filesystem_error;
	const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
	const fs::path temp = fs::temp_directory_path(filesystem_error);
	fs::path root;
	bool root_created = false;
	for (unsigned attempt = 0; !root_created && (attempt < 100); attempt++) {
		root = temp / ("vaeg-hostfat-manager-selftest-" + std::to_string(nonce) + "-" +
		               std::to_string(attempt));
		filesystem_error.clear();
		if (fs::create_directory(root, filesystem_error)) {
			root_created = true;
		} else if (filesystem_error == std::errc::file_exists) {
			filesystem_error.clear();
		}
	}
	if (filesystem_error || !root_created) {
		std::fprintf(stderr, "HOSTFAT manager selftest setup failed path=%s error=%s\\n",
		             root.u8string().c_str(), filesystem_error.message().c_str());
		return FAILURE;
	}
	BOOL result = FAILURE;
	try {
		{
			std::ofstream file(root / "manager.txt", std::ios::binary);
			file << "asynchronous snapshot\n";
			if (!file) {
				throw std::runtime_error("cannot create manager test file");
			}
		}
		if (hostfat_manager_initialize() != SUCCESS) {
			throw std::runtime_error("manager initialization failed");
		}
		char error[256]{};
		if (hostfat_manager_rebuild_async(root.u8string().c_str(), error, sizeof(error)) !=
		    SUCCESS) {
			throw std::runtime_error(error);
		}
		UINT32 started = SDL_GetTicks();
		unsigned polls = 0;
		UINT event = HOSTFAT_MANAGER_EVENT_NONE;
		while ((event == HOSTFAT_MANAGER_EVENT_NONE) && ((SDL_GetTicks() - started) < 30000)) {
			event = hostfat_manager_poll();
			polls++;
			SDL_Delay(1);
		}
		HOSTFAT_MANAGER_STATUS status{};
		hostfat_manager_get_status(&status);
		if ((event != HOSTFAT_MANAGER_EVENT_MOUNTED) || (polls < 2) ||
		    (status.state != HOSTFAT_MANAGER_MOUNTED) || !status.mounted ||
		    (status.info.files != 1)) {
			std::fprintf(stderr,
			             "HOSTFAT manager selftest detail: mount event=%u polls=%u "
			             "state=%u mounted=%d files=%u message=%s\\n",
			             event, polls, status.state, status.mounted, status.info.files,
			             status.message);
			throw std::runtime_error("asynchronous commit failed");
		}
		const UINT32 mounted_digest = hostfat_image_digest();
		const std::string missing_path = (root / "missing").u8string();
		if (hostfat_manager_rebuild_async(missing_path.c_str(), error, sizeof(error)) != SUCCESS) {
			throw std::runtime_error(error);
		}
		started = SDL_GetTicks();
		polls = 0;
		event = HOSTFAT_MANAGER_EVENT_NONE;
		while ((event == HOSTFAT_MANAGER_EVENT_NONE) && ((SDL_GetTicks() - started) < 30000)) {
			event = hostfat_manager_poll();
			polls++;
			SDL_Delay(1);
		}
		hostfat_manager_get_status(&status);
		if ((event != HOSTFAT_MANAGER_EVENT_FAILED) || (status.state != HOSTFAT_MANAGER_ERROR) ||
		    !status.mounted || (hostfat_image_digest() != mounted_digest) ||
		    (hostfat_manager_unmount(error, sizeof(error)) != SUCCESS)) {
			std::fprintf(
			    stderr,
			    "HOSTFAT manager selftest failed rebuild event=%u polls=%u state=%u mounted=%d digest=%08x expected=%08x message=%s error=%s\\n",
			    event, polls, status.state, status.mounted, hostfat_image_digest(), mounted_digest,
			    status.message, error);
			throw std::runtime_error("failed rebuild discarded mounted snapshot");
		}
		result = SUCCESS;
	} catch (...) {
		std::fprintf(stderr, "HOSTFAT manager selftest unknown failure\\n");
		result = FAILURE;
	}
	hostfat_manager_shutdown();
	filesystem_error.clear();
	fs::remove_all(root, filesystem_error);
	return result;
}
