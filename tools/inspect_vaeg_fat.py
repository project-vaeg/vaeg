#!/usr/bin/env python3
"""Inspect VAEG VHD/FAT16 media without modifying the image."""
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

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path
from typing import BinaryIO, Dict, Iterable, List, Optional, Tuple

VHD_HEADER_SIZE = 220
VHD_SIGNATURE = b"VHD"


def u16(buf: bytes, off: int) -> int:
    return struct.unpack_from("<H", buf, off)[0]


def u32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<I", buf, off)[0]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def detect_container(path: Path, physical_block_size: int) -> Dict[str, int | str | bool]:
    with path.open("rb") as fh:
        header = fh.read(VHD_HEADER_SIZE)
    if header[:3] == VHD_SIGNATURE and header[:7] == b"VHD1.00":
        sector_size = u16(header, 0x8E)
        totals = u32(header, 0x94)
        if not sector_size or not totals:
            raise ValueError("VHD header has zero sector size or total blocks")
        file_size = path.stat().st_size
        data_bytes = max(0, file_size - VHD_HEADER_SIZE)
        complete_blocks = data_bytes // sector_size
        trailing_bytes = data_bytes % sector_size
        expected_bytes = totals * sector_size
        if data_bytes < expected_bytes:
            classification = "truncated image"
        elif data_bytes > expected_bytes:
            classification = "overlong image"
        elif trailing_bytes:
            classification = "partial final block"
        else:
            classification = "valid complete image"
        return {
            "format": "VHD1.00",
            "header_size": VHD_HEADER_SIZE,
            "file_size": file_size,
            "physical_block_size": sector_size,
            "total_physical_blocks": totals,
            "complete_physical_blocks": complete_blocks,
            "trailing_data_bytes": trailing_bytes,
            "truncated": data_bytes < expected_bytes,
            "overlong": data_bytes > expected_bytes,
            "validation_classification": classification,
            "declared_logical_size": expected_bytes + VHD_HEADER_SIZE,
            "actual_logical_size": file_size,
            "missing_bytes": max(0, expected_bytes - data_bytes),
            "reported_capacity": expected_bytes,
            "mb_size": u16(header, 0x8C),
            "sectors": header[0x90],
            "surfaces": header[0x91],
            "cylinders": u16(header, 0x92),
        }
    file_size = path.stat().st_size
    complete_blocks, trailing_bytes = divmod(file_size, physical_block_size)
    return {
        "format": "headerless",
        "header_size": 0,
        "file_size": file_size,
        "physical_block_size": physical_block_size,
        "total_physical_blocks": complete_blocks,
        "complete_physical_blocks": complete_blocks,
        "trailing_data_bytes": trailing_bytes,
        "truncated": False,
        "overlong": False,
        "validation_classification": "valid complete image",
        "declared_logical_size": file_size,
        "actual_logical_size": file_size,
        "missing_bytes": 0,
        "reported_capacity": file_size,
    }


def read_physical(fh: BinaryIO, info: Dict[str, int | str | bool], lba: int, count: int = 1) -> bytes:
    size = int(info["physical_block_size"])
    fh.seek(int(info["header_size"]) + lba * size)
    return fh.read(count * size)


