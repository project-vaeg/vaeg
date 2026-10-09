<!--
Copyright (c) 2026 Nakata Maho

SPDX-License-Identifier: BSD-2-Clause
-->
# M59 representative evidence: canary

These records are selected deterministically from the complete machine table.
Expected and actual final state are shown independently.

## `0007c6816eaa00b97019651aaf5fee7f5fc7d81661413e69aaaf6952ac441325`

- Form: `C7`
- Upstream hash: `63fb4daff5c16d63b33cb6ffe128b46878f560d0`
- Instruction: `26c7aa247bdf4a`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `memory:pass`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[{"kind": "operand", "value": "8d32"}]`
- Physical addresses: `[{"kind": "operand", "value": "a6fa2"}]`
- Item analysis: `{"actual_destination": "4adf", "expected_destination": "4adf", "initial_destination": null, "logical_offset": "8d32", "offset_wrap": true, "operand_width": 16, "physical_address": "a6fa2", "physical_wrap": false, "segment": "es", "segment_value": "9e27"}`
- Initial registers: `{"ax": "247d", "bp": "2855", "bx": "3407", "cs": "096b", "cx": "bfc3", "di": "67d3", "ds": "5ba5", "dx": "5de2", "es": "9e27", "flags": "fc17", "ip": "f51b", "si": "e9b9", "sp": "3302", "ss": "9e0d"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "18bcb", "value": "26"}, {"address": "18bcc", "value": "c7"}, {"address": "18bcd", "value": "aa"}, {"address": "18bce", "value": "24"}, {"address": "18bcf", "value": "7b"}, {"address": "18bd0", "value": "df"}, {"address": "18bd1", "value": "4a"}, {"address": "a6fa2", "value": "df"}, {"address": "a6fa3", "value": "4a"}], "registers": {"ax": "247d", "bp": "2855", "bx": "3407", "cs": "096b", "cx": "bfc3", "di": "67d3", "ds": "5ba5", "dx": "5de2", "es": "9e27", "flags": "fc17", "ip": "f522", "si": "e9b9", "sp": "3302", "ss": "9e0d"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "18bcb", "value": "26"}, {"address": "18bcc", "value": "c7"}, {"address": "18bcd", "value": "aa"}, {"address": "18bce", "value": "24"}, {"address": "18bcf", "value": "7b"}, {"address": "18bd0", "value": "df"}, {"address": "18bd1", "value": "4a"}, {"address": "a6fa2", "value": "df"}, {"address": "a6fa3", "value": "4a"}], "registers": {"ax": "247d", "bp": "2855", "bx": "3407", "cs": "096b", "cx": "bfc3", "di": "67d3", "ds": "5ba5", "dx": "5de2", "es": "9e27", "flags": "fc17", "ip": "f522", "si": "e9b9", "sp": "3302", "ss": "9e0d"}, "termination": "normal"}`

## `00098a1727add96bce60dc61cb11cf62ee357fc68e8d4d18ac76abd7dd85e658`

- Form: `C6`
- Upstream hash: `3ead383cdaf66d6db1a84ca1963c01400bccb757`
- Instruction: `26c6f08c`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `register:failure`
- Conclusion status: `proven`
- Architectural mismatches: `registers`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_destination": "ba", "expected_destination": "8c", "initial_destination": "ba", "operand_width": 8}`
- Initial registers: `{"ax": "5bba", "bp": "27ac", "bx": "c225", "cs": "41a2", "cx": "7d61", "di": "9b80", "ds": "961f", "dx": "9421", "es": "d07a", "flags": "f406", "ip": "ad2f", "si": "f2a0", "sp": "6f0b", "ss": "dc87"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "4c74f", "value": "26"}, {"address": "4c750", "value": "c6"}, {"address": "4c751", "value": "f0"}, {"address": "4c752", "value": "8c"}], "registers": {"ax": "5b8c", "bp": "27ac", "bx": "c225", "cs": "41a2", "cx": "7d61", "di": "9b80", "ds": "961f", "dx": "9421", "es": "d07a", "flags": "f406", "ip": "ad33", "si": "f2a0", "sp": "6f0b", "ss": "dc87"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "4c74f", "value": "26"}, {"address": "4c750", "value": "c6"}, {"address": "4c751", "value": "f0"}, {"address": "4c752", "value": "8c"}], "registers": {"ax": "5bba", "bp": "27ac", "bx": "c225", "cs": "41a2", "cx": "7d61", "di": "9b80", "ds": "961f", "dx": "8c21", "es": "d07a", "flags": "f406", "ip": "ad33", "si": "f2a0", "sp": "6f0b", "ss": "dc87"}, "termination": "normal"}`

