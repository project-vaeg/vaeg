#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""Small deterministic model tests for the WD33C93 Transfer Info lifecycle."""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from enum import Enum


class State(Enum):
    IDLE = "idle"
    WAIT_FOR_REQ = "wait_for_req"
    BYTE_PENDING = "transfer_byte_pending"
    WAIT_FOR_POST_COUNT_REQ = "wait_for_post_count_req"
    COMPLETED = "completed_or_terminated"


@dataclass
class Transfer:
    state: State = State.IDLE
    int_pending: bool = False
    lci: bool = False
    bsy: bool = False
    cip: bool = False
    dbr: bool = False
    csr: int | None = None
    req: bool = False
    tc: int = 0
    bytes: int = 0
    req_sequence: int = 0
    ack_count: int = 0
    csr_latch_count: int = 0
    csr_read_count: int = 0
    active_direction: str = "spc-to-host"
    events: list[str] = field(default_factory=list)

    def command_write(self, tc: int, direction: str = "spc-to-host") -> bool:
        self.events.append("command_write")
        if self.int_pending:
            self.lci = True
            self.events.append("command_ignored:int_pending")
            return False
        if self.state in (State.WAIT_FOR_REQ, State.BYTE_PENDING,
                          State.WAIT_FOR_POST_COUNT_REQ):
            self.lci = True
            self.events.append("command_ignored:active")
            return False
        self.cip = True
        self.tc = tc
        self.active_direction = direction
        self.bsy = True
        self.dbr = False
        self.state = State.WAIT_FOR_REQ
        self.cip = False
        self.events.append("command_accepted")
        if self.req:
            self._start_byte()
        return True

    def target_req(self, service: int) -> None:
        self.req_sequence += 1
        self.req = True
        self.events.append(f"req_assert:{self.req_sequence}")
        if self.state == State.WAIT_FOR_REQ:
            self._start_byte()
            return
        if self.state == State.BYTE_PENDING:
            return
        if self.state == State.WAIT_FOR_POST_COUNT_REQ:
            self.req = False
            self.state = State.COMPLETED
            self.bsy = False
            self.dbr = False
            self.csr = service
            self.int_pending = True
            self.csr_latch_count += 1
            self.events.append(f"csr_latch:{service:02x}")
            return
        self.bsy = False
        if self.csr is None and not self.int_pending:
            self.csr = service
            self.int_pending = True
            self.csr_latch_count += 1
            self.events.append(f"csr_latch:{service:02x}")

    def _start_byte(self) -> None:
        self.state = State.BYTE_PENDING
        self.bsy = True
        self.dbr = True
        self.events.append("byte_pending")

    def transfer_byte(self, direction: str | None = None) -> None:
        assert self.state is State.BYTE_PENDING
        assert self.req and self.dbr
        assert direction is None or direction == self.active_direction
        self.req = False
        self.dbr = False
        self.tc -= 1
        self.bytes += 1
        self.ack_count += 1
        self.events.append(f"ack:{self.ack_count}")
        if self.tc:
            self.req_sequence += 1
            self.req = True
            self._start_byte()
            return
        self.state = State.WAIT_FOR_POST_COUNT_REQ
        self.events.append("post_count_req_wait")
        # There is no completion MCI until a distinct post-count REQ arrives.

    def phase_change_before_count_zero(self, status: int) -> None:
        assert self.state is State.BYTE_PENDING
        assert self.tc > 0
        self.req = False
        self.state = State.COMPLETED
        self.bsy = False
        self.dbr = False
        self.csr = status
        self.int_pending = True
        self.csr_latch_count += 1
        self.events.append(f"terminated:{status:02x}")

    def csr_read(self) -> int:
        assert self.csr is not None and self.int_pending
        value = self.csr
        self.csr = None
        self.int_pending = False
        self.csr_read_count += 1
        self.events.append(f"csr_read:{value:02x}")
        return value


def test_transfer_info_waits_for_req() -> None:
    t = Transfer()
    assert t.command_write(1)
    assert t.state is State.WAIT_FOR_REQ
    assert t.bsy and not t.dbr
    t.target_req(0x89)
    assert t.state is State.BYTE_PENDING and t.dbr


def test_transfer_info_accepts_already_asserted_req() -> None:
    t = Transfer(req=True)
    assert t.command_write(1)
    assert t.state is State.BYTE_PENDING
    assert t.bsy and t.dbr and t.req
    t.transfer_byte("spc-to-host")
    assert t.state is State.WAIT_FOR_POST_COUNT_REQ
    assert t.bytes == 1 and t.ack_count == 1


def test_transfer_info_does_not_raise_service_required_while_active() -> None:
    t = Transfer()
    assert t.command_write(1)
    t.target_req(0x89)
    assert t.csr is None and t.int_pending is False
    t.transfer_byte("spc-to-host")
    assert t.state is State.WAIT_FOR_POST_COUNT_REQ
    assert t.csr is None and t.bsy


def test_service_required_requires_no_active_level2_command() -> None:
    t = Transfer()
    t.target_req(0x89)
    assert t.csr == 0x89 and not t.bsy


def test_transfer_info_phase_change_before_tc_zero_returns_4mci() -> None:
    t = Transfer()
    assert t.command_write(4)
    t.target_req(0x89)
    t.phase_change_before_count_zero(0x4B)
    assert t.csr == 0x4B and t.state is State.COMPLETED
    assert t.bytes == 0 and t.ack_count == 0


def test_transfer_info_tc_zero_waits_for_next_req() -> None:
    t = Transfer()
    assert t.command_write(1)
    t.target_req(0x89)
    t.transfer_byte("spc-to-host")
    assert "post_count_req_wait" in t.events
    assert t.state is State.WAIT_FOR_POST_COUNT_REQ
    assert t.csr is None and t.csr_latch_count == 0
    t.target_req(0x1B)
    assert t.state is State.COMPLETED and t.csr == 0x1B


def test_transfer_info_completion_uses_next_req_mci() -> None:
    t = Transfer()
    assert t.command_write(1)
    t.target_req(0x89)
    t.transfer_byte("spc-to-host")
    t.target_req(0x1B)
    assert t.csr == 0x1B
    assert t.req_sequence == 2 and t.ack_count == 1
    assert t.events.index("post_count_req_wait") < t.events.index("csr_latch:1b")


def test_csr_latch_is_stable_while_int_pending() -> None:
    t = Transfer()
    t.target_req(0x89)
    assert t.csr == 0x89
    t.target_req(0x8B)
    assert t.csr == 0x89 and t.csr_latch_count == 1
    assert t.csr_read() == 0x89 and t.csr_read_count == 1


def test_command_during_int_pending_is_ignored() -> None:
    t = Transfer()
    t.target_req(0x89)
    assert not t.command_write(1)
    assert t.lci and t.csr == 0x89 and t.bytes == 0


def test_ignored_command_sets_lci() -> None:
    t = Transfer()
    t.target_req(0x89)
    old_csr = t.csr
    old_latch_count = t.csr_latch_count
    assert not t.command_write(4)
    assert t.lci
    assert t.csr == old_csr
    assert t.csr_latch_count == old_latch_count
    assert not t.bsy and not t.dbr


def run() -> None:
    tests = [name for name in globals() if name.startswith("test_")]
    for name in sorted(tests):
        globals()[name]()
    print(f"M75_TRANSFER_INFO_OK tests={len(tests)}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if not args.selftest:
        parser.error("--selftest is required")
    run()
