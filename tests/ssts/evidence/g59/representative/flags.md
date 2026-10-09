<!--
Copyright (c) 2026 Nakata Maho

SPDX-License-Identifier: BSD-2-Clause
-->
# M59 representative evidence: flags

These records are selected deterministically from the complete machine table.
Expected and actual final state are shown independently.

## `0000c3973227bb631ff5eb928614199f783d134bdd810d01c142e6f9ce544b4b`

- Form: `CC`
- Upstream hash: `08d2830e2dc890c4fff1d3e768b2413933a404dd`
- Instruction: `cc`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `no-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "7e48"}, {"kind": "expected-frame-byte", "value": "7e49"}, {"kind": "expected-frame-byte", "value": "7e4a"}, {"kind": "expected-frame-byte", "value": "7e4b"}, {"kind": "expected-frame-byte", "value": "7e4c"}, {"kind": "expected-frame-byte", "value": "7e4d"}, {"kind": "actual-frame-byte", "value": "7e48"}, {"kind": "actual-frame-byte", "value": "7e49"}, {"kind": "actual-frame-byte", "value": "7e4a"}, {"kind": "actual-frame-byte", "value": "7e4b"}, {"kind": "actual-frame-byte", "value": "7e4c"}, {"kind": "actual-frame-byte", "value": "7e4d"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "66338"}, {"kind": "expected-frame-byte", "value": "66339"}, {"kind": "expected-frame-byte", "value": "6633a"}, {"kind": "expected-frame-byte", "value": "6633b"}, {"kind": "expected-frame-byte", "value": "6633c"}, {"kind": "expected-frame-byte", "value": "6633d"}, {"kind": "actual-frame-byte", "value": "66338"}, {"kind": "actual-frame-byte", "value": "66339"}, {"kind": "actual-frame-byte", "value": "6633a"}, {"kind": "actual-frame-byte", "value": "6633b"}, {"kind": "actual-frame-byte", "value": "6633c"}, {"kind": "actual-frame-byte", "value": "6633d"}]`
- Item analysis: `{"actual_final_flags": "f853", "actual_final_sp": "7e48", "actual_frame_logical_addresses": ["7e48", "7e49", "7e4a", "7e4b", "7e4c", "7e4d"], "actual_frame_physical_addresses": ["66338", "66339", "6633a", "6633b", "6633c", "6633d"], "actual_saved_cs": "75c4", "actual_saved_flags": "0853", "actual_saved_ip": "07ce", "expected_final_flags": "f853", "expected_final_sp": "7e48", "expected_frame_logical_addresses": ["7e48", "7e49", "7e4a", "7e4b", "7e4c", "7e4d"], "expected_frame_physical_addresses": ["66338", "66339", "6633a", "6633b", "6633c", "6633d"], "expected_saved_cs": "75c4", "expected_saved_flags": "f853", "expected_saved_ip": "07ce", "frame_mapping": "determined", "initial_cs": "75c4", "initial_flags": "f853", "initial_ip": "07cd", "initial_sp": "7e4e", "initial_ss": "5e4f", "physical_boundary_crossed": false, "segment_boundary_crossed": false}`
- Initial registers: `{"ax": "bac7", "bp": "77ce", "bx": "7c18", "cs": "75c4", "cx": "7340", "di": "ec03", "ds": "1bd5", "dx": "a0f8", "es": "70e6", "flags": "f853", "ip": "07cd", "si": "3662", "sp": "7e4e", "ss": "5e4f"}`
- Expected final: `{"execution": {"interrupt_vector": 3, "kind": "interrupt"}, "ram": [{"address": "0000c", "value": "e1"}, {"address": "0000d", "value": "ac"}, {"address": "0000e", "value": "59"}, {"address": "0000f", "value": "ec"}, {"address": "66338", "value": "ce"}, {"address": "66339", "value": "07"}, {"address": "6633a", "value": "c4"}, {"address": "6633b", "value": "75"}, {"address": "6633c", "value": "53"}, {"address": "6633d", "value": "f8"}, {"address": "7640d", "value": "cc"}], "registers": {"ax": "bac7", "bp": "77ce", "bx": "7c18", "cs": "ec59", "cx": "7340", "di": "ec03", "ds": "1bd5", "dx": "a0f8", "es": "70e6", "flags": "f853", "ip": "ace1", "si": "3662", "sp": "7e48", "ss": "5e4f"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 3, "kind": "software_interrupt", "termination": "normal"}, "ram": [{"address": "0000c", "value": "e1"}, {"address": "0000d", "value": "ac"}, {"address": "0000e", "value": "59"}, {"address": "0000f", "value": "ec"}, {"address": "66338", "value": "ce"}, {"address": "66339", "value": "07"}, {"address": "6633a", "value": "c4"}, {"address": "6633b", "value": "75"}, {"address": "6633c", "value": "53"}, {"address": "6633d", "value": "08"}, {"address": "7640d", "value": "cc"}], "registers": {"ax": "bac7", "bp": "77ce", "bx": "7c18", "cs": "ec59", "cx": "7340", "di": "ec03", "ds": "1bd5", "dx": "a0f8", "es": "70e6", "flags": "f853", "ip": "ace1", "si": "3662", "sp": "7e48", "ss": "5e4f"}, "termination": "normal"}`

## `00011ca3bb06b5828cedf90e08e9190b67e822dd11f99878a46ff3b712693640`

- Form: `9E`
- Upstream hash: `8b9064df15b57998ef2fd753872d2038afa0c34c`
- Instruction: `9e`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `sahf-load`
- Conclusion status: `proven`
- Architectural mismatches: `registers`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_final_flags": "fcbb", "expected_final_flags": "fc93", "initial_ah": "bb", "initial_flags": "fc82"}`
- Initial registers: `{"ax": "bbcf", "bp": "0e63", "bx": "b3c6", "cs": "00b1", "cx": "c053", "di": "a041", "ds": "1de4", "dx": "7174", "es": "3cff", "flags": "fc82", "ip": "b042", "si": "6065", "sp": "4832", "ss": "3130"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "0bb52", "value": "9e"}], "registers": {"ax": "bbcf", "bp": "0e63", "bx": "b3c6", "cs": "00b1", "cx": "c053", "di": "a041", "ds": "1de4", "dx": "7174", "es": "3cff", "flags": "fc93", "ip": "b043", "si": "6065", "sp": "4832", "ss": "3130"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "0bb52", "value": "9e"}], "registers": {"ax": "bbcf", "bp": "0e63", "bx": "b3c6", "cs": "00b1", "cx": "c053", "di": "a041", "ds": "1de4", "dx": "7174", "es": "3cff", "flags": "fcbb", "ip": "b043", "si": "6065", "sp": "4832", "ss": "3130"}, "termination": "normal"}`