## `00129c0c301ba851127aaa940be80371267c4479465a7d5cacdba20a524fb5fb`

- Form: `C6`
- Upstream hash: `e11e80a48f9b608724cd2f8977c250b127f3ff23`
- Instruction: `c66b8442`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `memory:pass`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[{"kind": "operand", "value": "c09a"}]`
- Physical addresses: `[{"kind": "operand", "value": "7544a"}]`
- Item analysis: `{"actual_destination": "42", "expected_destination": "42", "initial_destination": null, "logical_offset": "c09a", "offset_wrap": false, "operand_width": 8, "physical_address": "7544a", "physical_wrap": false, "segment": "ss", "segment_value": "693b"}`
- Initial registers: `{"ax": "19c4", "bp": "1787", "bx": "b7db", "cs": "56a0", "cx": "ba92", "di": "a98f", "ds": "1161", "dx": "df93", "es": "e01d", "flags": "f852", "ip": "8659", "si": "247d", "sp": "b159", "ss": "693b"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "5f059", "value": "c6"}, {"address": "5f05a", "value": "6b"}, {"address": "5f05b", "value": "84"}, {"address": "5f05c", "value": "42"}, {"address": "7544a", "value": "42"}], "registers": {"ax": "19c4", "bp": "1787", "bx": "b7db", "cs": "56a0", "cx": "ba92", "di": "a98f", "ds": "1161", "dx": "df93", "es": "e01d", "flags": "f852", "ip": "865d", "si": "247d", "sp": "b159", "ss": "693b"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "5f059", "value": "c6"}, {"address": "5f05a", "value": "6b"}, {"address": "5f05b", "value": "84"}, {"address": "5f05c", "value": "42"}, {"address": "7544a", "value": "42"}], "registers": {"ax": "19c4", "bp": "1787", "bx": "b7db", "cs": "56a0", "cx": "ba92", "di": "a98f", "ds": "1161", "dx": "df93", "es": "e01d", "flags": "f852", "ip": "865d", "si": "247d", "sp": "b159", "ss": "693b"}, "termination": "normal"}`

## `001c44f80c858abdd2444a54069c7d6d4812bb90dd762d492491ed06d411f1c1`

- Form: `F7.2`
- Upstream hash: `06d979cc2bc08b65a41b9bf911a0ba95bb49ec95`
- Instruction: `f7d3`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `register:pass`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_destination": "c530", "expected_destination": "c530", "initial_destination": "3acf", "operand_width": 16}`
- Initial registers: `{"ax": "3d84", "bp": "2fbb", "bx": "3acf", "cs": "a02f", "cx": "a8e2", "di": "468f", "ds": "c746", "dx": "dac9", "es": "8618", "flags": "fcc6", "ip": "2c13", "si": "2546", "sp": "22c8", "ss": "831d"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "a2f03", "value": "f7"}, {"address": "a2f04", "value": "d3"}], "registers": {"ax": "3d84", "bp": "2fbb", "bx": "c530", "cs": "a02f", "cx": "a8e2", "di": "468f", "ds": "c746", "dx": "dac9", "es": "8618", "flags": "fcc6", "ip": "2c15", "si": "2546", "sp": "22c8", "ss": "831d"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "a2f03", "value": "f7"}, {"address": "a2f04", "value": "d3"}], "registers": {"ax": "3d84", "bp": "2fbb", "bx": "c530", "cs": "a02f", "cx": "a8e2", "di": "468f", "ds": "c746", "dx": "dac9", "es": "8618", "flags": "fcc6", "ip": "2c15", "si": "2546", "sp": "22c8", "ss": "831d"}, "termination": "normal"}`

## `0025400b2395546327f0a12875e2a6d301c3627ed250a7b1f3e779cc7d6ff18c`

- Form: `C7`
- Upstream hash: `f205c32d73f43e8ee800d0afb2b98f84cd5ec716`
- Instruction: `3ec7f1df49`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `register:failure`
- Conclusion status: `proven`
- Architectural mismatches: `registers`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_destination": "2ebb", "expected_destination": "49df", "initial_destination": "2ebb", "operand_width": 16}`
- Initial registers: `{"ax": "4e9b", "bp": "d605", "bx": "2968", "cs": "adc6", "cx": "2ebb", "di": "192f", "ds": "6d20", "dx": "2a48", "es": "12e7", "flags": "f4d3", "ip": "6463", "si": "2643", "sp": "370d", "ss": "cec5"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "b40c3", "value": "3e"}, {"address": "b40c4", "value": "c7"}, {"address": "b40c5", "value": "f1"}, {"address": "b40c6", "value": "df"}, {"address": "b40c7", "value": "49"}], "registers": {"ax": "4e9b", "bp": "d605", "bx": "2968", "cs": "adc6", "cx": "49df", "di": "192f", "ds": "6d20", "dx": "2a48", "es": "12e7", "flags": "f4d3", "ip": "6468", "si": "2643", "sp": "370d", "ss": "cec5"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "b40c3", "value": "3e"}, {"address": "b40c4", "value": "c7"}, {"address": "b40c5", "value": "f1"}, {"address": "b40c6", "value": "df"}, {"address": "b40c7", "value": "49"}], "registers": {"ax": "4e9b", "bp": "d605", "bx": "2968", "cs": "adc6", "cx": "2ebb", "di": "192f", "ds": "6d20", "dx": "2a48", "es": "12e7", "flags": "f4d3", "ip": "6468", "si": "49df", "sp": "370d", "ss": "cec5"}, "termination": "normal"}`

