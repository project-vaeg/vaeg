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
#ifndef VAEG_SDL2_HOSTFAT_PATH_H
#define VAEG_SDL2_HOSTFAT_PATH_H

#include <filesystem>
#include <string>

namespace vaeg_hostfat {

inline std::string normalize_path(const std::string &input) {
	std::size_t begin = 0;
	std::size_t end = input.size();
	while ((begin < end) && (input[begin] == ' ' || input[begin] == '\t')) {
		begin++;
	}
	while ((end > begin) && (input[end - 1] == ' ' || input[end - 1] == '\t')) {
		end--;
	}
	if ((end - begin >= 2) && (input[begin] == '"') && (input[end - 1] == '"')) {
		begin++;
		end--;
	}
	return input.substr(begin, end - begin);
}

inline std::filesystem::path path_from_utf8(const std::string &input) {
	return std::filesystem::u8path(normalize_path(input));
}

} // namespace vaeg_hostfat

#endif
