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
#include "compiler.h"
#include "romankana.h"

typedef struct {
	const char *roman;
	const char *token;
} ROMANKANA_RULE;

static const ROMANKANA_RULE rules[] = {
    {"kya", "kya"},   {"kyu", "kyu"},  {"kyo", "kyo"},  {"sha", "sha"}, {"shu", "shu"},
    {"sho", "sho"},   {"sya", "sha"},  {"syu", "shu"},  {"syo", "sho"}, {"cha", "cha"},
    {"chu", "chu"},   {"cho", "cho"},  {"cya", "cha"},  {"cyu", "chu"}, {"cyo", "cho"},
    {"tya", "cha"},   {"tyu", "chu"},  {"tyo", "cho"},  {"nya", "nya"}, {"nyu", "nyu"},
    {"nyo", "nyo"},   {"hya", "hya"},  {"hyu", "hyu"},  {"hyo", "hyo"}, {"mya", "mya"},
    {"myu", "myu"},   {"myo", "myo"},  {"rya", "rya"},  {"ryu", "ryu"}, {"ryo", "ryo"},
    {"gya", "gya"},   {"gyu", "gyu"},  {"gyo", "gyo"},  {"ja", "ja"},   {"ju", "ju"},
    {"jo", "jo"},     {"jya", "ja"},   {"jyu", "ju"},   {"jyo", "jo"},  {"zya", "ja"},
    {"zyu", "ju"},    {"zyo", "jo"},   {"bya", "bya"},  {"byu", "byu"}, {"byo", "byo"},
    {"pya", "pya"},   {"pyu", "pyu"},  {"pyo", "pyo"},  {"va", "va"},   {"vi", "vi"},
    {"vu", "vu"},     {"ve", "ve"},    {"vo", "vo"},    {"xya", "xya"}, {"xyu", "xyu"},
    {"xyo", "xyo"},   {"lya", "xya"},  {"lyu", "xyu"},  {"lyo", "xyo"}, {"xa", "xa"},
    {"xi", "xi"},     {"xu", "xu"},    {"xe", "xe"},    {"xo", "xo"},   {"la", "xa"},
    {"li", "xi"},     {"lu", "xu"},    {"le", "xe"},    {"lo", "xo"},   {"xtsu", "xtsu"},
    {"ltsu", "xtsu"}, {"xtu", "xtsu"}, {"ltu", "xtsu"}, {"shi", "shi"}, {"si", "shi"},
    {"chi", "chi"},   {"tsu", "tsu"},  {"tu", "tsu"},   {"ka", "ka"},   {"ki", "ki"},
    {"ku", "ku"},     {"ke", "ke"},    {"ko", "ko"},    {"sa", "sa"},   {"su", "su"},
    {"se", "se"},     {"so", "so"},    {"ta", "ta"},    {"te", "te"},   {"to", "to"},
    {"na", "na"},     {"ni", "ni"},    {"nu", "nu"},    {"ne", "ne"},   {"no", "no"},
    {"ha", "ha"},     {"hi", "hi"},    {"fu", "fu"},    {"he", "he"},   {"ho", "ho"},
    {"ma", "ma"},     {"mi", "mi"},    {"mu", "mu"},    {"me", "me"},   {"mo", "mo"},
    {"ya", "ya"},     {"yu", "yu"},    {"yo", "yo"},    {"ra", "ra"},   {"ri", "ri"},
    {"ru", "ru"},     {"re", "re"},    {"ro", "ro"},    {"wa", "wa"},   {"wo", "wo"},
    {"ga", "ga"},     {"gi", "gi"},    {"gu", "gu"},    {"ge", "ge"},   {"go", "go"},
    {"za", "za"},     {"ji", "ji"},    {"zu", "zu"},    {"ze", "ze"},   {"zo", "zo"},
    {"da", "da"},     {"de", "de"},    {"do", "do"},    {"ba", "ba"},   {"bi", "bi"},
    {"bu", "bu"},     {"be", "be"},    {"bo", "bo"},    {"pa", "pa"},   {"pi", "pi"},
    {"pu", "pu"},     {"pe", "pe"},    {"po", "po"},    {"a", "a"},     {"i", "i"},
    {"u", "u"},       {"e", "e"},      {"o", "o"}};

static int is_vowel(char c) {
	return (c == 'a') || (c == 'i') || (c == 'u') || (c == 'e') || (c == 'o');
}

