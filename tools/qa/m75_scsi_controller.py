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

"""Validate the reviewable M75 SCSI controller contract in the source tree."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


def validate(root: pathlib.Path) -> None:
    scsicmd = (root / "cbus" / "scsicmd.c").read_text(encoding="utf-8")
    scsiio = (root / "cbus" / "scsiio.c").read_text(encoding="utf-8")
    scsiio_h = (root / "cbus" / "scsiio.h").read_text(encoding="utf-8")
    trace = (root / "cpu" / "upd9002" / "upd9002_trace.c").read_text(encoding="utf-8")
    trace_h = (root / "cpu" / "upd9002" / "upd9002_trace.h").read_text(encoding="utf-8")
    cliopts = (root / "sdl2" / "cliopts.c").read_text(encoding="utf-8")
    np2 = (root / "sdl2" / "np2.c").read_text(encoding="utf-8")

    inquiry_match = re.search(
        r"static const BYTE hdd_inquiry\[[^\]]+\]\s*=\s*\{(.*?)\};",
        scsicmd, re.DOTALL)
    if inquiry_match is None:
        raise AssertionError("missing INQUIRY response table")
    inquiry_tokens = re.findall(
        r"0x[0-9a-fA-F]+|'(?:\\.|[^'])'", inquiry_match.group(1))
    inquiry_values = []
    for token in inquiry_tokens:
        if token.startswith("0x"):
            inquiry_values.append(int(token, 16))
        else:
            inquiry_values.append(ord(token[1]))
    if len(inquiry_values) != 36:
        raise AssertionError("INQUIRY response table must contain 36 bytes")
    if len(inquiry_values) < 5:
        raise AssertionError("INQUIRY response table is too short")
    if inquiry_values[4] != len(inquiry_values) - 5:
        raise AssertionError(
            "INQUIRY additional length must equal table length minus five")
    if inquiry_values[4] != 0x1f:
        raise AssertionError("INQUIRY additional length must be 1f")
    if inquiry_values[8:16] != [ord(c) for c in "NEC     "]:
        raise AssertionError("INQUIRY vendor field must be NEC padded to 8 bytes")
    if inquiry_values[16:32] != [ord(c) for c in "NP2-HDD         "]:
        raise AssertionError("INQUIRY product field must be 16 bytes")
    if inquiry_values[32:36] != [ord("1"), ord("."), ord("0"), ord("0")]:
        raise AssertionError("INQUIRY revision must be 1.00")
    require(scsicmd, "hdd_inquiry_unsupported_lun[36]",
            "unsupported-LUN INQUIRY response table")
    if re.search(r"0x7f\s*,\s*0x00\s*,\s*0x02\s*,\s*0x02\s*,\s*0x1f", scsicmd) is None:
        raise AssertionError("missing unsupported-LUN INQUIRY qualifier")
    for helper, label in (("scsicmd_cdb_lun", "CDB LUN extraction"),
                          ("scsicmd_target_lun", "WD Target LUN extraction"),
                          ("scsicmd_lun_supported", "central LUN validation"),
                          ("scsicmd_trace_cdb_result", "CDB result enumeration trace"),
                          ("scsiio_trace_target_selection", "selection enumeration trace"),
                          ("scsicmd_backend_selftest", "compiled LUN/INQUIRY tests")):
        require(scsicmd, helper, label)

    for opcode, name in (("0x00", "TEST UNIT READY"),
                         ("0x03", "REQUEST SENSE"),
                         ("0x12", "INQUIRY"),
                         ("0x25", "READ CAPACITY"),
                         ("0x1a", "MODE SENSE")):
        require(scsicmd, f"case {opcode}", f"{name} CDB")
    for phase in ("SCSIPH_COMMAND", "SCSIPH_DATAIN", "SCSIPH_DATAOUT",
                  "SCSIPH_STATUS", "SCSIPH_MSGIN"):
        require(scsicmd, f"case {phase}", f"{phase} transition")
    for phase, status, direction in (
            ("SCSIPH_DATAOUT", "0x88", "TRUE"),
            ("SCSIPH_DATAIN", "0x89", "FALSE"),
            ("SCSIPH_COMMAND", "0x8a", "TRUE"),
            ("SCSIPH_STATUS", "0x8b", "FALSE"),
            ("SCSIPH_INFOOUT", "0x8c", "TRUE"),
            ("SCSIPH_INFOIN", "0x8d", "FALSE"),
            ("SCSIPH_MSGOUT", "0x8e", "TRUE"),
            ("SCSIPH_MSGIN", "0x8f", "FALSE")):
        require(scsicmd, f"{{{phase}, {status}, {direction}}}",
                f"single-source phase contract {phase}")
    require(scsicmd, "REG8 scsicmd_phase_service_status(UINT phase)",
            "phase service-status lookup")
    require(scsicmd, "BOOL scsicmd_phase_host_to_spc(UINT phase)",
            "phase direction lookup")
    require(scsicmd, "REG8 scsicmd_transinfo(REG8 id)",
            "phase-aware transfer entry")
    require(scsicmd, "The next phase starts a fresh PIO data window.",
            "phase-boundary PIO cursor reset")
    require(scsicmd, "hdd_sense", "REQUEST SENSE response")
    require(scsicmd, "case 0x03", "REQUEST SENSE command execution")
    require(scsicmd, "scsicmd_putbe32", "big-endian response encoding")
    require(scsicmd, "scsicmd_putbe24", "24-bit geometry encoding")
    require(scsicmd, "scsicmd_geometry_valid", "geometry consistency validation")
    require(scsicmd, "(UINT64)sxsi->cylinders * sxsi->surfaces * sxsi->sectors",
            "cylinder/head/sector geometry invariant")
    require(scsicmd, "page = cdb[2] & 0x3f",
            "MODE SENSE page-code decode")
    require(scsicmd, "page != 0x00", "MODE SENSE empty page")
    require(scsicmd, "page != 0x04", "MODE SENSE rigid-disk page")
    require(scsicmd, "page != 0x3f", "MODE SENSE all-pages request")
    require(scsicmd, "page_offset = dbd ? 4 : 12",
            "MODE SENSE DBD layout")
    require(scsicmd, "geometry_offset = page_offset",
            "MODE SENSE page composition")
    require(scsicmd, "scsiio.data[geometry_offset + 1] = 0x16",
            "MODE SENSE page-04 length")
    require(scsicmd, "response_length = geometry_offset +",
            "MODE SENSE response length")
    require(scsicmd, "scsicmd_set_sense(0x05, 0x24, 0x00)",
            "MODE SENSE invalid-field CHECK CONDITION")
    require(scsicmd, "if (!scsicmd_check_condition)",
            "CHECK CONDITION phase selection")
    require(scsiio_h, "UINT cmdpos;", "transfer length state slot")
    require(scsiio_h, "BYTE reserved[2][0x2000];",
            "serialized board-ROM padding")
    require(scsiio, "case SCSICMD_TRANS_INFO:",
            "controller TRANSFER INFO dispatch")
    require(scsiio, "typedef enum {", "explicit Transfer Info state type")
    for state in ("SCSI_TRANSFER_IDLE", "SCSI_TRANSFER_WAIT_FOR_REQ",
                  "SCSI_TRANSFER_BYTE_PENDING",
                  "SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ",
                  "SCSI_TRANSFER_COMPLETED_OR_TERMINATED"):
        require(scsiio, state, f"Transfer Info lifecycle state {state}")
    require(scsiio, "static void scsiio_command_write(REG8 command)",
            "AR18 command acceptance boundary")
    require(scsiio, "command-write-pre", "AR18 pre-state trace")
    require(scsiio, "command-ignored reason=int-pending",
            "INT-pending command rejection")
    require(scsiio, "command-accepted", "accepted Level-II command trace")
    require(scsiio, "scsiio_target_assert_req", "REQ sequence trace")
    require(scsiio, "scsiio_target_negate_req", "target REQ transition")
    require(scsiio, "scsiio_initiator_assert_ack", "initiator ACK assertion")
    require(scsiio, "scsiio_initiator_negate_ack", "initiator ACK negation")
    require(scsiio, "scsiio_complete_byte_handshake", "ACK completion boundary")
    require(scsiio, "post-count-wait", "post-count REQ wait")
    require(scsiio, "WD33C93 exposes Transfer Count as high, middle, low",
            "WD33C93 transfer-count byte order")
    require(scsiio, "scsiio.reg[SCSICTR_TRANSCNT + 0] << 16",
            "transfer-count high-byte decode")
    require(scsiio, "scsiio.reg[SCSICTR_TRANSCNT + 2] = 0xff",
            "transfer-count low-byte decrement")
    require(scsicmd, "scsicmd_putbe24(scsiio.data + 9",
            "MODE SENSE block-length offset")
    require(scsiio, "scsiio.rddatpos >= scsiio.cmdpos",
            "REQ/ACK data completion")
    command_function = scsiio.split("static void scsicmd(REG8 cmd)", 1)[1]
    trans_info = command_function.split("case SCSICMD_TRANS_INFO:", 1)[1]
    trans_info = trans_info.split("\n\t}\n}\n", 1)[0]
    require(command_function.split("case SCSICMD_TRANS_INFO:", 1)[0],
            "scsiio.rddatpos = 0",
            "DATA IN cursor reset on command entry")
    if "scsiio.rddatpos = 0" in trans_info:
        raise AssertionError(
            "DATA IN cursor must survive repeated TRANSFER INFO requests")
    require(scsiio, "static REG8 scsiio_auxstatus(void)",
            "WD33C93 auxiliary-status composition")
    require(scsiio, "static void scsiio_data_write(REG8 dat)",
            "fixed DATA-window write helper")
    require(scsiio, "static REG8 scsiio_data_read(void)",
            "fixed DATA-window read helper")
    data_write = scsiio.split("static void scsiio_data_write(REG8 dat)", 1)[1]
    data_write = data_write.split("\n}\n", 1)[0]
    data_read = scsiio.split("static REG8 scsiio_data_read(void)", 1)[1]
    data_read = data_read.split("\n}\n", 1)[0]
    for helper, body in (("DATA write", data_write), ("DATA read", data_read)):
        if "nevent_set(" in body or "CPU_REMCLOCK" in body:
            raise AssertionError(
                f"{helper} byte pump must remain synchronous to the PIO access")
    require(scsiio, "scsiio.port != SCSICTR_CMD && scsiio.port != SCSICTR_DATA",
            "COMMAND/DATA fixed-window address behavior")
    require(scsiio, "SCSI_AUX_DBR",
            "DBR auxiliary-status bit")
    require(scsiio, "SCSI_AUX_CIP",
            "CIP auxiliary-status bit")
    require(scsiio, "SCSI_AUX_BSY",
            "BSY auxiliary-status bit")
    require(scsiio, "SCSI_C4_DMER",
            "DMER set/reset definition")
    require(scsiio, "hardware-pending",
            "unsupported NEC register warning")
    require(scsiio, "case SCSICTR_DATA:",
            "AR19 fixed DATA window")
    require(scsiio, "case SCSICTR_STATUS:",
            "AR17 status access")
    require(scsiio, "scsiio.port++;\n\t\treturn (scsiio.scsistatus)",
            "AR17 auto-increment to COMMAND")
    require(scsiio, "scsiio.membank & 4",
            "IRE1 IRQ gate")
    require(scsiio, "scsi_command_phase_pending",
            "post-SELECT COMMAND request latch")
    if "scsi_csr_pending" in scsiio:
        raise AssertionError("CSR event pending slot must be removed")
    require(scsiio, "scsi_target_selection_pending",
            "pull-model SELECT target state")
    require(scsiio, "scsi_target_publish",
            "pull-model target event publication")
    require(scsiio, "scsi_target_schedule_after_consume",
            "CSR-consume target-state gate")
    require(scsiio, "scsiintr_enqueue(\"select-command-phase\", status",
            "deferred COMMAND-phase CSR")
    require(scsiio, "scsi_target_selection_status = (ret & 0x80) ? 0x11 : ret",
            "SELECT completion CSR target state")
    require(scsiio, "M75c2 accumulates CDB through DATA",
            "M75c2 Transfer Info boundary")
    require(scsiio, "scsi_transfer_remaining",
            "host-programmed Transfer Count state")
    require(scsiio, "scsi_target_phase_ready",
            "target-side phase readiness gate")
    require(scsiio, "SCSI_TARGET_PROCESSING_CLOCKS",
            "target command processing event quantum")
    require(scsicmd, "REG8 scsicmd_phase_unexpected_status(UINT phase)",
            "short-transfer status encoding")
    require(data_read, "scsiio.rddatpos >= scsiio.cmdpos",
            "short DATA IN transfer detection")
    require(data_read, "scsi_transfer_remaining != 0",
            "short DATA IN residual count detection")
    require(data_read, "scsicmd_phase_unexpected_status(scsiio.phase)",
            "short DATA IN 48h-4Fh status")
    require(data_read, "scsiio.rddatpos >= scsiio.cmdpos",
            "allocation-short DATA IN handling")
    require(data_read, "scsiio_post_count_wait",
            "repeated DATA IN post-count handling")
    require(trans_info, "SCSI_AUX_DBR",
            "DBR held low from TRANSFER INFO until target readiness")
    require(trans_info,
            "nevent_set(NEVENT_SCSIIO, scsi_target_processing_clocks()",
            "emulated-clock delay before the first DBR assertion")
    require(scsiio, "scsiio_trace_jitter",
            "seeded diagnostic timing jitter API")
    require(cliopts, "--scsitrace-jitter-seed",
            "seeded SCSI timing jitter option")
    require(cliopts, "--scsitrace-jitter-span",
            "bounded SCSI timing jitter option")
    require(scsiio, "scsi_trace_data_phase_request_missing_count",
            "DATA IN request boundary counter")
    require(scsiio, "target-phase-delay",
            "target delay watchdog")
    require(scsiio, "scsiio_target_phase_ready_event",
            "target phase readiness event")
    require(scsiio, "scsi_trace_watchdog_schedule",
            "deterministic SCSI watchdog")
    require(scsiio, "NEVENT_SCSIWATCHDOG",
            "dedicated SCSI watchdog event")
    require(scsiio, "upd9002_guest_trace_scsi_status",
            "raw CSR trace notification")
    require(trace_h, "upd9002_guest_trace_start_cmdreq_windows",
            "command-request window trace API")
    require(trace_h, "upd9002_guest_trace_scsi_status",
            "CSR presentation trace API")
    require(trace, "scsi-cmdreq-windows-v1",
            "command-request window trace format")
    require(trace, "scsi-cmdreq-window-summary",
            "command-request window summary")
    require(trace, "presentation_instruction",
            "CSR presentation instruction timestamp")
    require(trace, "presentation_clock",
            "CSR presentation clock timestamp")
    require(trace, "ba_instruction", "047Eh write instruction timestamp")
    require(trace, "raw=8a", "COMMAND request presentation record")
    require(trace, "raw=%02x", "non-COMMAND CSR presentation record")
    require(trace, "0x1cbd", "foreground wait-point watch")
    require(trace, "0x1791", "main-pump exit watch")
    require(cliopts, "--scsitrace-cmdreq-windows",
            "command-request window CLI option")
    require(np2, "upd9002_guest_trace_start_cmdreq_windows",
            "command-request window CLI wiring")
    require(scsiio, "status == 0x85) || (status == 0x80",
            "bus-free status release after MESSAGE IN")
    require(scsiio, "target-phase-wait",
            "DBR-gated target phase wait")
    require(scsiio, "scsi_transfer_phase_pending && !scsi_target_phase_ready",
            "deferred target phase before DBR")
    require(scsiio, "if (scsi_transfer_phase_pending && scsi_target_phase_ready)",
            "CSR release after target readiness")
    require(scsiio, "M75c2 accumulates CDB through DATA",
            "M75c2 CDB accumulation boundary")
    require(scsiio, "scsiio_success_status_from_service",
            "phase-derived successful completion CSR")
    if "scsiio_post_count_wait(0x1a" in scsiio:
        raise AssertionError("post-count completion must not use a hard-coded 1Ah")
    require(scsiio, "scsi_transfer_completion_status",
            "post-count completion status state")
    require(scsiio, "scsitrace csr-%s",
            "CSR provenance trace format")
    require(scsiio, '"request"',
            "CSR request provenance event")
    require(scsiio, '"latch"',
            "CSR latch provenance event")
    require(scsiio, '"hostread"',
            "CSR host-read provenance event")
    require(scsiio, '"overrun"',
            "CSR overrun assertion event")
    require(scsiio, "scsitrace transfer-start",
            "M75c3 transfer phase trace")
    require(scsiio, "scsitrace transfer-result",
            "M75c3 transfer completion trace")
    require(scsiio, "scsitrace data-latched direction=spc-to-host",
            "PIO data-byte trace")
    require(scsiio, "ar19_accesses",
            "M75c3 AR19 access accounting")
    require(scsiio, "ar19_reads",
            "M75c3 AR19 read accounting")
    require(scsiio, "ar19_writes",
            "M75c3 AR19 write accounting")
    require(scsiio, "data_port_accesses",
            "M75c3 legacy data-port accounting")
    require(scsiio, "irq_requests",
            "M75c3 transfer IRQ request accounting")
    require(scsiio, "irq_assertions",
            "M75c3 transfer IRQ assertion accounting")
    require(scsiio, "cdb0",
            "M75c3 CDB opcode capture")
    require(scsiio, "level2-transfer-info",
            "Level-II Transfer Info attribution")
    if "legacy-scsi-phase-engine" in scsiio:
        raise AssertionError("legacy phase-engine ownership must not remain")
    require(scsiio, "SCSI_AUX_LCI | SCSI_AUX_BSY",
            "LCI and BSY auxiliary-status definitions")
    require(scsiio, "SCSI_AUX_PE | SCSI_AUX_DBR",
            "PE and DBR auxiliary-status definitions")
    require(scsiio, "reserved register range",
            "undefined register warning")
    require(scsiio, "case SCSICTR_PKGID:", "AR32 package-id audit")
    require(scsiio, "case SCSICTR_FIFO_CTRL:", "AR34 FIFO audit")
    require(scsiio, "case SCSICTR_FIFO_STATUS:", "AR35 FIFO audit")
    for field in ("scsi_csr_latched", "scsi_csr_event_active",
                  "scsi_target_selection_pending", "scsi_target_phase_delay_pending"):
        require(scsiio, field, f"single-depth CSR target state field {field}")
    if "scsi_csr_pending" in scsiio:
        raise AssertionError("one-entry pending CSR slot must not return")
    require(scsiio, "if (scsi_csr_event_active || scsi_csr_latched)",
            "CSR event admission")
    require(scsiio, "if (scsi_csr_latched)",
            "CSR consume on status read")
    require(scsiio, "scsi_csr_latched = FALSE",
            "CSR latch release")
    require(scsiio, "scsiio.resent = (2 << 3)", "VA IRQ6 default")
    require(scsiio, "iocore_attachinp(0x0cc6, scsiio_icc6)",
            "VA data-port input mapping")
    require(scsiio, "iocore_attachout(0x0cc6, scsiio_occ6)",
            "VA data-port output mapping")
    if "scsiio.bios" in scsiio or "mem + 0xd2000" in scsiio:
        raise AssertionError("SCSI board ROM must remain detached")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path.cwd())
    args = parser.parse_args()
    try:
        validate(args.root.resolve())
    except (AssertionError, OSError) as error:
        print(f"M75_SCSI_CONTROLLER_FAIL: {error}", file=sys.stderr)
        return 1
    print("M75_SCSI_CONTROLLER_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
