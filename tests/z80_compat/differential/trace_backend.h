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

#ifndef TESTS_COMPAT_DIFFERENTIAL_TRACE_BACKEND_H
#define TESTS_COMPAT_DIFFERENTIAL_TRACE_BACKEND_H

#include "trace_common.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace vaeg::z80_compat::differential {

template <typename Cpu> class TraceBackend final : public Backend {
  public:
	static constexpr std::uint32_t kAcknowledgePort = 0x102;

	TraceBackend() : memory_(this), io_(this), clock_{}, counter_{}, cpu_{} {
	}

	bool Initialize(const std::array<std::uint8_t, kStatusSize> &status,
	                const std::vector<Patch> &patches) override {
		memory_.data.fill(0);
		io_.inputs.fill(0xff);
		events_.clear();
		next_event_order_ = 0;
		consumed_clocks_ = 0;
		acknowledge_value_ = 0xff;
		State decoded;
		if (!DecodeState(status, &decoded)) {
			return false;
		}
		clock_.value = static_cast<std::uint32_t>(decoded.lastclock);
		counter_.remain = 0;
		counter_.owner = this;
		for (const Patch &patch : patches) {
			for (std::size_t index = 0; index < patch.bytes.size(); ++index) {
				memory_.data[static_cast<std::uint16_t>(patch.address + index)] =
				    patch.bytes[index];
			}
		}
		if (!cpu_.Init(&memory_, &io_, &clock_, &counter_, static_cast<int>(kAcknowledgePort))) {
			return false;
		}
		return cpu_.LoadStatus(status.data());
	}

	void AdvanceClock(std::uint32_t delta) override {
		clock_.value += delta;
	}

	void Exec() override {
		cpu_.Exec();
	}

	void SetIrq(bool asserted) override {
		cpu_.IRQ(0, asserted ? 1 : 0);
	}

	void SetWait(bool asserted) override {
		cpu_.Wait(asserted);
	}

	void Nmi() override {
		Record("nmi", 0, 0);
		cpu_.NMI();
	}

	bool SaveLoad() override {
		std::array<std::uint8_t, kStatusSize> image{};
		if (cpu_.GetStatusSize() != image.size() || !cpu_.SaveStatus(image.data())) {
			return false;
		}
		Record("state-save", 0, image[59]);
		if (!cpu_.LoadStatus(image.data())) {
			return false;
		}
		Record("state-load", 0, image[59]);
		return true;
	}

	void SetAcknowledge(std::uint8_t value) override {
		acknowledge_value_ = value;
	}

	void SetInput(std::uint8_t port, std::uint8_t value) override {
		io_.inputs[port] = value;
	}

	Snapshot Capture() override {
		std::array<std::uint8_t, kStatusSize> image{};
		Snapshot snapshot;
		if (!cpu_.SaveStatus(image.data()) || !DecodeState(image, &snapshot.state)) {
			snapshot.state = State{};
		}
		snapshot.live_pc = static_cast<std::uint16_t>(cpu_.GetPC());
		snapshot.public_pc = static_cast<std::uint16_t>(cpu_.GetReg()->pc);
		snapshot.consumed_clocks = consumed_clocks_;
		snapshot.full_memory_hash = HashMemory(memory_.data, 0, memory_.data.size());
		const std::uint64_t device = HashMemory(memory_.data, 0x2000, 0x2100);
		const std::uint64_t stack = HashMemory(memory_.data, 0xef00, 0xf100);
		snapshot.device_memory_hash = device ^ ((stack << 1) | (stack >> 63));
		snapshot.events = std::move(events_);
		events_.clear();
		return snapshot;
	}

  private:
	class Memory final : public IMemoryAccess {
	  public:
		explicit Memory(TraceBackend *owner) : owner_(owner) {
		}

		std::uint32_t IFCALL Read8(std::uint32_t address) override {
			const std::uint16_t wrapped = static_cast<std::uint16_t>(address);
			const std::uint8_t value = data[wrapped];
			owner_->Record("memory-read", wrapped, value);
			return value;
		}

		void IFCALL Write8(std::uint32_t address, std::uint32_t value) override {
			const std::uint16_t wrapped = static_cast<std::uint16_t>(address);
			const std::uint8_t byte = static_cast<std::uint8_t>(value);
			data[wrapped] = byte;
			owner_->Record("memory-write", wrapped, byte);
		}

		std::array<std::uint8_t, kMemorySize> data{};

	  private:
		TraceBackend *owner_;
	};

	class Io final : public IIOAccess {
	  public:
		explicit Io(TraceBackend *owner) : owner_(owner) {
		}

		std::uint32_t IFCALL In(std::uint32_t port) override {
			if (port == kAcknowledgePort) {
				owner_->Record("interrupt-ack", 0, owner_->acknowledge_value_);
				return owner_->acknowledge_value_;
			}
			const std::uint8_t masked = static_cast<std::uint8_t>(port);
			const std::uint8_t value = inputs[masked];
			owner_->Record("io-read", masked, value);
			return value;
		}

		void IFCALL Out(std::uint32_t port, std::uint32_t value) override {
			const std::uint8_t masked = static_cast<std::uint8_t>(port);
			const std::uint8_t byte = static_cast<std::uint8_t>(value);
			owner_->Record("io-write", masked, byte);
			if (masked == 0xf0) {
				owner_->acknowledge_value_ = byte;
			}
		}

		std::array<std::uint8_t, 256> inputs{};

	  private:
		TraceBackend *owner_;
	};

	class Clock final : public IClock {
	  public:
		std::uint32_t IFCALL now() override {
			return value;
		}
		std::uint32_t value = 0;
	};

	class Counter final : public IClockCounter {
	  public:
		void IFCALL past(std::int32_t clocks) override {
			remain -= clocks;
			owner->consumed_clocks_ += clocks;
		}
		std::int32_t IFCALL GetRemainclock() override {
			return remain;
		}
		void IFCALL SetRemainclock(std::int32_t value) override {
			remain = value;
		}
		TraceBackend *owner = nullptr;
		std::int32_t remain = 0;
	};

	void Record(const char *type, std::uint16_t address, std::uint8_t data) {
		const bool io_context = type[0] == 'i' && type[1] != '\0';
		const std::uint16_t live_pc = io_context ? static_cast<std::uint16_t>(cpu_.GetPC()) : 0;
		const std::uint16_t public_pc =
		    io_context ? static_cast<std::uint16_t>(cpu_.GetReg()->pc) : 0;
		events_.push_back(
		    {next_event_order_++, type, address, data, io_context, live_pc, public_pc});
	}

	Memory memory_;
	Io io_;
	Clock clock_;
	Counter counter_;
	Cpu cpu_;
	std::uint8_t acknowledge_value_ = 0xff;
	std::int64_t consumed_clocks_ = 0;
	std::uint32_t next_event_order_ = 0;
	std::vector<Event> events_;
};

} // namespace vaeg::z80_compat::differential

#endif