## `00015a81f6528aa77b401454d35d1bac69d0e791770b2feb7a581f764dd1f63f`

- Form: `CE`
- Upstream hash: `3944d06a6a57fa6bfb354890619beabe7dfa87c9`
- Instruction: `ce`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `no-interrupt-expected`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_final_flags": "f4d7", "expected_final_flags": "f4d7", "initial_flags": "f4d7"}`
- Initial registers: `{"ax": "d33c", "bp": "dd1d", "bx": "a38d", "cs": "15c3", "cx": "879d", "di": "abfb", "ds": "1fa9", "dx": "1b86", "es": "c795", "flags": "f4d7", "ip": "9f22", "si": "77fc", "sp": "efeb", "ss": "c4d1"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "1fb52", "value": "ce"}], "registers": {"ax": "d33c", "bp": "dd1d", "bx": "a38d", "cs": "15c3", "cx": "879d", "di": "abfb", "ds": "1fa9", "dx": "1b86", "es": "c795", "flags": "f4d7", "ip": "9f23", "si": "77fc", "sp": "efeb", "ss": "c4d1"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "1fb52", "value": "ce"}], "registers": {"ax": "d33c", "bp": "dd1d", "bx": "a38d", "cs": "15c3", "cx": "879d", "di": "abfb", "ds": "1fa9", "dx": "1b86", "es": "c795", "flags": "f4d7", "ip": "9f23", "si": "77fc", "sp": "efeb", "ss": "c4d1"}, "termination": "normal"}`

## `00058e8d5153ad9cd3e08f5b8539ca66bfbd22cb5c50bc9eb2f1960a8118cc81`

- Form: `9D`
- Upstream hash: `ae4d980f1e3c891987c77c9a13b8ec17bb021767`
- Instruction: `9d`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `popf-load`
- Conclusion status: `proven`
- Architectural mismatches: `registers`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_final_flags": "fa5a", "expected_final_flags": "fa52", "initial_flags": "f416", "stack_input_flags": "aa58"}`
- Initial registers: `{"ax": "269a", "bp": "5d1e", "bx": "3b4d", "cs": "b074", "cx": "11f4", "di": "2987", "ds": "aed5", "dx": "ff05", "es": "3b64", "flags": "f416", "ip": "ecc4", "si": "04d6", "sp": "2de6", "ss": "0efa"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "11d86", "value": "58"}, {"address": "11d87", "value": "aa"}, {"address": "bf404", "value": "9d"}], "registers": {"ax": "269a", "bp": "5d1e", "bx": "3b4d", "cs": "b074", "cx": "11f4", "di": "2987", "ds": "aed5", "dx": "ff05", "es": "3b64", "flags": "fa52", "ip": "ecc5", "si": "04d6", "sp": "2de8", "ss": "0efa"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "11d86", "value": "58"}, {"address": "11d87", "value": "aa"}, {"address": "bf404", "value": "9d"}], "registers": {"ax": "269a", "bp": "5d1e", "bx": "3b4d", "cs": "b074", "cx": "11f4", "di": "2987", "ds": "aed5", "dx": "ff05", "es": "3b64", "flags": "fa5a", "ip": "ecc5", "si": "04d6", "sp": "2de8", "ss": "0efa"}, "termination": "normal"}`

## `0007730448a5f7d447544bf480045e2c9924216ae2479870c807863c04c5d2e9`

- Form: `9F`
- Upstream hash: `6831ea42ebad62f3c4337ab6328ff3d01c17fe1d`
- Instruction: `9f`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `lahf-image`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_ah": "c3", "expected_ah": "c3", "initial_flags": "f8c3"}`
- Initial registers: `{"ax": "81ca", "bp": "a125", "bx": "2558", "cs": "d7d6", "cx": "5521", "di": "b23c", "ds": "a48d", "dx": "c4d3", "es": "1399", "flags": "f8c3", "ip": "cff3", "si": "eed5", "sp": "200c", "ss": "fd6a"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "e4d53", "value": "9f"}], "registers": {"ax": "c3ca", "bp": "a125", "bx": "2558", "cs": "d7d6", "cx": "5521", "di": "b23c", "ds": "a48d", "dx": "c4d3", "es": "1399", "flags": "f8c3", "ip": "cff4", "si": "eed5", "sp": "200c", "ss": "fd6a"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "e4d53", "value": "9f"}], "registers": {"ax": "c3ca", "bp": "a125", "bx": "2558", "cs": "d7d6", "cx": "5521", "di": "b23c", "ds": "a48d", "dx": "c4d3", "es": "1399", "flags": "f8c3", "ip": "cff4", "si": "eed5", "sp": "200c", "ss": "fd6a"}, "termination": "normal"}`

## `0007a4eef94283423280cdbad4e6e839560c669d9a71edec8aa905ff6b8b448a`