static int is_consonant(char c) {
	return ((c >= 'a') && (c <= 'z') && !is_vowel(c));
}

static int rule_prefix(const char *buf, UINT len, const char *rule) {
	UINT i;

	for (i = 0; i < len; i++) {
		if (rule[i] == '\0') {
			return 0;
		}
		if (buf[i] != rule[i]) {
			return 0;
		}
	}
	return 1;
}

static void consume(ROMANKANA_STATE *state, UINT count) {
	UINT i;

	if (count >= state->length) {
		state->length = 0;
		state->buffer[0] = '\0';
		return;
	}
	for (i = 0; i < state->length - count; i++) {
		state->buffer[i] = state->buffer[i + count];
	}
	state->length -= count;
	state->buffer[state->length] = '\0';
}

static int emit_token(ROMANKANA_EMIT emit, void *arg, const char *token) {
	if (emit != NULL) {
		emit(token, arg);
	}
	return 1;
}

static int process(ROMANKANA_STATE *state, ROMANKANA_EMIT emit, void *arg, int final) {
	int produced;
	UINT i;

	produced = 0;
	while (state->length > 0) {
		if ((state->length >= 2) && is_consonant(state->buffer[0]) &&
		    (state->buffer[0] == state->buffer[1]) && (state->buffer[0] != 'n')) {
			produced += emit_token(emit, arg, "xtsu");
			consume(state, 1);
			continue;
		}
		if ((state->length >= 2) && (state->buffer[0] == 'n') && (state->buffer[1] == '\'')) {
			produced += emit_token(emit, arg, "nn");
			consume(state, 2);
			continue;
		}
		if ((state->length >= 2) && (state->buffer[0] == 'n') && (state->buffer[1] == 'n')) {
			produced += emit_token(emit, arg, "nn");
			consume(state, 2);
			continue;
		}
		if ((state->length >= 2) && (state->buffer[0] == 'n') && is_consonant(state->buffer[1]) &&
		    (state->buffer[1] != 'y') && (state->buffer[1] != 'n')) {
			produced += emit_token(emit, arg, "nn");
			consume(state, 1);
			continue;
		}
		for (i = 0; i < NELEMENTS(rules); i++) {
			const char *roman = rules[i].roman;
			UINT len = (UINT)strlen(roman);
			if ((state->length >= len) && !memcmp(state->buffer, roman, len)) {
				produced += emit_token(emit, arg, rules[i].token);
				consume(state, len);
				break;
			}
		}
		if (i < NELEMENTS(rules)) {
			continue;
		}
		for (i = 0; i < NELEMENTS(rules); i++) {
			if (rule_prefix(state->buffer, state->length, rules[i].roman)) {
				return produced;
			}
		}
		if ((state->length == 1) && (state->buffer[0] == 'n') && !final) {
			return produced;
		}
		if ((state->length == 1) && (state->buffer[0] == 'n') && final) {
			produced += emit_token(emit, arg, "nn");
			consume(state, 1);
			continue;
		}
		produced += emit_token(emit, arg, "?");
		consume(state, 1);
	}
	return produced;
}

void romankana_reset(ROMANKANA_STATE *state) {
	if (state != NULL) {
		state->length = 0;
		state->buffer[0] = '\0';
	}
}

int romankana_feed(ROMANKANA_STATE *state, const char *text, ROMANKANA_EMIT emit, void *arg) {
	int produced;
	char c;

	if ((state == NULL) || (text == NULL)) {
		return 0;
	}
	produced = 0;
	while ((c = *text++) != '\0') {
		if ((c >= 'A') && (c <= 'Z')) {
			c = (char)(c - 'A' + 'a');
		}
		if (((c < 'a') || (c > 'z')) && (c != '\'')) {
			produced += emit_token(emit, arg, "?");
			romankana_reset(state);
			continue;
		}
		if (state->length + 1 >= sizeof(state->buffer)) {
			produced += emit_token(emit, arg, "?");
			romankana_reset(state);
		}
		state->buffer[state->length++] = c;
		state->buffer[state->length] = '\0';
		produced += process(state, emit, arg, 0);
	}
	return produced;
}

int romankana_flush(ROMANKANA_STATE *state, ROMANKANA_EMIT emit, void *arg) {
	if (state == NULL) {
		return 0;
	}
	return process(state, emit, arg, 1);
}
