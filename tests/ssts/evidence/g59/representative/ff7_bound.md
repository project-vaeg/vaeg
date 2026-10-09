<!--
Copyright (c) 2026 Nakata Maho

SPDX-License-Identifier: BSD-2-Clause
-->
# M59 representative evidence: ff7_bound

These records are selected deterministically from the complete machine table.
Expected and actual final state are shown independently.

## `000584448ce54b755fc3323e057dc1fad60f8b9535133b68866f848a1cdc8071`

- Form: `FF.7`
- Upstream hash: `d7d93dbbc6f555090586cb7f84e6c7adc1f220b5`
- Instruction: `ff7a57`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `normal-completion`
- Conclusion status: `proven`
- Architectural mismatches: `ram,registers`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "expected_execution": {"interrupt_vector": null, "kind": "normal"}, "failure_partition": "normal-completion"}`
- Initial registers: `{"ax": "f9cb", "bp": "3eab", "bx": "1fc1", "cs": "8db2", "cx": "91e7", "di": "9bff", "ds": "9945", "dx": "1da8", "es": "df12", "flags": "f053", "ip": "82f5", "si": "740d", "sp": "df59", "ss": "f4b3"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "02a87", "value": "cd"}, {"address": "02a88", "value": "0b"}, {"address": "95e15", "value": "ff"}, {"address": "95e16", "value": "7a"}, {"address": "95e17", "value": "57"}, {"address": "ffe3f", "value": "cd"}, {"address": "ffe40", "value": "0b"}], "registers": {"ax": "f9cb", "bp": "3eab", "bx": "1fc1", "cs": "8db2", "cx": "91e7", "di": "9bff", "ds": "9945", "dx": "1da8", "es": "df12", "flags": "f053", "ip": "82f8", "si": "740d", "sp": "df57", "ss": "f4b3"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "02a87", "value": "00"}, {"address": "02a88", "value": "00"}, {"address": "95e15", "value": "ff"}, {"address": "95e16", "value": "7a"}, {"address": "95e17", "value": "57"}, {"address": "ffe3f", "value": "00"}, {"address": "ffe40", "value": "00"}], "registers": {"ax": "f9cb", "bp": "3eab", "bx": "1fc1", "cs": "8db2", "cx": "91e7", "di": "9bff", "ds": "9945", "dx": "1da8", "es": "df12", "flags": "f053", "ip": "82f8", "si": "740d", "sp": "df5b", "ss": "f4b3"}, "termination": "normal"}`

## `001bd4ca1c82cd061dfe8ef5c5bef7e87acaf8803fa6b957db9d52becf66e03c`

- Form: `62`
- Upstream hash: `6e6053553e8e8bfa8b26b6091335d417121c2f88`
- Instruction: `6227`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `range-result-mismatch`
- Conclusion status: `proven`
- Architectural mismatches: `ram,registers`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "77c4"}, {"kind": "expected-frame-byte", "value": "77c5"}, {"kind": "expected-frame-byte", "value": "77c6"}, {"kind": "expected-frame-byte", "value": "77c7"}, {"kind": "expected-frame-byte", "value": "77c8"}, {"kind": "expected-frame-byte", "value": "77c9"}, {"kind": "actual-frame-byte", "value": "77ca"}, {"kind": "actual-frame-byte", "value": "77cb"}, {"kind": "actual-frame-byte", "value": "77cc"}, {"kind": "actual-frame-byte", "value": "77cd"}, {"kind": "actual-frame-byte", "value": "77ce"}, {"kind": "actual-frame-byte", "value": "77cf"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "96064"}, {"kind": "expected-frame-byte", "value": "96065"}, {"kind": "expected-frame-byte", "value": "96066"}, {"kind": "expected-frame-byte", "value": "96067"}, {"kind": "expected-frame-byte", "value": "96068"}, {"kind": "expected-frame-byte", "value": "96069"}, {"kind": "actual-frame-byte", "value": "9606a"}, {"kind": "actual-frame-byte", "value": "9606b"}, {"kind": "actual-frame-byte", "value": "9606c"}, {"kind": "actual-frame-byte", "value": "9606d"}, {"kind": "actual-frame-byte", "value": "9606e"}, {"kind": "actual-frame-byte", "value": "9606f"}]`
- Item analysis: `{"actual_execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "actual_range_result": "normal", "expected_execution": {"interrupt_vector": 5, "kind": "interrupt"}, "expected_range_result": "interrupt-5", "failure_partition": "range-result-mismatch", "interrupt_frame": {"actual_final_flags": "f847", "actual_final_sp": "77ca", "actual_frame_logical_addresses": ["77ca", "77cb", "77cc", "77cd", "77ce", "77cf"], "actual_frame_physical_addresses": ["9606a", "9606b", "9606c", "9606d", "9606e", "9606f"], "actual_saved_cs": null, "actual_saved_flags": null, "actual_saved_ip": null, "expected_final_flags": "f847", "expected_final_sp": "77c4", "expected_frame_logical_addresses": ["77c4", "77c5", "77c6", "77c7", "77c8", "77c9"], "expected_frame_physical_addresses": ["96064", "96065", "96066", "96067", "96068", "96069"], "expected_saved_cs": "677d", "expected_saved_flags": "f847", "expected_saved_ip": "4114", "frame_mapping": "underdetermined", "initial_cs": "677d", "initial_flags": "f847", "initial_ip": "4112", "initial_sp": "77ca", "initial_ss": "8e8a", "physical_boundary_crossed": false, "segment_boundary_crossed": false}}`
- Initial registers: `{"ax": "b14c", "bp": "2ae9", "bx": "29e5", "cs": "677d", "cx": "a035", "di": "6516", "ds": "aa5e", "dx": "fe13", "es": "1756", "flags": "f847", "ip": "4112", "si": "4bcc", "sp": "77ca", "ss": "8e8a"}`
- Expected final: `{"execution": {"interrupt_vector": 5, "kind": "interrupt"}, "ram": [{"address": "00014", "value": "f1"}, {"address": "00015", "value": "ff"}, {"address": "00016", "value": "fb"}, {"address": "00017", "value": "4a"}, {"address": "6b8e2", "value": "62"}, {"address": "6b8e3", "value": "27"}, {"address": "96064", "value": "14"}, {"address": "96065", "value": "41"}, {"address": "96066", "value": "7d"}, {"address": "96067", "value": "67"}, {"address": "96068", "value": "47"}, {"address": "96069", "value": "f8"}, {"address": "acfc5", "value": "62"}, {"address": "acfc6", "value": "24"}, {"address": "acfc7", "value": "be"}, {"address": "acfc8", "value": "ab"}], "registers": {"ax": "b14c", "bp": "2ae9", "bx": "29e5", "cs": "4afb", "cx": "a035", "di": "6516", "ds": "aa5e", "dx": "fe13", "es": "1756", "flags": "f847", "ip": "fff1", "si": "4bcc", "sp": "77c4", "ss": "8e8a"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "00014", "value": "f1"}, {"address": "00015", "value": "ff"}, {"address": "00016", "value": "fb"}, {"address": "00017", "value": "4a"}, {"address": "6b8e2", "value": "62"}, {"address": "6b8e3", "value": "27"}, {"address": "96064", "value": "00"}, {"address": "96065", "value": "00"}, {"address": "96066", "value": "00"}, {"address": "96067", "value": "00"}, {"address": "96068", "value": "00"}, {"address": "96069", "value": "00"}, {"address": "acfc5", "value": "62"}, {"address": "acfc6", "value": "24"}, {"address": "acfc7", "value": "be"}, {"address": "acfc8", "value": "ab"}], "registers": {"ax": "b14c", "bp": "2ae9", "bx": "29e5", "cs": "677d", "cx": "a035", "di": "6516", "ds": "aa5e", "dx": "fe13", "es": "1756", "flags": "f847", "ip": "4114", "si": "4bcc", "sp": "77ca", "ss": "8e8a"}, "termination": "normal"}`