- Form: `CE`
- Upstream hash: `2cea6a0413d131a54782e0cf25e5e8b0e8efa449`
- Instruction: `ce`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `no-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "1802"}, {"kind": "expected-frame-byte", "value": "1803"}, {"kind": "expected-frame-byte", "value": "1804"}, {"kind": "expected-frame-byte", "value": "1805"}, {"kind": "expected-frame-byte", "value": "1806"}, {"kind": "expected-frame-byte", "value": "1807"}, {"kind": "actual-frame-byte", "value": "1802"}, {"kind": "actual-frame-byte", "value": "1803"}, {"kind": "actual-frame-byte", "value": "1804"}, {"kind": "actual-frame-byte", "value": "1805"}, {"kind": "actual-frame-byte", "value": "1806"}, {"kind": "actual-frame-byte", "value": "1807"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "dab62"}, {"kind": "expected-frame-byte", "value": "dab63"}, {"kind": "expected-frame-byte", "value": "dab64"}, {"kind": "expected-frame-byte", "value": "dab65"}, {"kind": "expected-frame-byte", "value": "dab66"}, {"kind": "expected-frame-byte", "value": "dab67"}, {"kind": "actual-frame-byte", "value": "dab62"}, {"kind": "actual-frame-byte", "value": "dab63"}, {"kind": "actual-frame-byte", "value": "dab64"}, {"kind": "actual-frame-byte", "value": "dab65"}, {"kind": "actual-frame-byte", "value": "dab66"}, {"kind": "actual-frame-byte", "value": "dab67"}]`
- Item analysis: `{"actual_final_flags": "fc97", "actual_final_sp": "1802", "actual_frame_logical_addresses": ["1802", "1803", "1804", "1805", "1806", "1807"], "actual_frame_physical_addresses": ["dab62", "dab63", "dab64", "dab65", "dab66", "dab67"], "actual_saved_cs": "f957", "actual_saved_flags": "0c97", "actual_saved_ip": "224d", "expected_final_flags": "fc97", "expected_final_sp": "1802", "expected_frame_logical_addresses": ["1802", "1803", "1804", "1805", "1806", "1807"], "expected_frame_physical_addresses": ["dab62", "dab63", "dab64", "dab65", "dab66", "dab67"], "expected_saved_cs": "f957", "expected_saved_flags": "fc97", "expected_saved_ip": "224d", "frame_mapping": "determined", "initial_cs": "f957", "initial_flags": "fc97", "initial_ip": "224c", "initial_sp": "1808", "initial_ss": "d936", "physical_boundary_crossed": false, "segment_boundary_crossed": false}`
- Initial registers: `{"ax": "e71f", "bp": "3c8b", "bx": "d4b7", "cs": "f957", "cx": "635d", "di": "3705", "ds": "16c2", "dx": "0353", "es": "9c87", "flags": "fc97", "ip": "224c", "si": "1c09", "sp": "1808", "ss": "d936"}`
- Expected final: `{"execution": {"interrupt_vector": 4, "kind": "interrupt"}, "ram": [{"address": "00010", "value": "7c"}, {"address": "00011", "value": "7a"}, {"address": "00012", "value": "df"}, {"address": "00013", "value": "f2"}, {"address": "dab62", "value": "4d"}, {"address": "dab63", "value": "22"}, {"address": "dab64", "value": "57"}, {"address": "dab65", "value": "f9"}, {"address": "dab66", "value": "97"}, {"address": "dab67", "value": "fc"}, {"address": "fb7bc", "value": "ce"}], "registers": {"ax": "e71f", "bp": "3c8b", "bx": "d4b7", "cs": "f2df", "cx": "635d", "di": "3705", "ds": "16c2", "dx": "0353", "es": "9c87", "flags": "fc97", "ip": "7a7c", "si": "1c09", "sp": "1802", "ss": "d936"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 4, "kind": "software_interrupt", "termination": "normal"}, "ram": [{"address": "00010", "value": "7c"}, {"address": "00011", "value": "7a"}, {"address": "00012", "value": "df"}, {"address": "00013", "value": "f2"}, {"address": "dab62", "value": "4d"}, {"address": "dab63", "value": "22"}, {"address": "dab64", "value": "57"}, {"address": "dab65", "value": "f9"}, {"address": "dab66", "value": "97"}, {"address": "dab67", "value": "0c"}, {"address": "fb7bc", "value": "ce"}], "registers": {"ax": "e71f", "bp": "3c8b", "bx": "d4b7", "cs": "f2df", "cx": "635d", "di": "3705", "ds": "16c2", "dx": "0353", "es": "9c87", "flags": "fc97", "ip": "7a7c", "si": "1c09", "sp": "1802", "ss": "d936"}, "termination": "normal"}`

## `000dfea7a111adba3e5a13fd92a6182223b6df015f47572636ed13b2fb84bcba`

- Form: `CD`
- Upstream hash: `99f9273c04be99d046e6684a70c26746c9efd0ef`
- Instruction: `cd96`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `no-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "b0ad"}, {"kind": "expected-frame-byte", "value": "b0ae"}, {"kind": "expected-frame-byte", "value": "b0af"}, {"kind": "expected-frame-byte", "value": "b0b0"}, {"kind": "expected-frame-byte", "value": "b0b1"}, {"kind": "expected-frame-byte", "value": "b0b2"}, {"kind": "actual-frame-byte", "value": "b0ad"}, {"kind": "actual-frame-byte", "value": "b0ae"}, {"kind": "actual-frame-byte", "value": "b0af"}, {"kind": "actual-frame-byte", "value": "b0b0"}, {"kind": "actual-frame-byte", "value": "b0b1"}, {"kind": "actual-frame-byte", "value": "b0b2"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "7aced"}, {"kind": "expected-frame-byte", "value": "7acee"}, {"kind": "expected-frame-byte", "value": "7acef"}, {"kind": "expected-frame-byte", "value": "7acf0"}, {"kind": "expected-frame-byte", "value": "7acf1"}, {"kind": "expected-frame-byte", "value": "7acf2"}, {"kind": "actual-frame-byte", "value": "7aced"}, {"kind": "actual-frame-byte", "value": "7acee"}, {"kind": "actual-frame-byte", "value": "7acef"}, {"kind": "actual-frame-byte", "value": "7acf0"}, {"kind": "actual-frame-byte", "value": "7acf1"}, {"kind": "actual-frame-byte", "value": "7acf2"}]`
- Item analysis: `{"actual_final_flags": "f482", "actual_final_sp": "b0ad", "actual_frame_logical_addresses": ["b0ad", "b0ae", "b0af", "b0b0", "b0b1", "b0b2"], "actual_frame_physical_addresses": ["7aced", "7acee", "7acef", "7acf0", "7acf1", "7acf2"], "actual_saved_cs": "1140", "actual_saved_flags": "0482", "actual_saved_ip": "da1c", "expected_final_flags": "f482", "expected_final_sp": "b0ad", "expected_frame_logical_addresses": ["b0ad", "b0ae", "b0af", "b0b0", "b0b1", "b0b2"], "expected_frame_physical_addresses": ["7aced", "7acee", "7acef", "7acf0", "7acf1", "7acf2"], "expected_saved_cs": "1140", "expected_saved_flags": "f482", "expected_saved_ip": "da1c", "frame_mapping": "determined", "initial_cs": "1140", "initial_flags": "f482", "initial_ip": "da1a", "initial_sp": "b0b3", "initial_ss": "6fc4", "physical_boundary_crossed": false, "segment_boundary_crossed": false}`
- Initial registers: `{"ax": "e4fd", "bp": "1a0d", "bx": "2acb", "cs": "1140", "cx": "359f", "di": "1f5f", "ds": "a433", "dx": "8b6c", "es": "1245", "flags": "f482", "ip": "da1a", "si": "9801", "sp": "b0b3", "ss": "6fc4"}`
- Expected final: `{"execution": {"interrupt_vector": 150, "kind": "interrupt"}, "ram": [{"address": "00258", "value": "66"}, {"address": "00259", "value": "75"}, {"address": "0025a", "value": "ba"}, {"address": "0025b", "value": "19"}, {"address": "1ee1a", "value": "cd"}, {"address": "1ee1b", "value": "96"}, {"address": "7aced", "value": "1c"}, {"address": "7acee", "value": "da"}, {"address": "7acef", "value": "40"}, {"address": "7acf0", "value": "11"}, {"address": "7acf1", "value": "82"}, {"address": "7acf2", "value": "f4"}], "registers": {"ax": "e4fd", "bp": "1a0d", "bx": "2acb", "cs": "19ba", "cx": "359f", "di": "1f5f", "ds": "a433", "dx": "8b6c", "es": "1245", "flags": "f482", "ip": "7566", "si": "9801", "sp": "b0ad", "ss": "6fc4"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 150, "kind": "software_interrupt", "termination": "normal"}, "ram": [{"address": "00258", "value": "66"}, {"address": "00259", "value": "75"}, {"address": "0025a", "value": "ba"}, {"address": "0025b", "value": "19"}, {"address": "1ee1a", "value": "cd"}, {"address": "1ee1b", "value": "96"}, {"address": "7aced", "value": "1c"}, {"address": "7acee", "value": "da"}, {"address": "7acef", "value": "40"}, {"address": "7acf0", "value": "11"}, {"address": "7acf1", "value": "82"}, {"address": "7acf2", "value": "04"}], "registers": {"ax": "e4fd", "bp": "1a0d", "bx": "2acb", "cs": "19ba", "cx": "359f", "di": "1f5f", "ds": "a433", "dx": "8b6c", "es": "1245", "flags": "f482", "ip": "7566", "si": "9801", "sp": "b0ad", "ss": "6fc4"}, "termination": "normal"}`

