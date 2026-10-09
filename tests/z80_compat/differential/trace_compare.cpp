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

#include "trace_common.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vaeg::z80_compat::differential {
namespace {

using Fields = std::map<std::string, std::string>;

struct ParsedEvent {
	std::string type;
	std::string address;
	std::string data;
	std::string live_pc;
	std::string public_pc;
};

struct Checkpoint {
	Fields fields;
	std::vector<ParsedEvent> events;
};

using Trace = std::map<std::string, Checkpoint>;

Fields ParseFields(const std::string &line) {
	Fields fields;
	std::istringstream stream(line);
	std::string word;
	stream >> word;
	while (stream >> word) {
		const std::size_t equals = word.find('=');
		if (equals != std::string::npos) {
			fields[word.substr(0, equals)] = word.substr(equals + 1);
		}
	}
	return fields;
}

std::string Required(const Fields &fields, const char *name, const std::string &path,
                     std::size_t line) {
	const auto found = fields.find(name);
	if (found == fields.end()) {
		throw std::runtime_error(path + ':' + std::to_string(line) + ": missing " + name);
	}
	return found->second;
}

Trace ReadTrace(const std::string &path, std::string *backend, std::string *suite) {
	std::ifstream input(path, std::ios::binary);
	if (!input) {
		throw std::runtime_error("cannot read trace " + path);
	}
	Trace trace;
	std::vector<ParsedEvent> pending;
	std::map<std::string, std::uint32_t> occurrences;
	std::string text;
	std::size_t line_number = 0;
	bool header_seen = false;
	while (std::getline(input, text)) {
		++line_number;
		if (!text.empty() && text.back() == '\r') {
			throw std::runtime_error(path + ':' + std::to_string(line_number) +
			                         ": CR is not canonical LF");
		}
		if (text.rfind("trace ", 0) == 0) {
			if (header_seen) {
				throw std::runtime_error(path + ": duplicate trace header");
			}
			const Fields fields = ParseFields(text);
			if (Required(fields, "schema", path, line_number) != kTraceSchema) {
				throw std::runtime_error(path + ": unsupported trace schema");
			}
			*backend = Required(fields, "backend", path, line_number);
			*suite = Required(fields, "suite", path, line_number);
			header_seen = true;
		} else if (text.rfind("event ", 0) == 0) {
			const Fields fields = ParseFields(text);
			pending.push_back({Required(fields, "type", path, line_number),
			                   Required(fields, "address", path, line_number),
			                   Required(fields, "data", path, line_number),
			                   Required(fields, "live_pc", path, line_number),
			                   Required(fields, "public_pc", path, line_number)});
		} else if (text.rfind("checkpoint ", 0) == 0) {
			Fields fields = ParseFields(text);
			const std::string test = Required(fields, "test", path, line_number);
			const std::string reason = Required(fields, "reason", path, line_number);
			if (Required(fields, "normalized", path, line_number) != "1") {
				continue;
			}
			const std::string base = test + '/' + reason;
			const std::uint32_t occurrence = occurrences[base]++;
			const std::string key = base + '/' + std::to_string(occurrence);
			trace.emplace(key, Checkpoint{std::move(fields), std::move(pending)});
			pending.clear();
		} else if (text.rfind("test ", 0) != 0 && !text.empty()) {
			throw std::runtime_error(path + ':' + std::to_string(line_number) +
			                         ": unknown canonical record");
		}
	}
	if (!header_seen) {
		throw std::runtime_error(path + ": missing trace header");
	}
	return trace;
}

bool EventsEqual(const std::vector<ParsedEvent> &left, const std::vector<ParsedEvent> &right) {
	if (left.size() != right.size()) {
		return false;
	}
	for (std::size_t index = 0; index < left.size(); ++index) {
		if (left[index].type != right[index].type || left[index].address != right[index].address ||
		    left[index].data != right[index].data || left[index].live_pc != right[index].live_pc ||
		    left[index].public_pc != right[index].public_pc) {
			return false;
		}
	}
	return true;
}

std::string FormatEvents(const std::vector<ParsedEvent> &events) {
	std::ostringstream stream;
	const std::size_t limit = std::min<std::size_t>(events.size(), 12);
	for (std::size_t index = 0; index < limit; ++index) {
		if (index != 0) {
			stream << ',';
		}
		stream << events[index].type << ':' << events[index].address << ':' << events[index].data
		       << ':' << events[index].live_pc << ':' << events[index].public_pc;
	}
	if (events.size() > limit) {
		stream << ",...(" << events.size() << ')';
	}
	return stream.str();
}

bool IsHaltPcRepresentationDifference(const std::string &key, const std::string &field,
                                      const std::string &legacy, const std::string &modern) {
	if (field != "live_pc" && field != "public_pc") {
		return false;
	}
	const bool halt_fixture = key.rfind("fixture-halt/", 0) == 0;
	const bool halt_state = key.rfind("state-halt-resume/", 0) == 0;
	return (halt_fixture || halt_state) && legacy != modern;
}

const char *AllowedFieldDifference(const std::string &key, const std::string &field,
                                   const std::string &legacy, const std::string &modern) {
	if (IsHaltPcRepresentationDifference(key, field, legacy, modern)) {
		return "vaeg-m38-halt-pc-model";
	}
	if (key == "exchange-af/exec-return/0" && field == "af") {
		return "vaeg-m38-legacy-xf-exchange";
	}
	static const std::set<std::string> block_io = {
	    "io-ini/exec-return/0",  "io-ind/exec-return/0",  "io-inir/exec-return/0",
	    "io-indr/exec-return/0", "io-outi/exec-return/0", "io-outd/exec-return/0",
	    "io-otir/exec-return/0", "io-otdr/exec-return/0"};
	if (block_io.count(key) != 0 && field == "af") {
		return "vaeg-m38-legacy-block-io-flags";
	}
	static const std::set<std::string> block_memory = {
	    "memory-ldi/exec-return/0", "memory-ldd/exec-return/0", "memory-ldir/exec-return/0",
	    "memory-lddr/exec-return/0"};
	if (block_memory.count(key) != 0 && field == "af") {
		return "vaeg-m38-legacy-block-memory-flags";
	}
	static const std::set<std::string> interrupt_r = {
	    "interrupt-ei-following-instruction/completed-interrupt-acceptance/0",
	    "interrupt-im1/exec-return/0",
	    "interrupt-im2-vector-order/exec-return/0",
	    "interrupt-nmi/completed-nmi/0",
	    "interrupt-nmi-priority/nmi-priority/0",
	    "interrupt-nmi-priority/post-nmi-slice/0",
	    "interrupt-persistent-level/initial-acceptance/0",
	    "interrupt-persistent-level/reaccepted-persistent-level/0"};
	if (interrupt_r.count(key) != 0 && field == "r_low") {
		return "vaeg-m38-legacy-interrupt-r";
	}
	if ((key == "interrupt-halt-wake/halt-exit/0" ||
	     key == "state-halt-resume/halt-resumed-slice/0") &&
	    field == "r_low") {
		return "vaeg-m38-legacy-halt-r";
	}
	if (key == "halt-entry/exec-return/0" && (field == "live_pc" || field == "public_pc")) {
		return "vaeg-m38-halt-pc-model";
	}
	if ((key == "state-irq-resume/restored-irq-acceptance/0" ||
	     key == "state-ei-inhibition-boundary/restored-ei-next-slice/0") &&
	    field == "r_low") {
		return "vaeg-m38-legacy-interrupt-r";
	}
	if (key == "interrupt-deasserted-before-acceptance/deasserted-slice/0") {
		static const std::set<std::string> fields = {
		    "sp", "pc", "iff1", "iff2", "live_pc", "public_pc", "memory_hash", "device_hash"};
		if (fields.count(field) != 0) {
			return "vaeg-m38-legacy-ei-fusion-deassert";
		}
	}
	return nullptr;
}

const char *AllowedEventDifference(const std::string &key) {
	if (key == "conditional-jr-not-taken/exec-return/0") {
		return "vaeg-m38-legacy-skipped-branch-operand-read";
	}
	if (key == "interrupt-halt-wake/halt-exit/0") {
		return "vaeg-m38-halt-fetch-model";
	}
	if (key == "state-halt-resume/halt-resumed-slice/0") {
		return "vaeg-m38-halt-fetch-model";
	}
	if (key == "state-ei-inhibition-boundary/restored-ei-next-slice/0") {
		return "vaeg-m38-ei-save-boundary-scheduling";
	}
	if (key == "interrupt-deasserted-before-acceptance/deasserted-slice/0") {
		return "vaeg-m38-legacy-ei-fusion-deassert";
	}
	static const std::set<std::string> stack_order = {
	    "call/exec-return/0",
	    "push-bc/exec-return/0",
	    "interrupt-ei-following-instruction/completed-interrupt-acceptance/0",
	    "interrupt-im0-rst-08/exec-return/0",
	    "interrupt-im0-rst-38/exec-return/0",
	    "interrupt-im1/exec-return/0",
	    "interrupt-im2-vector-order/exec-return/0",
	    "interrupt-nmi/completed-nmi/0",
	    "interrupt-nmi-priority/nmi-priority/0",
	    "interrupt-persistent-level/initial-acceptance/0",
	    "interrupt-persistent-level/reaccepted-persistent-level/0",
	    "interrupt-ei-following-changes-ack/completed-interrupt-acceptance/0"};
	if (key == "state-irq-resume/restored-irq-acceptance/0") {
		return "vaeg-m38-legacy-stack-write-order";
	}
	if (stack_order.count(key) != 0) {
		return "vaeg-m38-legacy-stack-write-order";
	}
	return nullptr;
}

long long SignedField(const Fields &fields, const char *name) {
	return std::stoll(fields.at(name));
}

void WriteCycleReport(const std::string &path, const Trace &legacy, const Trace &modern) {
	if (path.empty()) {
		return;
	}
	std::ofstream output(path, std::ios::binary);
	if (!output) {
		throw std::runtime_error("cannot write cycle report " + path);
	}
	output << "# CPU compatibility cycle deltas\n\n"
	       << "Generated by `vaeg_z80_compat_trace_compare` from canonical M38 "
	          "normalized checkpoints. Clock differences are evidence, not "
	          "an unconditional compatibility failure. A verified "
	          "architectural timing correction may move an otherwise "
	          "identical event to a later `Exec()` slice when eventual "
	          "convergence is proven. Lost, duplicated, reordered, or "
	          "permanently divergent external effects remain failures.\n\n"
	       << "| Scenario/checkpoint | Legacy consumed | New consumed | Delta "
	          "(new-old) | Legacy remain | New remain |\n"
	       << "|---|---:|---:|---:|---:|---:|\n";
	std::map<std::pair<long long, long long>, std::size_t> generated;
	std::size_t differences = 0;
	for (const auto &entry : legacy) {
		const auto found = modern.find(entry.first);
		if (found == modern.end()) {
			continue;
		}
		const long long old_consumed = SignedField(entry.second.fields, "consumed");
		const long long new_consumed = SignedField(found->second.fields, "consumed");
		const long long old_remain = SignedField(entry.second.fields, "remain");
		const long long new_remain = SignedField(found->second.fields, "remain");
		if (old_consumed == new_consumed && old_remain == new_remain) {
			continue;
		}
		++differences;
		if (entry.first.rfind("generated-", 0) == 0) {
			++generated[{new_consumed - old_consumed, new_remain - old_remain}];
			continue;
		}
		output << "| `" << entry.first << "` | " << old_consumed << " | " << new_consumed << " | "
		       << new_consumed - old_consumed << " | " << old_remain << " | " << new_remain
		       << " |\n";
	}
	output << "\n## Generated-corpus groups\n\n"
	       << "| Consumed delta | Remaining delta | Checkpoints |\n"
	       << "|---:|---:|---:|\n";
	for (const auto &entry : generated) {
		output << "| " << entry.first.first << " | " << entry.first.second << " | " << entry.second
		       << " |\n";
	}
	if (generated.empty()) {
		output << "| 0 | 0 | 0 |\n";
	}
	output << "\nTotal normalized checkpoints with a cycle/balance delta: " << differences << ".\n";
}

} // namespace
} // namespace vaeg::z80_compat::differential