## `001e7a0472dbf6f45af3dfbbae3e1dd499c33fa50f21b768decaba6833ff6743`

- Form: `62`
- Upstream hash: `6381fb33db3f48403ee49a03b0dc2a3cbcf29c4f`
- Instruction: `6238`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `stack-frame-mismatch`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "7770"}, {"kind": "expected-frame-byte", "value": "7771"}, {"kind": "expected-frame-byte", "value": "7772"}, {"kind": "expected-frame-byte", "value": "7773"}, {"kind": "expected-frame-byte", "value": "7774"}, {"kind": "expected-frame-byte", "value": "7775"}, {"kind": "actual-frame-byte", "value": "7770"}, {"kind": "actual-frame-byte", "value": "7771"}, {"kind": "actual-frame-byte", "value": "7772"}, {"kind": "actual-frame-byte", "value": "7773"}, {"kind": "actual-frame-byte", "value": "7774"}, {"kind": "actual-frame-byte", "value": "7775"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "5d880"}, {"kind": "expected-frame-byte", "value": "5d881"}, {"kind": "expected-frame-byte", "value": "5d882"}, {"kind": "expected-frame-byte", "value": "5d883"}, {"kind": "expected-frame-byte", "value": "5d884"}, {"kind": "expected-frame-byte", "value": "5d885"}, {"kind": "actual-frame-byte", "value": "5d880"}, {"kind": "actual-frame-byte", "value": "5d881"}, {"kind": "actual-frame-byte", "value": "5d882"}, {"kind": "actual-frame-byte", "value": "5d883"}, {"kind": "actual-frame-byte", "value": "5d884"}, {"kind": "actual-frame-byte", "value": "5d885"}]`
- Item analysis: `{"actual_execution": {"interrupt_count": 1, "interrupt_vector": 5, "kind": "exception", "termination": "normal"}, "actual_range_result": "interrupt-5", "expected_execution": {"interrupt_vector": 5, "kind": "interrupt"}, "expected_range_result": "interrupt-5", "failure_partition": "stack-frame-mismatch", "interrupt_frame": {"actual_final_flags": "f442", "actual_final_sp": "7770", "actual_frame_logical_addresses": ["7770", "7771", "7772", "7773", "7774", "7775"], "actual_frame_physical_addresses": ["5d880", "5d881", "5d882", "5d883", "5d884", "5d885"], "actual_saved_cs": "af07", "actual_saved_flags": "0442", "actual_saved_ip": "589f", "expected_final_flags": "f442", "expected_final_sp": "7770", "expected_frame_logical_addresses": ["7770", "7771", "7772", "7773", "7774", "7775"], "expected_frame_physical_addresses": ["5d880", "5d881", "5d882", "5d883", "5d884", "5d885"], "expected_saved_cs": "af07", "expected_saved_flags": "f442", "expected_saved_ip": "589f", "frame_mapping": "determined", "initial_cs": "af07", "initial_flags": "f442", "initial_ip": "589d", "initial_sp": "7776", "initial_ss": "5611", "physical_boundary_crossed": false, "segment_boundary_crossed": false}}`
- Initial registers: `{"ax": "dd4b", "bp": "50af", "bx": "7050", "cs": "af07", "cx": "78db", "di": "0f19", "ds": "0abc", "dx": "db39", "es": "d7af", "flags": "f442", "ip": "589d", "si": "e9b5", "sp": "7776", "ss": "5611"}`
- Expected final: `{"execution": {"interrupt_vector": 5, "kind": "interrupt"}, "ram": [{"address": "00014", "value": "ca"}, {"address": "00015", "value": "48"}, {"address": "00016", "value": "11"}, {"address": "00017", "value": "0c"}, {"address": "105c5", "value": "93"}, {"address": "105c6", "value": "a9"}, {"address": "105c7", "value": "52"}, {"address": "105c8", "value": "0e"}, {"address": "5d880", "value": "9f"}, {"address": "5d881", "value": "58"}, {"address": "5d882", "value": "07"}, {"address": "5d883", "value": "af"}, {"address": "5d884", "value": "42"}, {"address": "5d885", "value": "f4"}, {"address": "b490d", "value": "62"}, {"address": "b490e", "value": "38"}], "registers": {"ax": "dd4b", "bp": "50af", "bx": "7050", "cs": "0c11", "cx": "78db", "di": "0f19", "ds": "0abc", "dx": "db39", "es": "d7af", "flags": "f442", "ip": "48ca", "si": "e9b5", "sp": "7770", "ss": "5611"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 5, "kind": "exception", "termination": "normal"}, "ram": [{"address": "00014", "value": "ca"}, {"address": "00015", "value": "48"}, {"address": "00016", "value": "11"}, {"address": "00017", "value": "0c"}, {"address": "105c5", "value": "93"}, {"address": "105c6", "value": "a9"}, {"address": "105c7", "value": "52"}, {"address": "105c8", "value": "0e"}, {"address": "5d880", "value": "9f"}, {"address": "5d881", "value": "58"}, {"address": "5d882", "value": "07"}, {"address": "5d883", "value": "af"}, {"address": "5d884", "value": "42"}, {"address": "5d885", "value": "04"}, {"address": "b490d", "value": "62"}, {"address": "b490e", "value": "38"}], "registers": {"ax": "dd4b", "bp": "50af", "bx": "7050", "cs": "0c11", "cx": "78db", "di": "0f19", "ds": "0abc", "dx": "db39", "es": "d7af", "flags": "f442", "ip": "48ca", "si": "e9b5", "sp": "7770", "ss": "5611"}, "termination": "normal"}`

## `002362fe9f1f6db92f27d302c2a524c600e81a2801c481c280568320375f7155`

- Form: `62`
- Upstream hash: `adb9e07d0d9738520bb4f4bda03cc3e829d0f6b2`
- Instruction: `26624f95`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `range-result-mismatch`
- Conclusion status: `proven`
- Architectural mismatches: `registers`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_execution": {"interrupt_count": 1, "interrupt_vector": 5, "kind": "exception", "termination": "normal"}, "actual_range_result": "interrupt-5", "expected_execution": {"interrupt_vector": null, "kind": "normal"}, "expected_range_result": "normal", "failure_partition": "range-result-mismatch"}`
- Initial registers: `{"ax": "3f26", "bp": "f272", "bx": "7afb", "cs": "5673", "cx": "2ff1", "di": "58fb", "ds": "b6c0", "dx": "aeb1", "es": "2ac0", "flags": "f892", "ip": "dbd9", "si": "9f9d", "sp": "cc2a", "ss": "1d85"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "32690", "value": "21"}, {"address": "32691", "value": "cf"}, {"address": "32692", "value": "62"}, {"address": "32693", "value": "7b"}, {"address": "64309", "value": "26"}, {"address": "6430a", "value": "62"}, {"address": "6430b", "value": "4f"}, {"address": "6430c", "value": "95"}], "registers": {"ax": "3f26", "bp": "f272", "bx": "7afb", "cs": "5673", "cx": "2ff1", "di": "58fb", "ds": "b6c0", "dx": "aeb1", "es": "2ac0", "flags": "f892", "ip": "dbdd", "si": "9f9d", "sp": "cc2a", "ss": "1d85"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 5, "kind": "exception", "termination": "normal"}, "ram": [{"address": "32690", "value": "21"}, {"address": "32691", "value": "cf"}, {"address": "32692", "value": "62"}, {"address": "32693", "value": "7b"}, {"address": "64309", "value": "26"}, {"address": "6430a", "value": "62"}, {"address": "6430b", "value": "4f"}, {"address": "6430c", "value": "95"}], "registers": {"ax": "3f26", "bp": "f272", "bx": "7afb", "cs": "0000", "cx": "2ff1", "di": "58fb", "ds": "b6c0", "dx": "aeb1", "es": "2ac0", "flags": "f892", "ip": "0000", "si": "9f9d", "sp": "cc24", "ss": "1d85"}, "termination": "normal"}`