## `0011cd2cfaf2f1fcd8c58e112c8d2fa321cd57d23694c93c75dd2b6c09615874`

- Form: `9C`
- Upstream hash: `0affcaabcf5525ab930a7e763df9094ff33acc05`
- Instruction: `9c`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `pushf-no-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[{"kind": "expected-stack-byte", "value": "6af2"}, {"kind": "expected-stack-byte", "value": "6af3"}, {"kind": "actual-stack-byte", "value": "6af2"}, {"kind": "actual-stack-byte", "value": "6af3"}]`
- Physical addresses: `[{"kind": "expected-stack-byte", "value": "0a852"}, {"kind": "expected-stack-byte", "value": "0a853"}, {"kind": "actual-stack-byte", "value": "0a852"}, {"kind": "actual-stack-byte", "value": "0a853"}]`
- Item analysis: `{"actual_pushed_flags": "f4d7", "actual_stack_logical_addresses": ["6af2", "6af3"], "actual_stack_physical_addresses": ["0a852", "0a853"], "expected_pushed_flags": "f4d7", "expected_stack_logical_addresses": ["6af2", "6af3"], "expected_stack_physical_addresses": ["0a852", "0a853"], "initial_flags": "f4d7", "physical_boundary_crossed": false, "segment_boundary_crossed": false}`
- Initial registers: `{"ax": "48ed", "bp": "69bf", "bx": "6f0d", "cs": "e378", "cx": "0c9f", "di": "842f", "ds": "6978", "dx": "fd77", "es": "26a9", "flags": "f4d7", "ip": "d1ba", "si": "7bd4", "sp": "6af4", "ss": "03d6"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "0a852", "value": "d7"}, {"address": "0a853", "value": "f4"}, {"address": "f093a", "value": "9c"}], "registers": {"ax": "48ed", "bp": "69bf", "bx": "6f0d", "cs": "e378", "cx": "0c9f", "di": "842f", "ds": "6978", "dx": "fd77", "es": "26a9", "flags": "f4d7", "ip": "d1bb", "si": "7bd4", "sp": "6af2", "ss": "03d6"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "0a852", "value": "d7"}, {"address": "0a853", "value": "f4"}, {"address": "f093a", "value": "9c"}], "registers": {"ax": "48ed", "bp": "69bf", "bx": "6f0d", "cs": "e378", "cx": "0c9f", "di": "842f", "ds": "6978", "dx": "fd77", "es": "26a9", "flags": "f4d7", "ip": "d1bb", "si": "7bd4", "sp": "6af2", "ss": "03d6"}, "termination": "normal"}`

## `003fb963282fd821694d19d8ba16fc77f4948b6348fd2ab45993a768f2a9bd7a`

- Form: `9D`
- Upstream hash: `abca712f9564b106cd33f9767630880f1dbc1a0b`
- Instruction: `9d`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `popf-load`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_final_flags": "f4d6", "expected_final_flags": "f4d6", "initial_flags": "f097", "stack_input_flags": "74d4"}`
- Initial registers: `{"ax": "c747", "bp": "f291", "bx": "8057", "cs": "d740", "cx": "26b8", "di": "301c", "ds": "93e9", "dx": "740b", "es": "092a", "flags": "f097", "ip": "3fc6", "si": "32ed", "sp": "5869", "ss": "401f"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "45a59", "value": "d4"}, {"address": "45a5a", "value": "74"}, {"address": "db3c6", "value": "9d"}], "registers": {"ax": "c747", "bp": "f291", "bx": "8057", "cs": "d740", "cx": "26b8", "di": "301c", "ds": "93e9", "dx": "740b", "es": "092a", "flags": "f4d6", "ip": "3fc7", "si": "32ed", "sp": "586b", "ss": "401f"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "45a59", "value": "d4"}, {"address": "45a5a", "value": "74"}, {"address": "db3c6", "value": "9d"}], "registers": {"ax": "c747", "bp": "f291", "bx": "8057", "cs": "d740", "cx": "26b8", "di": "301c", "ds": "93e9", "dx": "740b", "es": "092a", "flags": "f4d6", "ip": "3fc7", "si": "32ed", "sp": "586b", "ss": "401f"}, "termination": "normal"}`

## `0040ac750a492c90012a5dfa4b33ec9708b97e74e1c2a8b8c0bfe58f22cfdf1a`

