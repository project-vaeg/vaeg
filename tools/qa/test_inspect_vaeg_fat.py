#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.

import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("inspect_vaeg_fat", ROOT / "tools/inspect_vaeg_fat.py")
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


PHYSICAL = 256
HEADER = 220
TOTAL_PHYSICAL = 40000
PARTITION_LBA = 8
BYTES_PER_SECTOR = 1024
BLOCKS_PER_LOGICAL = 4
TOTAL_LOGICAL = 9990
FAT_SECTORS = 20


def make_image(path: Path, corrupt_fat=False, mismatch_fat=False, nonzero_padding=False):
    data = bytearray(HEADER + TOTAL_PHYSICAL * PHYSICAL)
    data[:7] = b"VHD1.00"
    struct.pack_into("<H", data, 0x8C, 40)
    struct.pack_into("<H", data, 0x8E, PHYSICAL)
    data[0x90] = 32
    data[0x91] = 8
    struct.pack_into("<H", data, 0x92, 640)
    struct.pack_into("<I", data, 0x94, TOTAL_PHYSICAL)
    boot_off = HEADER + PARTITION_LBA * PHYSICAL
    boot = bytearray(BYTES_PER_SECTOR)
    boot[0:3] = b"VAE"
    struct.pack_into("<H", boot, 11, BYTES_PER_SECTOR)
    boot[13] = 1
    struct.pack_into("<H", boot, 14, 1)
    boot[16] = 2
    struct.pack_into("<H", boot, 17, 32)
    struct.pack_into("<H", boot, 19, TOTAL_LOGICAL)
    boot[21] = 0xF8
    struct.pack_into("<H", boot, 22, FAT_SECTORS)
    struct.pack_into("<H", boot, 24, 32)
    struct.pack_into("<H", boot, 26, 8)
    struct.pack_into("<I", boot, 28, 2)
    boot[510:512] = b"\x55\xaa"
    data[boot_off:boot_off + BYTES_PER_SECTOR] = boot
    fat_size = FAT_SECTORS * BYTES_PER_SECTOR
    fat = bytearray(fat_size)
    struct.pack_into("<HH", fat, 0, 0xFFF8, 0xFFFF)
    if corrupt_fat:
        for off in range(4, fat_size, 2):
            struct.pack_into("<H", fat, off, 0xFFFF)
    if nonzero_padding:
        # Leave valid data-cluster entries free, but mark only the FAT
        # capacity padding.  The inspector must not count these entries.
        cluster_count = (TOTAL_LOGICAL - (1 + 2 * FAT_SECTORS + 1))
        first_padding = 2 + cluster_count
        for entry in range(first_padding, fat_size // 2):
            struct.pack_into("<H", fat, entry * 2, 0xFFFF)
    fat1_off = HEADER + (PARTITION_LBA + 4) * PHYSICAL
    fat2_off = HEADER + (PARTITION_LBA + 4 + FAT_SECTORS * BLOCKS_PER_LOGICAL) * PHYSICAL
    data[fat1_off:fat1_off + fat_size] = fat
    data[fat2_off:fat2_off + fat_size] = fat
    if mismatch_fat:
        data[fat2_off + 512] ^= 0x01
    root_off = HEADER + (PARTITION_LBA + (1 + 2 * FAT_SECTORS) * BLOCKS_PER_LOGICAL) * PHYSICAL
    data[root_off:root_off + BYTES_PER_SECTOR] = bytes(BYTES_PER_SECTOR)
    path.write_bytes(data)


class FatInspectionTests(unittest.TestCase):
    def test_fat16_bpb_is_decoded_from_four_physical_blocks(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "healthy.hdd"
            make_image(path)
            result = MODULE.inspect(path, PHYSICAL)
            self.assertEqual(len(result["candidates"]), 1)
            self.assertEqual(result["selected"]["partition_start_physical_lba"], PARTITION_LBA)
            self.assertEqual(result["selected"]["bytes_per_sector"], 1024)
            self.assertEqual(result["selected"]["fat_type"], "FAT16")

    def test_fat16_free_cluster_count(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "healthy.hdd"
            make_image(path)
            result = MODULE.inspect(path, PHYSICAL)
            copy = result["fat"]["copies"][0]
            self.assertGreater(copy["free_entries"], 0)
            self.assertEqual(copy["valid_data_cluster_entries"], result["selected"]["cluster_count"])
            self.assertEqual(copy["free_entries"], result["selected"]["cluster_count"])
            self.assertEqual(copy["padding_entries"],
                             copy["fat_entry_capacity"] - 2 - result["selected"]["cluster_count"])
            self.assertTrue(result["fat"]["equal"])

    def test_fat16_free_count_excludes_padding_entries(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "padding.hdd"
            make_image(path, nonzero_padding=True)
            result = MODULE.inspect(path, PHYSICAL)
            copy = result["fat"]["copies"][0]
            self.assertEqual(copy["free_entries"], result["selected"]["cluster_count"])
            self.assertEqual(copy["allocated_entries"], 0)
            self.assertGreater(copy["nonzero_padding_entries"], 0)

    def test_fat16_full_disk_reports_zero_free_clusters(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "full.hdd"
            make_image(path, corrupt_fat=True)
            result = MODULE.inspect(path, PHYSICAL)
            self.assertEqual(result["fat"]["copies"][0]["free_entries"], 0)
            self.assertEqual(result["fat"]["copies"][0]["valid_data_cluster_entries"],
                             result["selected"]["cluster_count"])

    def test_fat16_fat_copies_must_match(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "mismatch.hdd"
            make_image(path, mismatch_fat=True)
            result = MODULE.inspect(path, PHYSICAL)
            self.assertFalse(result["fat"]["equal"])

    def test_fat16_root_directory_has_unused_entry(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "healthy.hdd"
            make_image(path)
            result = MODULE.inspect(path, PHYSICAL)
            self.assertEqual(result["root_directory"]["first_unused_entry"], 0)

    def test_expected_manifest_is_derived_from_bpb(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "healthy.hdd"
            make_image(path)
            result = MODULE.inspect(path, PHYSICAL)
            manifest = result["expected_manifest"]
            self.assertEqual(
                [(item["range_name"], item["first_physical_lba"],
                  item["physical_block_count"], item["byte_count"])
                 for item in manifest],
                [
                    ("boot sector", PARTITION_LBA, 4, 1024),
                    ("FAT1", PARTITION_LBA + 4, FAT_SECTORS * 4,
                     FAT_SECTORS * BYTES_PER_SECTOR),
                    ("FAT2", PARTITION_LBA + 4 + FAT_SECTORS * 4,
                     FAT_SECTORS * 4, FAT_SECTORS * BYTES_PER_SECTOR),
                    ("root directory",
                     PARTITION_LBA + (1 + 2 * FAT_SECTORS) * 4, 4, 1024),
                    ("first data cluster",
                     PARTITION_LBA + (1 + 2 * FAT_SECTORS + 1) * 4, 4, 1024),
                ],
            )
            for item in manifest:
                self.assertEqual(len(item["first_64_bytes"]), 128)
                self.assertEqual(len(item["last_64_bytes"]), 128)
                self.assertEqual(len(item["sha256"]), 64)

    def test_fat16_partition_offset_and_header_offset_are_respected(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "healthy.hdd"
            make_image(path)
            result = MODULE.inspect(path, PHYSICAL)
            self.assertEqual(result["container"]["header_size"], HEADER)
            self.assertEqual(result["selected"]["partition_start_physical_lba"], PARTITION_LBA)

    def test_compare_reports_partial_tail_separately(self):
        with tempfile.TemporaryDirectory() as td:
            first = Path(td) / "first.hdd"
            second = Path(td) / "second.hdd"
            make_image(first)
            make_image(second)
            second.write_bytes(second.read_bytes()[:HEADER + 3 * PHYSICAL + 7])
            result = MODULE.changed_ranges(first, second, PHYSICAL)
            self.assertIsNotNone(result["partial_tail"])
            self.assertTrue(result["partial_tail"]["different"])

    def test_vhd_truncation_is_reported_without_fabricating_blocks(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "truncated.hdd"
            make_image(path)
            original = path.read_bytes()
            path.write_bytes(original[:HEADER + 3 * PHYSICAL + 17])
            info = MODULE.detect_container(path, PHYSICAL)
            self.assertTrue(info["truncated"])
            self.assertEqual(info["complete_physical_blocks"], 3)
            self.assertEqual(info["trailing_data_bytes"], 17)
            result = MODULE.inspect(path, PHYSICAL)
            self.assertEqual(result["candidates"], [])
            self.assertEqual(result["structural_errors"], ["truncated image"])
            forensic = MODULE.inspect(path, PHYSICAL, forensic_partial=True)
            self.assertEqual(len(forensic["candidates"]), 0)

    def test_fat16_compare_reports_changed_physical_lbas(self):
        with tempfile.TemporaryDirectory() as td:
            before = Path(td) / "before.hdd"
            after = Path(td) / "after.hdd"
            make_image(before)
            make_image(after, corrupt_fat=True)
            result = MODULE.changed_ranges(before, after, PHYSICAL)
            self.assertGreater(result["changed_blocks"], 0)
            self.assertTrue(any(r["first_physical_lba"] <= PARTITION_LBA + 4 <= r["last_physical_lba"] for r in result["ranges"]))


if __name__ == "__main__":
    unittest.main()