def logical_sector(fh: BinaryIO, info: Dict[str, int | str | bool], physical_lba: int, logical_size: int) -> bytes:
    psize = int(info["physical_block_size"])
    if logical_size % psize:
        return b""
    return read_physical(fh, info, physical_lba, logical_size // psize)


def parse_bpb(sector: bytes, partition_lba: int, info: Dict[str, int | str | bool]) -> Optional[Dict[str, object]]:
    if len(sector) < 36:
        return None
    bps = u16(sector, 11)
    spc = sector[13]
    reserved = u16(sector, 14)
    fats = sector[16]
    root_entries = u16(sector, 17)
    total16 = u16(sector, 19)
    media = sector[21]
    fatsz = u16(sector, 22)
    sectors_per_track = u16(sector, 24)
    heads = u16(sector, 26)
    hidden = u32(sector, 28) if len(sector) >= 32 else 0
    total32 = u32(sector, 32) if len(sector) >= 36 else 0
    errors: List[str] = []
    if bps not in (256, 512, 1024, 2048, 4096) or (bps & (bps - 1)):
        errors.append("unsupported bytes-per-sector")
    psize = int(info["physical_block_size"])
    if bps == 0 or bps % psize:
        errors.append("logical sector is not an integral physical-block count")
    if spc == 0 or (spc & (spc - 1)):
        errors.append("sectors-per-cluster is not a nonzero power of two")
    if reserved == 0:
        errors.append("reserved-sector count is zero")
    if fats == 0:
        errors.append("FAT count is zero")
    if fatsz == 0:
        errors.append("FAT16 size is zero")
    total = total16 or total32
    root_sectors = math.ceil(root_entries * 32 / bps) if bps else 0
    first_fat = reserved
    first_root = reserved + fats * fatsz
    first_data = first_root + root_sectors
    data_sectors = total - first_data if total >= first_data else -1
    clusters = data_sectors // spc if data_sectors >= 0 and spc else -1
    fat_type = "FAT12" if 1 <= clusters < 4085 else "FAT16" if 4085 <= clusters < 65525 else "FAT32/invalid"
    physical_total = int(info["total_physical_blocks"])
    blocks_per_logical = bps // psize if bps and bps % psize == 0 else 0
    end_lba = partition_lba + total * blocks_per_logical if blocks_per_logical else 0
    if end_lba > physical_total:
        errors.append("calculated partition exceeds image")
    if total == 0:
        errors.append("total sectors is zero")
    return {
        "partition_start_physical_lba": partition_lba,
        "bytes_per_sector": bps,
        "sectors_per_cluster": spc,
        "reserved_sectors": reserved,
        "number_of_fats": fats,
        "root_directory_entries": root_entries,
        "total_sectors_16": total16,
        "media": media,
        "sectors_per_fat": fatsz,
        "sectors_per_track": sectors_per_track,
        "number_of_heads": heads,
        "hidden_sectors": hidden,
        "total_sectors_32": total32,
        "total_sectors": total,
        "root_dir_sectors": root_sectors,
        "first_fat_sector": first_fat,
        "first_root_dir_sector": first_root,
        "first_data_sector": first_data,
        "data_sectors": data_sectors,
        "cluster_count": clusters,
        "fat_type": fat_type,
        "structural_errors": errors,
        "valid": not errors and fat_type == "FAT16",
        "blocks_per_logical_sector": blocks_per_logical,
    }


def find_candidates(path: Path, info: Dict[str, int | str | bool]) -> List[Dict[str, object]]:
    psize = int(info["physical_block_size"])
    max_lba = int(info.get("complete_physical_blocks", info["total_physical_blocks"]))
    candidates: List[Dict[str, object]] = []
    with path.open("rb") as fh:
        for lba in range(max_lba):
            raw = read_physical(fh, info, lba)
            if len(raw) < psize:
                break
            # Read a short prefix first; reject impossible BPBs before reading
            # a full logical sector.
            if len(raw) < 36:
                continue
            bps = u16(raw, 11)
            if bps == 0 or bps % psize:
                continue
            sector = logical_sector(fh, info, lba, bps)
            bpb = parse_bpb(sector, lba, info)
            if bpb is None:
                continue
            # Candidate must have FAT16 geometry and a plausible media byte.
            if bpb["fat_type"] == "FAT16" and bpb["media"] in (0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF):
                bpb["boot_sector_sha256"] = sha256(sector)
                bpb["boot_sector_first_32"] = sector[:32].hex()
                candidates.append(bpb)
    return candidates


def expected_manifest(path: Path, info: Dict[str, int | str | bool],
                     bpb: Dict[str, object]) -> List[Dict[str, object]]:
    physical_size = int(info["physical_block_size"])
    blocks_per_logical = int(bpb["blocks_per_logical_sector"])
    partition_start = int(bpb["partition_start_physical_lba"])
    fat_size = int(bpb["sectors_per_fat"]) * blocks_per_logical
    root_size = int(bpb["root_dir_sectors"]) * blocks_per_logical
    data_start = partition_start + int(bpb["first_data_sector"]) * blocks_per_logical
    ranges = [
        ("boot sector", partition_start, blocks_per_logical),
        ("FAT1", partition_start + int(bpb["first_fat_sector"]) * blocks_per_logical, fat_size),
        ("FAT2", partition_start + (int(bpb["first_fat_sector"]) +
                                    int(bpb["sectors_per_fat"])) * blocks_per_logical, fat_size),
        ("root directory", partition_start + int(bpb["first_root_dir_sector"]) * blocks_per_logical, root_size),
        ("first data cluster", data_start, int(bpb["sectors_per_cluster"]) * blocks_per_logical),
    ]
    manifest = []
    with path.open("rb") as fh:
        for name, first_lba, block_count in ranges:
            data = read_physical(fh, info, first_lba, block_count)
            expected_bytes = block_count * physical_size
            if len(data) != expected_bytes:
                raise ValueError(f"{name} range is incomplete")
            manifest.append({
                "range_name": name,
                "first_physical_lba": first_lba,
                "physical_block_count": block_count,
                "byte_count": len(data),
                "sha256": sha256(data),
                "first_64_bytes": data[:64].hex(),
                "last_64_bytes": data[-64:].hex(),
            })
    return manifest


def fat_report(path: Path, info: Dict[str, int | str | bool], bpb: Dict[str, object]) -> Dict[str, object]:
    psize = int(info["physical_block_size"])
    start = int(bpb["partition_start_physical_lba"])
    bps = int(bpb["bytes_per_sector"])
    bpl = int(bpb["blocks_per_logical_sector"])
    fatsz = int(bpb["sectors_per_fat"])
    fat_bytes = fatsz * bps
    reports = []
    with path.open("rb") as fh:
        valid_cluster_count = int(bpb["cluster_count"])
        for copy_index in range(int(bpb["number_of_fats"])):
            lba = start + (int(bpb["first_fat_sector"]) + copy_index * fatsz) * bpl
            data = read_physical(fh, info, lba, fat_bytes // psize)
            entries = [u16(data, off) for off in range(0, len(data) - 1, 2)]
            from_cluster2 = entries[2:]
            valid_entries = from_cluster2[:valid_cluster_count]
            padding_entries = from_cluster2[valid_cluster_count:]
            reports.append({
                "index": copy_index + 1,
                "physical_lba_start": lba,
                "physical_lba_end": lba + len(data) // psize - 1,
                "sha256": sha256(data),
                "first_64_bytes": data[:64].hex(),
                "last_64_bytes": data[-64:].hex(),
                "first_32_entries": [f"{x:04x}" for x in entries[:32]],
                "fat_entry_capacity": len(entries),
                "valid_data_cluster_entries": len(valid_entries),
                "padding_entries": len(padding_entries),
                "free_entries": sum(x == 0 for x in valid_entries),
                "allocated_entries": sum(x not in (0, 0xFFF7) and x < 0xFFF8 for x in valid_entries),
                "bad_clusters": sum(x == 0xFFF7 for x in valid_entries),
                "end_of_chain_entries": sum(0xFFF8 <= x <= 0xFFFF for x in valid_entries),
                "reserved_entries": sum(x != 0 and (x == 0xfff7 or x >= 0xfff8) for x in entries[:2]),
                "nonzero_padding_entries": sum(x != 0 for x in padding_entries),
                "entry_count": len(valid_entries),
            })
    return {"copies": reports, "equal": len(reports) <= 1 or len({r["sha256"] for r in reports}) == 1}


def root_report(path: Path, info: Dict[str, int | str | bool], bpb: Dict[str, object]) -> Dict[str, object]:
    psize = int(info["physical_block_size"])
    bps = int(bpb["bytes_per_sector"])
    bpl = int(bpb["blocks_per_logical_sector"])
    start = int(bpb["partition_start_physical_lba"])
    count = int(bpb["root_dir_sectors"]) * bpl
    lba = start + int(bpb["first_root_dir_sector"]) * bpl
    with path.open("rb") as fh:
        data = read_physical(fh, info, lba, count)
    occupied = deleted = 0
    first_unused = None
    labels = []
    for i in range(0, len(data), 32):
        ent = data[i:i + 32]
        if len(ent) < 32:
            break
        if ent[0] == 0 and first_unused is None:
            first_unused = i // 32
        elif ent[0] == 0xE5:
            deleted += 1
        elif ent[0] != 0:
            occupied += 1
            if ent[11] == 0x08:
                labels.append(ent[:11].decode("ascii", "replace"))
    return {
        "physical_lba_start": lba,
        "physical_lba_end": lba + count - 1,
        "sha256": sha256(data),
        "first_128_bytes": data[:128].hex(),
        "occupied_entries": occupied,
        "deleted_entries": deleted,
        "first_unused_entry": first_unused,
        "volume_labels": labels,
    }


def inspect(path: Path, physical_block_size: int, forensic_partial: bool = False) -> Dict[str, object]:
    info = detect_container(path, physical_block_size)
    complete = info.get("validation_classification") == "valid complete image"
    candidates = find_candidates(path, info) if (complete or forensic_partial) else []
    result: Dict[str, object] = {"image": str(path), "container": info, "candidates": candidates, "structural_errors": []}
    if not complete and not forensic_partial:
        result["structural_errors"] = [str(info["validation_classification"])]
        return result
    valid = [c for c in candidates if c["valid"]]
    if len(valid) == 1:
        result["selected"] = valid[0]
        result["fat"] = fat_report(path, info, valid[0])
        result["root_directory"] = root_report(path, info, valid[0])
        result["expected_manifest"] = expected_manifest(path, info, valid[0])
    elif len(valid) > 1:
        result["structural_errors"] = ["multiple valid FAT candidates; selection is ambiguous"]
    else:
        result["structural_errors"] = ["no unique structurally valid FAT16 BPB candidate"]
    return result


def changed_ranges(first: Path, second: Path, physical_block_size: int) -> Dict[str, object]:
    a = detect_container(first, physical_block_size)
    b = detect_container(second, physical_block_size)
    if a["physical_block_size"] != b["physical_block_size"]:
        raise ValueError("images use different physical block sizes")
    size = int(a["physical_block_size"])
    first_blocks = int(a.get("complete_physical_blocks", a["total_physical_blocks"]))
    second_blocks = int(b.get("complete_physical_blocks", b["total_physical_blocks"]))
    # Compare all blocks present in either file.  A truncated image therefore
    # reports the physical range that exists only on the other side instead
    # of silently treating the missing tail as zero-filled media.
    count = max(first_blocks, second_blocks)
    ranges: List[Dict[str, object]] = []
    start = None
    with first.open("rb") as af, second.open("rb") as bf:
        for lba in range(count):
            x = read_physical(af, a, lba)
            y = read_physical(bf, b, lba)
            different = x != y
            if different and start is None:
                start = lba
            if not different and start is not None:
                ranges.append(_range_report(af, a, bf, b, start, lba - 1, size))
                start = None
        if start is not None:
            ranges.append(_range_report(af, a, bf, b, start, count - 1, size))
    def tail_bytes(path: Path, info: Dict[str, object], complete: int) -> bytes:
        with path.open("rb") as fh:
            fh.seek(int(info["header_size"]) + complete * size)
            return fh.read()

    first_tail = tail_bytes(first, a, first_blocks)
    second_tail = tail_bytes(second, b, second_blocks)
    partial_tail: Optional[Dict[str, object]] = None
    if first_tail or second_tail:
        partial_tail = {
            "physical_lba": max(first_blocks, second_blocks),
            "first_bytes": len(first_tail),
            "second_bytes": len(second_tail),
            "first_sha256": sha256(first_tail),
            "second_sha256": sha256(second_tail),
            "different": first_tail != second_tail,
        }

    return {
        "first": str(first),
        "second": str(second),
        "compared_blocks": count,
        "first_complete_blocks": first_blocks,
        "second_complete_blocks": second_blocks,
        "changed_blocks": sum(r["block_count"] for r in ranges),
        "ranges": ranges,
        "partial_tail": partial_tail,
        "headers_equal": _read_header(first) == _read_header(second),
        "first_truncated": bool(a.get("truncated", False)),
        "second_truncated": bool(b.get("truncated", False)),
    }


def _read_header(path: Path) -> bytes:
    with path.open("rb") as fh:
        return fh.read(VHD_HEADER_SIZE)


def _range_report(af: BinaryIO, ai: Dict[str, object], bf: BinaryIO, bi: Dict[str, object], start: int, end: int, size: int) -> Dict[str, object]:
    x = read_physical(af, ai, start, end - start + 1)
    y = read_physical(bf, bi, start, end - start + 1)
    return {"first_physical_lba": start, "last_physical_lba": end, "block_count": end - start + 1, "first_32_bytes_first": x[:32].hex(), "first_32_bytes_second": y[:32].hex(), "sha256_first": sha256(x), "sha256_second": sha256(y)}


def main(argv: Optional[Iterable[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, type=Path)
    parser.add_argument("--physical-block-size", type=int, default=256)
    parser.add_argument("--compare", type=Path)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--forensic-partial", action="store_true",
                        help="inspect FAT candidates despite incomplete backing storage")
    args = parser.parse_args(argv)
    try:
        result = inspect(args.image, args.physical_block_size, args.forensic_partial)
        if args.compare:
            result["compare"] = changed_ranges(args.image, args.compare, args.physical_block_size)
    except (OSError, ValueError, struct.error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        c = result["container"]
        print(f"image header size: {c['header_size']}")
        print(f"file size: {c['file_size']}")
        print(f"declared logical size: {c['declared_logical_size']}")
        print(f"actual logical size: {c['actual_logical_size']}")
        print(f"missing bytes: {c['missing_bytes']}")
        print(f"physical block size: {c['physical_block_size']}")
        print(f"complete physical blocks: {c['complete_physical_blocks']}")
        print(f"header physical blocks: {c['total_physical_blocks']}")
        print(f"truncated: {c['truncated']}")
        print(f"validation classification: {c['validation_classification']}")
        print(f"partial-tail bytes: {c['trailing_data_bytes']}")
        print(f"FAT candidates: {len(result['candidates'])}")
        if "selected" in result:
            s = result["selected"]
            print(f"partition start: physical LBA {s['partition_start_physical_lba']}")
            print(f"BPB: bytes/sector={s['bytes_per_sector']} sectors/cluster={s['sectors_per_cluster']} reserved={s['reserved_sectors']} FATs={s['number_of_fats']} FAT16-size={s['sectors_per_fat']}")
            print(f"FAT type: {s['fat_type']} clusters={s['cluster_count']}")
            fat = result["fat"]
            print(f"FAT1/FAT2 equal: {fat['equal']}")
            copy = fat["copies"][0]
            print(f"FAT entry capacity: {copy['fat_entry_capacity']}")
            print(f"valid data-cluster entries: {copy['valid_data_cluster_entries']}")
            print(f"padding entries: {copy['padding_entries']}")
            print(f"free valid clusters: {copy['free_entries']}")
            print(f"allocated valid clusters: {copy['allocated_entries']}")
            print(f"bad valid clusters: {copy['bad_clusters']}")
            print(f"reserved valid entries: {copy['reserved_entries']}")
            print(f"nonzero padding entries: {copy['nonzero_padding_entries']}")
            print(f"root first unused entry: {result['root_directory']['first_unused_entry']}")
            for item in result["expected_manifest"]:
                print(f"expected range: {item['range_name']} LBA {item['first_physical_lba']} blocks={item['physical_block_count']} bytes={item['byte_count']} sha256={item['sha256']}")
        for error in result.get("structural_errors", []):
            print(f"structural error: {error}")
        if "compare" in result:
            comp = result["compare"]
            print(f"compared physical blocks: {comp['compared_blocks']}")
            print(f"changed physical blocks: {comp['changed_blocks']}")
            print(f"first truncated: {comp['first_truncated']}; second truncated: {comp['second_truncated']}")
            for r in comp["ranges"]:
                print(f"changed range: {r['first_physical_lba']}-{r['last_physical_lba']} ({r['block_count']} blocks)")
            if comp.get("partial_tail") and comp["partial_tail"]["different"]:
                tail = comp["partial_tail"]
                print(f"changed trailing bytes at physical LBA {tail['physical_lba']}: {tail['first_bytes']} vs {tail['second_bytes']} bytes")
    return 0 if "selected" in result and not result.get("structural_errors") else 1


if __name__ == "__main__":
    raise SystemExit(main())