## `0025a9cdee82c8146db46790b6b0b59631eac5415c35f13e399df9753ef879c1`

- Form: `F7.2`
- Upstream hash: `04b698732f8395489a207dee874be8844d2aa7b0`
- Instruction: `2ef711`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `memory:pass`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[{"kind": "operand", "value": "cb66"}]`
- Physical addresses: `[{"kind": "operand", "value": "f2756"}]`
- Item analysis: `{"actual_destination": "8ab5", "expected_destination": "8ab5", "initial_destination": "754a", "logical_offset": "cb66", "offset_wrap": false, "operand_width": 16, "physical_address": "f2756", "physical_wrap": false, "segment": "cs", "segment_value": "e5bf"}`
- Initial registers: `{"ax": "8a18", "bp": "1a44", "bx": "2406", "cs": "e5bf", "cx": "a775", "di": "a760", "ds": "5219", "dx": "2dc8", "es": "2897", "flags": "f813", "ip": "0be4", "si": "7585", "sp": "7e8a", "ss": "2815"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "e67d4", "value": "2e"}, {"address": "e67d5", "value": "f7"}, {"address": "e67d6", "value": "11"}, {"address": "f2756", "value": "b5"}, {"address": "f2757", "value": "8a"}], "registers": {"ax": "8a18", "bp": "1a44", "bx": "2406", "cs": "e5bf", "cx": "a775", "di": "a760", "ds": "5219", "dx": "2dc8", "es": "2897", "flags": "f813", "ip": "0be7", "si": "7585", "sp": "7e8a", "ss": "2815"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "e67d4", "value": "2e"}, {"address": "e67d5", "value": "f7"}, {"address": "e67d6", "value": "11"}, {"address": "f2756", "value": "b5"}, {"address": "f2757", "value": "8a"}], "registers": {"ax": "8a18", "bp": "1a44", "bx": "2406", "cs": "e5bf", "cx": "a775", "di": "a760", "ds": "5219", "dx": "2dc8", "es": "2897", "flags": "f813", "ip": "0be7", "si": "7585", "sp": "7e8a", "ss": "2815"}, "termination": "normal"}`

## `0053d136ae70e95fdda6041d307e979fcb9057e5d98a360f1068da42cd82722c`

- Form: `C6`
- Upstream hash: `bdfe76712a3aedac53bdec92d256b08d5e5986b5`
- Instruction: `c6f6b8`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `register:pass`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_destination": "b8", "expected_destination": "b8", "initial_destination": "b0", "operand_width": 8}`
- Initial registers: `{"ax": "5977", "bp": "ee97", "bx": "160c", "cs": "9f1c", "cx": "72ec", "di": "1dba", "ds": "8575", "dx": "b050", "es": "cfe2", "flags": "f042", "ip": "97f5", "si": "3659", "sp": "a1b1", "ss": "0a91"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "a89b5", "value": "c6"}, {"address": "a89b6", "value": "f6"}, {"address": "a89b7", "value": "b8"}], "registers": {"ax": "5977", "bp": "ee97", "bx": "160c", "cs": "9f1c", "cx": "72ec", "di": "1dba", "ds": "8575", "dx": "b850", "es": "cfe2", "flags": "f042", "ip": "97f8", "si": "3659", "sp": "a1b1", "ss": "0a91"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "a89b5", "value": "c6"}, {"address": "a89b6", "value": "f6"}, {"address": "a89b7", "value": "b8"}], "registers": {"ax": "5977", "bp": "ee97", "bx": "160c", "cs": "9f1c", "cx": "72ec", "di": "1dba", "ds": "8575", "dx": "b850", "es": "cfe2", "flags": "f042", "ip": "97f8", "si": "3659", "sp": "a1b1", "ss": "0a91"}, "termination": "normal"}`