- Form: `9E`
- Upstream hash: `dae014c7f1ae2ab5343d8f0391badbecd8397866`
- Instruction: `9e`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `sahf-load`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_final_flags": "f0d2", "expected_final_flags": "f0d2", "initial_ah": "d0", "initial_flags": "f093"}`
- Initial registers: `{"ax": "d032", "bp": "b0f8", "bx": "7437", "cs": "6c86", "cx": "8d8e", "di": "492f", "ds": "7621", "dx": "0b1e", "es": "7d27", "flags": "f093", "ip": "dd13", "si": "9ba6", "sp": "d862", "ss": "e31d"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "7a573", "value": "9e"}], "registers": {"ax": "d032", "bp": "b0f8", "bx": "7437", "cs": "6c86", "cx": "8d8e", "di": "492f", "ds": "7621", "dx": "0b1e", "es": "7d27", "flags": "f0d2", "ip": "dd14", "si": "9ba6", "sp": "d862", "ss": "e31d"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "7a573", "value": "9e"}], "registers": {"ax": "d032", "bp": "b0f8", "bx": "7437", "cs": "6c86", "cx": "8d8e", "di": "492f", "ds": "7621", "dx": "0b1e", "es": "7d27", "flags": "f0d2", "ip": "dd14", "si": "9ba6", "sp": "d862", "ss": "e31d"}, "termination": "normal"}`

## `007e62c57c1a718b7fb18abf4309759fdd45984163c2d22893ce9bffac585421`

- Form: `9C`
- Upstream hash: `3754620f704d8dd6646c0bb28d2a76c2c681cf35`
- Instruction: `9c`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `pushf-physical-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[{"kind": "expected-stack-byte", "value": "cf04"}, {"kind": "expected-stack-byte", "value": "cf05"}, {"kind": "actual-stack-byte", "value": "cf04"}, {"kind": "actual-stack-byte", "value": "cf05"}]`
- Physical addresses: `[{"kind": "expected-stack-byte", "value": "04f84"}, {"kind": "expected-stack-byte", "value": "04f85"}, {"kind": "actual-stack-byte", "value": "04f84"}, {"kind": "actual-stack-byte", "value": "04f85"}]`
- Item analysis: `{"actual_pushed_flags": "fc87", "actual_stack_logical_addresses": ["cf04", "cf05"], "actual_stack_physical_addresses": ["04f84", "04f85"], "expected_pushed_flags": "fc87", "expected_stack_logical_addresses": ["cf04", "cf05"], "expected_stack_physical_addresses": ["04f84", "04f85"], "initial_flags": "fc87", "physical_boundary_crossed": true, "segment_boundary_crossed": false}`
- Initial registers: `{"ax": "ea6f", "bp": "f752", "bx": "d926", "cs": "d692", "cx": "896f", "di": "bd97", "ds": "e1e6", "dx": "bf14", "es": "ab18", "flags": "fc87", "ip": "71ec", "si": "1965", "sp": "cf06", "ss": "f808"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "04f84", "value": "87"}, {"address": "04f85", "value": "fc"}, {"address": "ddb0c", "value": "9c"}], "registers": {"ax": "ea6f", "bp": "f752", "bx": "d926", "cs": "d692", "cx": "896f", "di": "bd97", "ds": "e1e6", "dx": "bf14", "es": "ab18", "flags": "fc87", "ip": "71ed", "si": "1965", "sp": "cf04", "ss": "f808"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "04f84", "value": "87"}, {"address": "04f85", "value": "fc"}, {"address": "ddb0c", "value": "9c"}], "registers": {"ax": "ea6f", "bp": "f752", "bx": "d926", "cs": "d692", "cx": "896f", "di": "bd97", "ds": "e1e6", "dx": "bf14", "es": "ab18", "flags": "fc87", "ip": "71ed", "si": "1965", "sp": "cf04", "ss": "f808"}, "termination": "normal"}`

## `0199d4e94995854d9b05d3a14ea0edd88ee43a00c9a9664a82bec8855e5e978e`

- Form: `CC`
- Upstream hash: `a1f6e25583cc972778b708abb6085ff635065c49`
- Instruction: `cc`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `physical-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "76af"}, {"kind": "expected-frame-byte", "value": "76b0"}, {"kind": "expected-frame-byte", "value": "76b1"}, {"kind": "expected-frame-byte", "value": "76b2"}, {"kind": "expected-frame-byte", "value": "76b3"}, {"kind": "expected-frame-byte", "value": "76b4"}, {"kind": "actual-frame-byte", "value": "76af"}, {"kind": "actual-frame-byte", "value": "76b0"}, {"kind": "actual-frame-byte", "value": "76b1"}, {"kind": "actual-frame-byte", "value": "76b2"}, {"kind": "actual-frame-byte", "value": "76b3"}, {"kind": "actual-frame-byte", "value": "76b4"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "02a0f"}, {"kind": "expected-frame-byte", "value": "02a10"}, {"kind": "expected-frame-byte", "value": "02a11"}, {"kind": "expected-frame-byte", "value": "02a12"}, {"kind": "expected-frame-byte", "value": "02a13"}, {"kind": "expected-frame-byte", "value": "02a14"}, {"kind": "actual-frame-byte", "value": "02a0f"}, {"kind": "actual-frame-byte", "value": "02a10"}, {"kind": "actual-frame-byte", "value": "02a11"}, {"kind": "actual-frame-byte", "value": "02a12"}, {"kind": "actual-frame-byte", "value": "02a13"}, {"kind": "actual-frame-byte", "value": "02a14"}]`
- Item analysis: `{"actual_final_flags": "f0c3", "actual_final_sp": "76af", "actual_frame_logical_addresses": ["76af", "76b0", "76b1", "76b2", "76b3", "76b4"], "actual_frame_physical_addresses": ["02a0f", "02a10", "02a11", "02a12", "02a13", "02a14"], "actual_saved_cs": "fe22", "actual_saved_flags": "00c3", "actual_saved_ip": "b249", "expected_final_flags": "f0c3", "expected_final_sp": "76af", "expected_frame_logical_addresses": ["76af", "76b0", "76b1", "76b2", "76b3", "76b4"], "expected_frame_physical_addresses": ["02a0f", "02a10", "02a11", "02a12", "02a13", "02a14"], "expected_saved_cs": "fe22", "expected_saved_flags": "f0c3", "expected_saved_ip": "b249", "frame_mapping": "determined", "initial_cs": "fe22", "initial_flags": "f0c3", "initial_ip": "b248", "initial_sp": "76b5", "initial_ss": "fb36", "physical_boundary_crossed": true, "segment_boundary_crossed": false}`
- Initial registers: `{"ax": "30cb", "bp": "90bf", "bx": "9728", "cs": "fe22", "cx": "0b79", "di": "2578", "ds": "fef7", "dx": "325c", "es": "250d", "flags": "f0c3", "ip": "b248", "si": "9331", "sp": "76b5", "ss": "fb36"}`
- Expected final: `{"execution": {"interrupt_vector": 3, "kind": "interrupt"}, "ram": [{"address": "0000c", "value": "bf"}, {"address": "0000d", "value": "5a"}, {"address": "0000e", "value": "db"}, {"address": "0000f", "value": "73"}, {"address": "02a0f", "value": "49"}, {"address": "02a10", "value": "b2"}, {"address": "02a11", "value": "22"}, {"address": "02a12", "value": "fe"}, {"address": "02a13", "value": "c3"}, {"address": "02a14", "value": "f0"}, {"address": "09468", "value": "cc"}], "registers": {"ax": "30cb", "bp": "90bf", "bx": "9728", "cs": "73db", "cx": "0b79", "di": "2578", "ds": "fef7", "dx": "325c", "es": "250d", "flags": "f0c3", "ip": "5abf", "si": "9331", "sp": "76af", "ss": "fb36"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 3, "kind": "software_interrupt", "termination": "normal"}, "ram": [{"address": "0000c", "value": "bf"}, {"address": "0000d", "value": "5a"}, {"address": "0000e", "value": "db"}, {"address": "0000f", "value": "73"}, {"address": "02a0f", "value": "49"}, {"address": "02a10", "value": "b2"}, {"address": "02a11", "value": "22"}, {"address": "02a12", "value": "fe"}, {"address": "02a13", "value": "c3"}, {"address": "02a14", "value": "00"}, {"address": "09468", "value": "cc"}], "registers": {"ax": "30cb", "bp": "90bf", "bx": "9728", "cs": "73db", "cx": "0b79", "di": "2578", "ds": "fef7", "dx": "325c", "es": "250d", "flags": "f0c3", "ip": "5abf", "si": "9331", "sp": "76af", "ss": "fb36"}, "termination": "normal"}`