## `0278f04a8fe35fdace053590f9588b78cd2b016a173fb36c301eede22219f272`

- Form: `62`
- Upstream hash: `1df6f0c07916b521244e90bef1a86934444bead8`
- Instruction: `2e62884a74`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `normal-completion`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "actual_range_result": "normal", "expected_execution": {"interrupt_vector": null, "kind": "normal"}, "expected_range_result": "normal", "failure_partition": "normal-completion"}`
- Initial registers: `{"ax": "b8bf", "bp": "3220", "bx": "dc83", "cs": "75ea", "cx": "d06b", "di": "05cd", "ds": "15dc", "dx": "d443", "es": "259b", "flags": "fcc6", "ip": "446b", "si": "805a", "sp": "c393", "ss": "90cd"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "7a30b", "value": "2e"}, {"address": "7a30c", "value": "62"}, {"address": "7a30d", "value": "88"}, {"address": "7a30e", "value": "4a"}, {"address": "7a30f", "value": "74"}, {"address": "82fc7", "value": "0c"}, {"address": "82fc8", "value": "b8"}, {"address": "82fc9", "value": "98"}, {"address": "82fca", "value": "d7"}], "registers": {"ax": "b8bf", "bp": "3220", "bx": "dc83", "cs": "75ea", "cx": "d06b", "di": "05cd", "ds": "15dc", "dx": "d443", "es": "259b", "flags": "fcc6", "ip": "4470", "si": "805a", "sp": "c393", "ss": "90cd"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "7a30b", "value": "2e"}, {"address": "7a30c", "value": "62"}, {"address": "7a30d", "value": "88"}, {"address": "7a30e", "value": "4a"}, {"address": "7a30f", "value": "74"}, {"address": "82fc7", "value": "0c"}, {"address": "82fc8", "value": "b8"}, {"address": "82fc9", "value": "98"}, {"address": "82fca", "value": "d7"}], "registers": {"ax": "b8bf", "bp": "3220", "bx": "dc83", "cs": "75ea", "cx": "d06b", "di": "05cd", "ds": "15dc", "dx": "d443", "es": "259b", "flags": "fcc6", "ip": "4470", "si": "805a", "sp": "c393", "ss": "90cd"}, "termination": "normal"}`