## `00a4ab13aa350fc2f4d697d9b60c6a92bd22d51e1ffbefeaba07a0eaff8ac863`

- Form: `F7.2`
- Upstream hash: `f310f5f3e62412925d46a357fbcf3efa764fb0c7`
- Instruction: `f7522c`
- Classification: `applicable`
- Official outcome: `fail`
- Primary partition: `memory:failure`
- Conclusion status: `proven`
- Architectural mismatches: `ram`
- Logical addresses: `[{"kind": "operand", "value": "fcb8"}]`
- Physical addresses: `[{"kind": "operand", "value": "53d88"}]`
- Item analysis: `{"actual_destination": "3324", "expected_destination": "cc24", "initial_destination": "33db", "logical_offset": "fcb8", "offset_wrap": false, "operand_width": 16, "physical_address": "53d88", "physical_wrap": false, "segment": "ss", "segment_value": "440d"}`
- Initial registers: `{"ax": "63b1", "bp": "b9e3", "bx": "1870", "cs": "8052", "cx": "7bff", "di": "e4ba", "ds": "599c", "dx": "9174", "es": "d553", "flags": "fcc2", "ip": "8b25", "si": "42a9", "sp": "4091", "ss": "440d"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "53d88", "value": "24"}, {"address": "53d89", "value": "cc"}, {"address": "89045", "value": "f7"}, {"address": "89046", "value": "52"}, {"address": "89047", "value": "2c"}], "registers": {"ax": "63b1", "bp": "b9e3", "bx": "1870", "cs": "8052", "cx": "7bff", "di": "e4ba", "ds": "599c", "dx": "9174", "es": "d553", "flags": "fcc2", "ip": "8b28", "si": "42a9", "sp": "4091", "ss": "440d"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "53d88", "value": "24"}, {"address": "53d89", "value": "33"}, {"address": "89045", "value": "f7"}, {"address": "89046", "value": "52"}, {"address": "89047", "value": "2c"}], "registers": {"ax": "63b1", "bp": "b9e3", "bx": "1870", "cs": "8052", "cx": "7bff", "di": "e4ba", "ds": "599c", "dx": "9174", "es": "d553", "flags": "fcc2", "ip": "8b28", "si": "42a9", "sp": "4091", "ss": "440d"}, "termination": "normal"}`

## `03d3a7d6a048c9046cc9b3039ad2108e4deb22683ef865218c430f873fda6dc1`

- Form: `C7`
- Upstream hash: `26527b02809cf98a04102df10816b51f039e78aa`
- Instruction: `26c7f6f7c6`
- Classification: `applicable`
- Official outcome: `pass`
- Primary partition: `register:pass`
- Conclusion status: `proven`
- Architectural mismatches: `none`
- Logical addresses: `[]`
- Physical addresses: `[]`
- Item analysis: `{"actual_destination": "c6f7", "expected_destination": "c6f7", "initial_destination": "ddd4", "operand_width": 16}`
- Initial registers: `{"ax": "9ca7", "bp": "a313", "bx": "bdd1", "cs": "da83", "cx": "4d71", "di": "8724", "ds": "208f", "dx": "342f", "es": "62d1", "flags": "fc06", "ip": "82ca", "si": "ddd4", "sp": "7b21", "ss": "850d"}`
- Expected final: `{"execution": {"interrupt_vector": null, "kind": "normal"}, "ram": [{"address": "e2afa", "value": "26"}, {"address": "e2afb", "value": "c7"}, {"address": "e2afc", "value": "f6"}, {"address": "e2afd", "value": "f7"}, {"address": "e2afe", "value": "c6"}], "registers": {"ax": "9ca7", "bp": "a313", "bx": "bdd1", "cs": "da83", "cx": "4d71", "di": "8724", "ds": "208f", "dx": "342f", "es": "62d1", "flags": "fc06", "ip": "82cf", "si": "c6f7", "sp": "7b21", "ss": "850d"}, "termination": "normal"}`
- Actual final: `{"execution": {"interrupt_count": 0, "interrupt_vector": null, "kind": "normal", "termination": "normal"}, "ram": [{"address": "e2afa", "value": "26"}, {"address": "e2afb", "value": "c7"}, {"address": "e2afc", "value": "f6"}, {"address": "e2afd", "value": "f7"}, {"address": "e2afe", "value": "c6"}], "registers": {"ax": "9ca7", "bp": "a313", "bx": "bdd1", "cs": "da83", "cx": "4d71", "di": "8724", "ds": "208f", "dx": "342f", "es": "62d1", "flags": "fc06", "ip": "82cf", "si": "c6f7", "sp": "7b21", "ss": "850d"}, "termination": "normal"}`