## `0383c91c3b086a47782972cf959d9cb3ea7245a673162b4e60c57f9f690de8cf`

- Form: `CD`
- Upstream hash: `f1eea885e1baad4153f930aa3cd0536db88709bc`
- Instruction: `cd73`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `physical-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "e39f"}, {"kind": "expected-frame-byte", "value": "e3a0"}, {"kind": "expected-frame-byte", "value": "e3a1"}, {"kind": "expected-frame-byte", "value": "e3a2"}, {"kind": "expected-frame-byte", "value": "e3a3"}, {"kind": "expected-frame-byte", "value": "e3a4"}, {"kind": "actual-frame-byte", "value": "e39f"}, {"kind": "actual-frame-byte", "value": "e3a0"}, {"kind": "actual-frame-byte", "value": "e3a1"}, {"kind": "actual-frame-byte", "value": "e3a2"}, {"kind": "actual-frame-byte", "value": "e3a3"}, {"kind": "actual-frame-byte", "value": "e3a4"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "00f7f"}, {"kind": "expected-frame-byte", "value": "00f80"}, {"kind": "expected-frame-byte", "value": "00f81"}, {"kind": "expected-frame-byte", "value": "00f82"}, {"kind": "expected-frame-byte", "value": "00f83"}, {"kind": "expected-frame-byte", "value": "00f84"}, {"kind": "actual-frame-byte", "value": "00f7f"}, {"kind": "actual-frame-byte", "value": "00f80"}, {"kind": "actual-frame-byte", "value": "00f81"}, {"kind": "actual-frame-byte", "value": "00f82"}, {"kind": "actual-frame-byte", "value": "00f83"}, {"kind": "actual-frame-byte", "value": "00f84"}]`
- Item analysis: `{"actual_final_flags": "f4d6", "actual_final_sp": "e39f", "actual_frame_logical_addresses": ["e39f", "e3a0", "e3a1", "e3a2", "e3a3", "e3a4"], "actual_frame_physical_addresses": ["00f7f", "00f80", "00f81", "00f82", "00f83", "00f84"], "actual_saved_cs": "bd13", "actual_saved_flags": "04d6", "actual_saved_ip": "3a13", "expected_final_flags": "f4d6", "expected_final_sp": "e39f", "expected_frame_logical_addresses": ["e39f", "e3a0", "e3a1", "e3a2", "e3a3", "e3a4"], "expected_frame_physical_addresses": ["00f7f", "00f80", "00f81", "00f82", "00f83", "00f84"], "expected_saved_cs": "bd13", "expected_saved_flags": "f4d6", "expected_saved_ip": "3a13", "frame_mapping": "determined", "initial_cs": "bd13", "initial_flags": "f4d6", "initial_ip": "3a11", "initial_sp": "e3a5", "initial_ss": "f2be", "physical_boundary_crossed": true, "segment_boundary_crossed": false}`
- Initial registers: `{"ax": "01cf", "bp": "4a0d", "bx": "e147", "cs": "bd13", "cx": "ce7f", "di": "8223", "ds": "3a9f", "dx": "5279", "es": "9d57", "flags": "f4d6", "ip": "3a11", "si": "38aa", "sp": "e3a5", "ss": "f2be"}`
- Expected final: `{"execution": {"interrupt_vector": 115, "kind": "interrupt"}, "ram": [{"address": "001cc", "value": "fa"}, {"address": "001cd", "value": "19"}, {"address": "001ce", "value": "d0"}, {"address": "001cf", "value": "89"}, {"address": "00f7f", "value": "13"}, {"address": "00f80", "value": "3a"}, {"address": "00f81", "value": "13"}, {"address": "00f82", "value": "bd"}, {"address": "00f83", "value": "d6"}, {"address": "00f84", "value": "f4"}, {"address": "c0b41", "value": "cd"}, {"address": "c0b42", "value": "73"}], "registers": {"ax": "01cf", "bp": "4a0d", "bx": "e147", "cs": "89d0", "cx": "ce7f", "di": "8223", "ds": "3a9f", "dx": "5279", "es": "9d57", "flags": "f4d6", "ip": "19fa", "si": "38aa", "sp": "e39f", "ss": "f2be"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 115, "kind": "software_interrupt", "termination": "normal"}, "ram": [{"address": "001cc", "value": "fa"}, {"address": "001cd", "value": "19"}, {"address": "001ce", "value": "d0"}, {"address": "001cf", "value": "89"}, {"address": "00f7f", "value": "13"}, {"address": "00f80", "value": "3a"}, {"address": "00f81", "value": "13"}, {"address": "00f82", "value": "bd"}, {"address": "00f83", "value": "d6"}, {"address": "00f84", "value": "04"}, {"address": "c0b41", "value": "cd"}, {"address": "c0b42", "value": "73"}], "registers": {"ax": "01cf", "bp": "4a0d", "bx": "e147", "cs": "89d0", "cx": "ce7f", "di": "8223", "ds": "3a9f", "dx": "5279", "es": "9d57", "flags": "f4d6", "ip": "19fa", "si": "38aa", "sp": "e39f", "ss": "f2be"}, "termination": "normal"}`