int main(int argc, char **argv) {
	using namespace vaeg::z80_compat::differential;
	if (argc < 3) {
		std::cerr << "usage: vaeg_z80_compat_trace_compare REFERENCE REPEAT "
		             "[--cycle-report PATH]\n";
		return 2;
	}
	std::string cycle_report;
	for (int index = 3; index < argc; ++index) {
		const std::string argument(argv[index]);
		if (argument == "--cycle-report" && index + 1 < argc) {
			cycle_report = argv[++index];
		} else {
			std::cerr << "unknown or incomplete option: " << argument << '\n';
			return 2;
		}
	}
	try {
		std::string legacy_backend;
		std::string modern_backend;
		std::string legacy_suite;
		std::string modern_suite;
		const Trace legacy = ReadTrace(argv[1], &legacy_backend, &legacy_suite);
		const Trace modern = ReadTrace(argv[2], &modern_backend, &modern_suite);
		if (legacy_backend != "reference" || modern_backend != "repeat" ||
		    legacy_suite != modern_suite) {
			throw std::runtime_error("trace backend or suite mismatch");
		}
		WriteCycleReport(cycle_report, legacy, modern);

		std::size_t failures = 0;
		std::size_t accepted = 0;
		if (legacy_suite == "convergence") {
			const std::string key = "cycle-fdd-io-eventual-convergence/"
			                        "eventual-convergence/0";
			const auto old = legacy.find(key);
			const auto current = modern.find(key);
			if (old == legacy.end() || current == modern.end()) {
				std::cerr << "DIVERGENCE " << key << ": convergence checkpoint absent\n";
				++failures;
			} else {
				const auto is_io = [](const ParsedEvent &event) {
					return event.type == "io-read" || event.type == "io-write" ||
					       event.type == "interrupt-ack";
				};
				const auto is_expected_out = [](const ParsedEvent &event) {
					return event.type == "io-write" && event.address == "00f4" &&
					       event.data == "5a";
				};
				const auto count = [](const std::vector<ParsedEvent> &events,
				                      const auto &predicate) {
					return static_cast<std::size_t>(
					    std::count_if(events.begin(), events.end(), predicate));
				};
				const std::size_t old_io = count(old->second.events, is_io);
				const std::size_t new_io = count(current->second.events, is_io);
				const std::size_t old_out = count(old->second.events, is_expected_out);
				const std::size_t new_out = count(current->second.events, is_expected_out);
				if (old_io != 1 || new_io != 1 || old_out != 1 || new_out != 1) {
					std::cerr << "DIVERGENCE " << key << ": expected exactly one OUT 00f4:5a; "
					          << "legacy io/out=" << old_io << '/' << old_out
					          << " new io/out=" << new_io << '/' << new_out << '\n';
					++failures;
				} else {
					std::cout << "CONVERGENCE " << key << " exactly-one-out=00f4:5a\n";
				}
			}
		}
		std::set<std::string> keys;
		for (const auto &entry : legacy) {
			keys.insert(entry.first);
		}
		for (const auto &entry : modern) {
			keys.insert(entry.first);
		}
		const std::vector<std::string> compared_fields = {
		    "af",     "bc",      "de",        "hl",   "ix",          "iy",         "sp",
		    "pc",     "af2",     "bc2",       "de2",  "hl2",         "i",          "r_low",
		    "r_bit7", "iff1",    "iff2",      "im",   "halt",        "wait",       "irq",
		    "ei",     "live_pc", "public_pc", "last", "memory_hash", "device_hash"};
		for (const std::string &key : keys) {
			const auto old = legacy.find(key);
			const auto current = modern.find(key);
			if (old == legacy.end() || current == modern.end()) {
				std::cerr << "DIVERGENCE " << key << ": normalized checkpoint absent from "
				          << (old == legacy.end() ? "legacy" : "new") << '\n';
				++failures;
				continue;
			}
			for (const std::string &field : compared_fields) {
				const std::string &old_value = old->second.fields.at(field);
				const std::string &new_value = current->second.fields.at(field);
				if (old_value == new_value) {
					continue;
				}
				const char *allow = AllowedFieldDifference(key, field, old_value, new_value);
				if (allow != nullptr) {
					std::cout << "ACCEPT " << allow << ' ' << key << ' ' << field
					          << " legacy=" << old_value << " new=" << new_value << '\n';
					++accepted;
					continue;
				}
				std::cerr << "DIVERGENCE " << key << " field=" << field << " legacy=" << old_value
				          << " new=" << new_value << '\n';
				++failures;
			}
			if (!EventsEqual(old->second.events, current->second.events)) {
				const char *allow = AllowedEventDifference(key);
				if (allow != nullptr) {
					std::cout << "ACCEPT " << allow << ' ' << key << " events legacy=["
					          << FormatEvents(old->second.events) << "] new=["
					          << FormatEvents(current->second.events) << "]\n";
					++accepted;
				} else {
					std::cerr << "DIVERGENCE " << key << " events legacy=["
					          << FormatEvents(old->second.events) << "] new=["
					          << FormatEvents(current->second.events) << "]\n";
					++failures;
				}
			}
		}
		if (failures != 0) {
			std::cerr << "comparison failed: " << failures
			          << " unresolved normalized divergences\n";
			return 1;
		}
		std::cout << "comparison passed: " << legacy.size() << " normalized checkpoints, "
		          << accepted << " exact allowlist matches, cycle deltas recorded\n";
		return 0;
	} catch (const std::exception &exception) {
		std::cerr << "trace comparison error: " << exception.what() << '\n';
		return 2;
	}
}