## `04bd44786f3ab29fd4c844a20f686418085bbeb54644563693a454b5995efa61`

- Form: `CE`
- Upstream hash: `c3d91b4020f0281178840463de4d53d51de5bd51`
- Instruction: `ce`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `physical-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "6084"}, {"kind": "expected-frame-byte", "value": "6085"}, {"kind": "expected-frame-byte", "value": "6086"}, {"kind": "expected-frame-byte", "value": "6087"}, {"kind": "expected-frame-byte", "value": "6088"}, {"kind": "expected-frame-byte", "value": "6089"}, {"kind": "actual-frame-byte", "value": "6084"}, {"kind": "actual-frame-byte", "value": "6085"}, {"kind": "actual-frame-byte", "value": "6086"}, {"kind": "actual-frame-byte", "value": "6087"}, {"kind": "actual-frame-byte", "value": "6088"}, {"kind": "actual-frame-byte", "value": "6089"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "022f4"}, {"kind": "expected-frame-byte", "value": "022f5"}, {"kind": "expected-frame-byte", "value": "022f6"}, {"kind": "expected-frame-byte", "value": "022f7"}, {"kind": "expected-frame-byte", "value": "022f8"}, {"kind": "expected-frame-byte", "value": "022f9"}, {"kind": "actual-frame-byte", "value": "022f4"}, {"kind": "actual-frame-byte", "value": "022f5"}, {"kind": "actual-frame-byte", "value": "022f6"}, {"kind": "actual-frame-byte", "value": "022f7"}, {"kind": "actual-frame-byte", "value": "022f8"}, {"kind": "actual-frame-byte", "value": "022f9"}]`
- Item analysis: `{"actual_final_flags": "f853", "actual_final_sp": "6084", "actual_frame_logical_addresses": ["6084", "6085", "6086", "6087", "6088", "6089"], "actual_frame_physical_addresses": ["022f4", "022f5", "022f6", "022f7", "022f8", "022f9"], "actual_saved_cs": "2665", "actual_saved_flags": "0853", "actual_saved_ip": "8cf8", "expected_final_flags": "f853", "expected_final_sp": "6084", "expected_frame_logical_addresses": ["6084", "6085", "6086", "6087", "6088", "6089"], "expected_frame_physical_addresses": ["022f4", "022f5", "022f6", "022f7", "022f8", "022f9"], "expected_saved_cs": "2665", "expected_saved_flags": "f853", "expected_saved_ip": "8cf8", "frame_mapping": "determined", "initial_cs": "2665", "initial_flags": "f853", "initial_ip": "8cf7", "initial_sp": "608a", "initial_ss": "fc27", "physical_boundary_crossed": true, "segment_boundary_crossed": false}`
- Initial registers: `{"ax": "0332", "bp": "917f", "bx": "3a86", "cs": "2665", "cx": "c43b", "di": "176a", "ds": "abae", "dx": "e438", "es": "2e9b", "flags": "f853", "ip": "8cf7", "si": "c241", "sp": "608a", "ss": "fc27"}`
- Expected final: `{"execution": {"interrupt_vector": 4, "kind": "interrupt"}, "ram": [{"address": "00010", "value": "7c"}, {"address": "00011", "value": "9d"}, {"address": "00012", "value": "8c"}, {"address": "00013", "value": "b2"}, {"address": "022f4", "value": "f8"}, {"address": "022f5", "value": "8c"}, {"address": "022f6", "value": "65"}, {"address": "022f7", "value": "26"}, {"address": "022f8", "value": "53"}, {"address": "022f9", "value": "f8"}, {"address": "2f347", "value": "ce"}], "registers": {"ax": "0332", "bp": "917f", "bx": "3a86", "cs": "b28c", "cx": "c43b", "di": "176a", "ds": "abae", "dx": "e438", "es": "2e9b", "flags": "f853", "ip": "9d7c", "si": "c241", "sp": "6084", "ss": "fc27"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 4, "kind": "software_interrupt", "termination": "normal"}, "ram": [{"address": "00010", "value": "7c"}, {"address": "00011", "value": "9d"}, {"address": "00012", "value": "8c"}, {"address": "00013", "value": "b2"}, {"address": "022f4", "value": "f8"}, {"address": "022f5", "value": "8c"}, {"address": "022f6", "value": "65"}, {"address": "022f7", "value": "26"}, {"address": "022f8", "value": "53"}, {"address": "022f9", "value": "08"}, {"address": "2f347", "value": "ce"}], "registers": {"ax": "0332", "bp": "917f", "bx": "3a86", "cs": "b28c", "cx": "c43b", "di": "176a", "ds": "abae", "dx": "e438", "es": "2e9b", "flags": "f853", "ip": "9d7c", "si": "c241", "sp": "6084", "ss": "fc27"}, "termination": "normal"}`

## `97656ccbabdcb985a47ad03aa0f348b7ba5a9228a730f3a807b2fd58fd2216c9`

- Form: `CC`
- Upstream hash: `3edf2d91d70eb8e6e6055ae6660ce781bb4bc7a2`
- Instruction: `cc`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `segment-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-frame-byte", "value": "fffe"}, {"kind": "expected-frame-byte", "value": "ffff"}, {"kind": "expected-frame-byte", "value": "0000"}, {"kind": "expected-frame-byte", "value": "0001"}, {"kind": "expected-frame-byte", "value": "0002"}, {"kind": "expected-frame-byte", "value": "0003"}, {"kind": "actual-frame-byte", "value": "fffe"}, {"kind": "actual-frame-byte", "value": "ffff"}, {"kind": "actual-frame-byte", "value": "0000"}, {"kind": "actual-frame-byte", "value": "0001"}, {"kind": "actual-frame-byte", "value": "0002"}, {"kind": "actual-frame-byte", "value": "0003"}]`
- Physical addresses: `[{"kind": "expected-frame-byte", "value": "fae3e"}, {"kind": "expected-frame-byte", "value": "fae3f"}, {"kind": "expected-frame-byte", "value": "eae40"}, {"kind": "expected-frame-byte", "value": "eae41"}, {"kind": "expected-frame-byte", "value": "eae42"}, {"kind": "expected-frame-byte", "value": "eae43"}, {"kind": "actual-frame-byte", "value": "fae3e"}, {"kind": "actual-frame-byte", "value": "fae3f"}, {"kind": "actual-frame-byte", "value": "eae40"}, {"kind": "actual-frame-byte", "value": "eae41"}, {"kind": "actual-frame-byte", "value": "eae42"}, {"kind": "actual-frame-byte", "value": "eae43"}]`
- Item analysis: `{"actual_final_flags": "fc16", "actual_final_sp": "fffe", "actual_frame_logical_addresses": ["fffe", "ffff", "0000", "0001", "0002", "0003"], "actual_frame_physical_addresses": ["fae3e", "fae3f", "eae40", "eae41", "eae42", "eae43"], "actual_saved_cs": "4d2e", "actual_saved_flags": "0c16", "actual_saved_ip": "1162", "expected_final_flags": "fc16", "expected_final_sp": "fffe", "expected_frame_logical_addresses": ["fffe", "ffff", "0000", "0001", "0002", "0003"], "expected_frame_physical_addresses": ["fae3e", "fae3f", "eae40", "eae41", "eae42", "eae43"], "expected_saved_cs": "4d2e", "expected_saved_flags": "fc16", "expected_saved_ip": "1162", "frame_mapping": "determined", "initial_cs": "4d2e", "initial_flags": "fc16", "initial_ip": "1161", "initial_sp": "0004", "initial_ss": "eae4", "physical_boundary_crossed": false, "segment_boundary_crossed": true}`
- Initial registers: `{"ax": "990c", "bp": "185f", "bx": "347e", "cs": "4d2e", "cx": "0507", "di": "a398", "ds": "b9a6", "dx": "1dc2", "es": "69b2", "flags": "fc16", "ip": "1161", "si": "65db", "sp": "0004", "ss": "eae4"}`
- Expected final: `{"execution": {"interrupt_vector": 3, "kind": "interrupt"}, "ram": [{"address": "0000c", "value": "f4"}, {"address": "0000d", "value": "66"}, {"address": "0000e", "value": "9b"}, {"address": "0000f", "value": "ef"}, {"address": "4e441", "value": "cc"}, {"address": "eae40", "value": "2e"}, {"address": "eae41", "value": "4d"}, {"address": "eae42", "value": "16"}, {"address": "eae43", "value": "fc"}, {"address": "fae3e", "value": "62"}, {"address": "fae3f", "value": "11"}], "registers": {"ax": "990c", "bp": "185f", "bx": "347e", "cs": "ef9b", "cx": "0507", "di": "a398", "ds": "b9a6", "dx": "1dc2", "es": "69b2", "flags": "fc16", "ip": "66f4", "si": "65db", "sp": "fffe", "ss": "eae4"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 1, "interrupt_vector": 3, "kind": "software_interrupt", "termination": "normal"}, "ram": [{"address": "0000c", "value": "f4"}, {"address": "0000d", "value": "66"}, {"address": "0000e", "value": "9b"}, {"address": "0000f", "value": "ef"}, {"address": "4e441", "value": "cc"}, {"address": "eae40", "value": "2e"}, {"address": "eae41", "value": "4d"}, {"address": "eae42", "value": "16"}, {"address": "eae43", "value": "0c"}, {"address": "fae3e", "value": "62"}, {"address": "fae3f", "value": "11"}], "registers": {"ax": "990c", "bp": "185f", "bx": "347e", "cs": "ef9b", "cx": "0507", "di": "a398", "ds": "b9a6", "dx": "1dc2", "es": "69b2", "flags": "fc16", "ip": "66f4", "si": "65db", "sp": "fffe", "ss": "eae4"}, "termination": "normal"}`

## `a653d376bbf27c39b1d3a8e6dc232b692355947fb81a0f1051c91ca8606b13e8`

- Form: `9C`
- Upstream hash: `a2c8233d52273767783ecd3ee27110273eff445f`
- Instruction: `9c`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `pushf-segment-boundary`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "expected-stack-byte", "value": "ffff"}, {"kind": "expected-stack-byte", "value": "0000"}, {"kind": "actual-stack-byte", "value": "ffff"}, {"kind": "actual-stack-byte", "value": "0000"}]`
- Physical addresses: `[{"kind": "expected-stack-byte", "value": "4da7f"}, {"kind": "expected-stack-byte", "value": "3da80"}, {"kind": "actual-stack-byte", "value": "4da7f"}, {"kind": "actual-stack-byte", "value": "3da80"}]`
- Item analysis: `{"actual_pushed_flags": "00d6", "actual_stack_logical_addresses": ["ffff", "0000"], "actual_stack_physical_addresses": ["4da7f", "3da80"], "expected_pushed_flags": "f0d6", "expected_stack_logical_addresses": ["ffff", "0000"], "expected_stack_physical_addresses": ["4da7f", "3da80"], "initial_flags": "f0d6", "physical_boundary_crossed": false, "segment_boundary_crossed": true}`
- Initial registers: `{"ax": "b6b3", "bp": "8a37", "bx": "56b7", "cs": "adfd", "cx": "88a9", "di": "a865", "ds": "0474", "dx": "29ae", "es": "e94d", "flags": "f0d6", "ip": "1606", "si": "9503", "sp": "0001", "ss": "3da8"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "3da80", "value": "f0"}, {"address": "4da7f", "value": "d6"}, {"address": "af5d6", "value": "9c"}], "registers": {"ax": "b6b3", "bp": "8a37", "bx": "56b7", "cs": "adfd", "cx": "88a9", "di": "a865", "ds": "0474", "dx": "29ae", "es": "e94d", "flags": "f0d6", "ip": "1607", "si": "9503", "sp": "ffff", "ss": "3da8"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "3da80", "value": "00"}, {"address": "4da7f", "value": "d6"}, {"address": "af5d6", "value": "9c"}], "registers": {"ax": "b6b3", "bp": "8a37", "bx": "56b7", "cs": "adfd", "cx": "88a9", "di": "a865", "ds": "0474", "dx": "29ae", "es": "e94d", "flags": "f0d6", "ip": "1607", "si": "9503", "sp": "ffff", "ss": "3da8"}, "termination": "normal"}`
