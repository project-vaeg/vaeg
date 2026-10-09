#!/usr/bin/env python3
#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
from datetime import datetime
import os
import re
import struct
import tempfile
from pathlib import Path


SECTOR_SIZE = 1024
DATA_START_LBA = 11
DATA_CLUSTER_COUNT = 1269
LAST_DATA_CLUSTER = DATA_CLUSTER_COUNT + 1
ROOT_START_LBA = 5
ROOT_SECTORS = 6
SYSTEM_LAYOUTS = {
    "1.05": {
        "ENGINEIO.SYS": (2, 4096),
        "PCENGINE.SYS": (6, 52090),
        "ADVGBIOS.SYS": (58, 30956),
        "PCENGINE.COM": (57, 5),
    },
    "1.1": {
        "ENGINEIO.SYS": (2, 4096),
        "PCENGINE.SYS": (6, 62347),
        "ADVGBIOS.SYS": (67, 16364),
        "PCENGINE.COM": (83, 5),
    },
}
SYSTEM_FILES = tuple(next(iter(SYSTEM_LAYOUTS.values())))


class DiskError(Exception):
    pass


def short_name(name):
    upper = name.upper()
    if upper.count(".") > 1:
        raise DiskError(f"name is not 8.3: {name}")
    base, _, extension = upper.partition(".")
    if not base or len(base) > 8 or len(extension) > 3:
        raise DiskError(f"name is not 8.3: {name}")
    valid = re.compile(r"^[A-Z0-9_$~!#%&'()@^`{}-]+$")
    if not valid.fullmatch(base) or (extension and not valid.fullmatch(extension)):
        raise DiskError(f"name uses unsupported FAT characters: {name}")
    return base.ljust(8).encode("ascii") + extension.ljust(3).encode("ascii")


def display_name(raw_name):
    base = raw_name[:8].decode("ascii").rstrip()
    extension = raw_name[8:11].decode("ascii").rstrip()
    return base + (f".{extension}" if extension else "")


def special_directory_name(name):
    return name.encode("ascii").ljust(11, b" ")


def iter_entries(directory):
    for offset in range(0, len(directory), 32):
        entry = bytes(directory[offset:offset + 32])
        if entry[0] == 0:
            break
        if entry[0] == 0xE5:
            continue
        yield offset, entry


def find_entry(directory, raw_name):
    first_free = None
    for offset in range(0, len(directory), 32):
        first = directory[offset]
        if first in (0x00, 0xE5) and first_free is None:
            first_free = offset
        if first == 0x00:
            break
        if first != 0xE5 and bytes(directory[offset:offset + 11]) == raw_name:
            return offset, True
    if first_free is None:
        raise DiskError("directory is full")
    return first_free, False


def fat_now():
    """Return the current local time encoded as FAT time and date words."""
    now = datetime.now()
    year = min(max(now.year, 1980), 2107)
    fat_date = ((year - 1980) << 9) | (now.month << 5) | now.day
    fat_time = (now.hour << 11) | (now.minute << 5) | (now.second // 2)
    return fat_time, fat_date


def make_entry(raw_name, attributes, first_cluster, size):
    fat_time, fat_date = fat_now()
    entry = bytearray(32)
    entry[:11] = raw_name
    entry[11] = attributes
    struct.pack_into("<H", entry, 14, fat_time)
    struct.pack_into("<H", entry, 16, fat_date)
    struct.pack_into("<H", entry, 18, fat_date)
    struct.pack_into("<H", entry, 22, fat_time)
    struct.pack_into("<H", entry, 24, fat_date)
    struct.pack_into("<H", entry, 26, first_cluster)
    struct.pack_into("<I", entry, 28, size)
    return entry


class PcEngineDisk:
    def __init__(self, image, require_system_files=True):
        self.image = bytearray(image)
        if len(self.image) < 0x2B0:
            raise DiskError("source is too small to be a D88 image")
        if self.image[0x1B] != 0x20:
            raise DiskError("source is not a 2HD D88 image")
        if struct.unpack_from("<I", self.image, 0x1C)[0] != len(self.image):
            raise DiskError("D88 header size does not match the file size")
        if self.image[0x1A] != 0:
            raise DiskError("source D88 is write protected")

        self.sectors = self._parse_sectors()
        self.normal_sectors = []
        for cylinder in range(80):
            for head in range(2):
                for record in range(1, 9):
                    key = (cylinder, head, record)
                    if key not in self.sectors:
                        raise DiskError(
                            f"missing normal-area sector C={cylinder} H={head} R={record}"
                        )
                    sector = self.sectors[key]
                    if sector[1:] != (SECTOR_SIZE, 3, 0):
                        raise DiskError(
                            "unexpected normal-area sector form at "
                            f"C={cylinder} H={head} R={record}"
                        )
                    self.normal_sectors.append(sector)

        self.boot_sector = bytes(self.read_lbas(0, 1))
        self.fat = self.read_lbas(1, 2)
        fat_copy = self.read_lbas(3, 2)
        self.root = self.read_lbas(ROOT_START_LBA, ROOT_SECTORS)
        if self.fat != fat_copy:
            raise DiskError("the two source FAT12 copies differ")
        if self.fat[:3] != b"\xfe\xff\xff":
            raise DiskError("source does not have the expected PC-Engine FAT12 header")
        self.system_version = None
        if require_system_files:
            self.system_version = self.validate_system_files()

    def _parse_sectors(self):
        track_offsets = struct.unpack_from("<164I", self.image, 0x20)
        sectors = {}
        for track_index, track_offset in enumerate(track_offsets):
            if track_offset == 0:
                continue
            if track_offset + 16 > len(self.image):
                raise DiskError(f"invalid D88 track offset at index {track_index}")
            sector_count = struct.unpack_from("<H", self.image, track_offset + 4)[0]
            position = track_offset
            for _ in range(sector_count):
                if position + 16 > len(self.image):
                    raise DiskError(f"truncated sector header at track {track_index}")
                cylinder, head, record, size_code = struct.unpack_from(
                    "<BBBB", self.image, position
                )
                status = self.image[position + 8]
                stored_size = struct.unpack_from("<H", self.image, position + 14)[0]
                data_offset = position + 16
                if data_offset + stored_size > len(self.image):
                    raise DiskError(f"truncated sector data at track {track_index}")
                key = (cylinder, head, record)
                if key in sectors:
                    raise DiskError(
                        f"duplicate D88 sector C={cylinder} H={head} R={record}"
                    )
                sectors[key] = (data_offset, stored_size, size_code, status)
                position = data_offset + stored_size
        return sectors

    def read_lbas(self, start, count):
        result = bytearray()
        for lba in range(start, start + count):
            data_offset = self.normal_sectors[lba][0]
            result.extend(self.image[data_offset:data_offset + SECTOR_SIZE])
        return result

    def write_lbas(self, start, data):
        if len(data) % SECTOR_SIZE:
            raise DiskError("internal non-sector-aligned write")
        for index in range(len(data) // SECTOR_SIZE):
            data_offset = self.normal_sectors[start + index][0]
            begin = index * SECTOR_SIZE
            self.image[data_offset:data_offset + SECTOR_SIZE] = data[
                begin:begin + SECTOR_SIZE
            ]

    def read_cluster(self, cluster):
        self.validate_cluster(cluster)
        return self.read_lbas(DATA_START_LBA + cluster - 2, 1)

    def write_cluster(self, cluster, data):
        self.validate_cluster(cluster)
        if len(data) != SECTOR_SIZE:
            raise DiskError("internal non-cluster-sized write")
        self.write_lbas(DATA_START_LBA + cluster - 2, data)

    @staticmethod
    def validate_cluster(cluster):
        if not 2 <= cluster <= LAST_DATA_CLUSTER:
            raise DiskError(f"cluster outside data area: {cluster}")

    def fat_get(self, cluster, fat=None):
        source = self.fat if fat is None else fat
        offset = cluster + cluster // 2
        pair = source[offset] | (source[offset + 1] << 8)
        if cluster & 1:
            pair >>= 4
        return pair & 0xFFF

    def fat_set(self, cluster, value, fat=None):
        target = self.fat if fat is None else fat
        offset = cluster + cluster // 2
        value &= 0xFFF
        if cluster & 1:
            target[offset] = (target[offset] & 0x0F) | ((value & 0x0F) << 4)
            target[offset + 1] = (value >> 4) & 0xFF
        else:
            target[offset] = value & 0xFF
            target[offset + 1] = (target[offset + 1] & 0xF0) | (
                (value >> 8) & 0x0F
            )

    def cluster_chain(self, first_cluster, fat=None):
        cluster = first_cluster
        visited = set()
        chain = []
        while 2 <= cluster <= LAST_DATA_CLUSTER:
            if cluster in visited:
                raise DiskError("loop in FAT12 chain")
            visited.add(cluster)
            chain.append(cluster)
            following = self.fat_get(cluster, fat)
            if following >= 0xFF8:
                return chain
            if following == 0:
                raise DiskError("unterminated FAT12 chain")
            cluster = following
        raise DiskError("FAT12 chain points outside the data area")

    def release_chain(self, first_cluster):
        for cluster in self.cluster_chain(first_cluster):
            self.fat_set(cluster, 0)

    def allocate_clusters(self, count):
        free_clusters = [
            cluster
            for cluster in range(2, LAST_DATA_CLUSTER + 1)
            if self.fat_get(cluster) == 0
        ][:count]
        if len(free_clusters) != count:
            raise DiskError("not enough free space")
        for index, cluster in enumerate(free_clusters):
            following = 0xFFF if index + 1 == count else free_clusters[index + 1]
            self.fat_set(cluster, following)
        return free_clusters

    def validate_system_files(self):
        observed = {}
        for name in SYSTEM_FILES:
            offset, exists = find_entry(self.root, short_name(name))
            if not exists:
                raise DiskError(
                    f"source is not a supported PC-Engine disk: missing {name}"
                )
            cluster = struct.unpack_from("<H", self.root, offset + 26)[0]
            size = struct.unpack_from("<I", self.root, offset + 28)[0]
            observed[name] = (cluster, size)
        for version, expected in SYSTEM_LAYOUTS.items():
            if observed == expected:
                return version
        raise DiskError("unsupported PC-Engine 1.05/1.1 system-file layout")

    def flush(self):
        self.write_lbas(1, self.fat)
        self.write_lbas(3, self.fat)
        self.write_lbas(ROOT_START_LBA, self.root)
        if bytes(self.read_lbas(0, 1)) != self.boot_sector:
            raise DiskError("internal error: boot sector changed")

    def free_bytes(self):
        free_clusters = sum(
            self.fat_get(cluster) == 0
            for cluster in range(2, LAST_DATA_CLUSTER + 1)
        )
        return free_clusters * SECTOR_SIZE


def write_new_file(path, contents):
    destination = Path(path)
    if destination.exists():
        raise DiskError("output already exists; refusing to overwrite it")
    parent = destination.parent
    if not parent.is_dir():
        raise DiskError("output directory does not exist")
    temporary_name = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=parent, prefix=f"{destination.name}.tmp.", delete=False
        ) as temporary:
            temporary_name = temporary.name
            temporary.write(contents)
        os.chmod(temporary_name, 0o644)
        os.replace(temporary_name, destination)
    finally:
        if temporary_name and os.path.exists(temporary_name):
            os.unlink(temporary_name)


def create_vanilla(source, output):
    disk = PcEngineDisk(Path(source).read_bytes())
    source_fat = bytes(disk.fat)
    new_fat = bytearray(len(disk.fat))
    new_fat[:3] = disk.fat[:3]
    new_root = bytearray(len(disk.root))
    preserved_clusters = set()

    root_offset = 0
    for name in SYSTEM_FILES:
        source_offset, _ = find_entry(disk.root, short_name(name))
        entry = bytes(disk.root[source_offset:source_offset + 32])
        new_root[root_offset:root_offset + 32] = entry
        root_offset += 32
        first_cluster = struct.unpack_from("<H", entry, 26)[0]
        chain = disk.cluster_chain(first_cluster, source_fat)
        preserved_clusters.update(chain)
        for cluster in chain:
            disk.fat_set(cluster, disk.fat_get(cluster, source_fat), new_fat)

    disk.fat = new_fat
    disk.root = new_root
    zero_cluster = bytes(SECTOR_SIZE)
    for cluster in range(2, LAST_DATA_CLUSTER + 1):
        if cluster not in preserved_clusters:
            disk.write_cluster(cluster, zero_cluster)
    disk.flush()
    write_new_file(output, disk.image)
    print(f"Created vanilla PC-Engine {disk.system_version} system disk: {output}")
    print(f"Remaining FAT12 space: {disk.free_bytes()} bytes")


def create_data_disk(source, output):
    disk = PcEngineDisk(Path(source).read_bytes())
    new_fat = bytearray(len(disk.fat))
    new_fat[:3] = disk.fat[:3]
    disk.fat = new_fat
    disk.root = bytearray(len(disk.root))
    zero_cluster = bytes(SECTOR_SIZE)
    for cluster in range(2, LAST_DATA_CLUSTER + 1):
        disk.write_cluster(cluster, zero_cluster)
    disk.flush()
    write_new_file(output, disk.image)
    print(f"Created empty PC-Engine data disk: {output}")
    print(f"Remaining FAT12 space: {disk.free_bytes()} bytes")


def add_file(disk, directory, payload):
    raw_name = short_name(payload.name)
    offset, exists = find_entry(directory, raw_name)
    if exists:
        attributes = directory[offset + 11]
        if attributes & 0x18:
            raise DiskError(f"refusing to replace directory or label: {payload.name}")
        old_cluster = struct.unpack_from("<H", directory, offset + 26)[0]
        if old_cluster:
            disk.release_chain(old_cluster)

    contents = payload.read_bytes()
    count = (len(contents) + SECTOR_SIZE - 1) // SECTOR_SIZE
    clusters = disk.allocate_clusters(count)
    for index, cluster in enumerate(clusters):
        begin = index * SECTOR_SIZE
        chunk = contents[begin:begin + SECTOR_SIZE]
        disk.write_cluster(cluster, chunk.ljust(SECTOR_SIZE, b"\x00"))
    directory[offset:offset + 32] = make_entry(
        raw_name, 0x20, clusters[0] if clusters else 0, len(contents)
    )
    return len(contents)


def create_directory(disk, parent_directory, name, additional_entries):
    raw_name = short_name(name)
    offset, exists = find_entry(parent_directory, raw_name)
    if exists:
        if not parent_directory[offset + 11] & 0x10:
            raise DiskError(f"payload entry is not a directory: {name}")
        cluster = struct.unpack_from("<H", parent_directory, offset + 26)[0]
        chain = disk.cluster_chain(cluster)
        contents = bytearray()
        for item in chain:
            contents.extend(disk.read_cluster(item))
        used_entries = sum(1 for _ in iter_entries(contents))
        required_entries = used_entries + additional_entries
    else:
        required_entries = 2 + additional_entries
        required_clusters = max(
            1, (required_entries * 32 + SECTOR_SIZE - 1) // SECTOR_SIZE
        )
        chain = disk.allocate_clusters(required_clusters)
        contents = bytearray(required_clusters * SECTOR_SIZE)
        contents[0:32] = make_entry(
            special_directory_name("."), 0x10, chain[0], 0
        )
        contents[32:64] = make_entry(special_directory_name(".."), 0x10, 0, 0)
        parent_directory[offset:offset + 32] = make_entry(
            raw_name, 0x10, chain[0], 0
        )
        return chain, contents

    required_clusters = max(
        1, (required_entries * 32 + SECTOR_SIZE - 1) // SECTOR_SIZE
    )
    if required_clusters > len(chain):
        added = disk.allocate_clusters(required_clusters - len(chain))
        disk.fat_set(chain[-1], added[0])
        chain.extend(added)
        contents.extend(bytes((required_clusters * SECTOR_SIZE) - len(contents)))
    return chain, contents


def create_root_directory(disk, name, additional_entries):
    return create_directory(disk, disk.root, name, additional_entries)


def write_directory_chain(disk, chain, contents):
    for index, cluster in enumerate(chain):
        begin = index * SECTOR_SIZE
        disk.write_cluster(cluster, contents[begin:begin + SECTOR_SIZE])


def install_directory(disk, parent_directory, directory_path):
    items = sorted(directory_path.iterdir(), key=lambda path: path.name)
    if any(not item.is_file() and not item.is_dir() for item in items):
        raise DiskError(f"unexpected directory payload entry: {directory_path}")

    chain, directory = create_directory(
        disk, parent_directory, directory_path.name, len(items)
    )
    installed_files = 0
    installed_bytes = 0
    for item in items:
        if item.is_file():
            installed_bytes += add_file(disk, directory, item)
            installed_files += 1
        else:
            child_files, child_bytes = install_directory(disk, directory, item)
            installed_files += child_files
            installed_bytes += child_bytes
    write_directory_chain(disk, chain, directory)
    return installed_files, installed_bytes


def install_payload(image, payload_root):
    image_path = Path(image)
    disk = PcEngineDisk(image_path.read_bytes(), require_system_files=False)
    payload = Path(payload_root)
    if not payload.is_dir():
        raise DiskError("payload directory does not exist")

    allowed_root_names = set(SYSTEM_FILES)
    existing_names = {
        display_name(entry[:11])
        for _, entry in iter_entries(disk.root)
        if not entry[11] & 0x08
    }
    if existing_names not in (set(), allowed_root_names):
        raise DiskError(
            "install input is neither an empty data disk nor a vanilla "
            "PC-Engine 1.05/1.1 system disk"
        )

    installed_files = 0
    installed_bytes = 0
    root_payload = payload / "root"
    if root_payload.is_dir():
        for item in sorted(root_payload.iterdir(), key=lambda path: path.name):
            if not item.is_file():
                raise DiskError(f"unexpected root payload entry: {item.name}")
            installed_bytes += add_file(disk, disk.root, item)
            installed_files += 1

    for directory_path in sorted(
        (path for path in payload.iterdir() if path.is_dir() and path.name != "root"),
        key=lambda path: path.name,
    ):
        child_files, child_bytes = install_directory(
            disk, disk.root, directory_path
        )
        installed_files += child_files
        installed_bytes += child_bytes

    disk.flush()
    image_path.write_bytes(disk.image)
    print(f"Installed {installed_files} files ({installed_bytes} bytes)")
    print(f"Remaining FAT12 space: {disk.free_bytes()} bytes")


def list_directory(disk, directory, prefix, visited):
    for _, entry in iter_entries(directory):
        if entry[0] == ord(".") or entry[11] & 0x08:
            continue
        name = display_name(entry[:11])
        size = struct.unpack_from("<I", entry, 28)[0]
        cluster = struct.unpack_from("<H", entry, 26)[0]
        if entry[11] & 0x10:
            print(f"{prefix}{name}\\")
            if cluster in visited:
                raise DiskError("directory loop")
            visited.add(cluster)
            chain = disk.cluster_chain(cluster)
            contents = bytearray()
            for item in chain:
                contents.extend(disk.read_cluster(item))
            list_directory(disk, contents, f"{prefix}{name}\\", visited)
        else:
            print(f"{prefix}{name} {size}")


def list_image(image):
    disk = PcEngineDisk(Path(image).read_bytes(), require_system_files=False)
    list_directory(disk, disk.root, "A:\\", set())
    print(f"Free bytes: {disk.free_bytes()}")


def main():
    parser = argparse.ArgumentParser(
        description="Manipulate the known PC-Engine 1.05/1.1 D88/FAT12 layout"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    vanilla_parser = subparsers.add_parser("vanilla")
    vanilla_parser.add_argument("--source", required=True)
    vanilla_parser.add_argument("--output", required=True)

    data_parser = subparsers.add_parser("data")
    data_parser.add_argument("--source", required=True)
    data_parser.add_argument("--output", required=True)

    install_parser = subparsers.add_parser("install")
    install_parser.add_argument("--image", required=True)
    install_parser.add_argument("--payload", required=True)

    list_parser = subparsers.add_parser("list")
    list_parser.add_argument("--image", required=True)

    args = parser.parse_args()
    try:
        if args.command == "vanilla":
            create_vanilla(args.source, args.output)
        elif args.command == "data":
            create_data_disk(args.source, args.output)
        elif args.command == "install":
            install_payload(args.image, args.payload)
        else:
            list_image(args.image)
    except (DiskError, OSError) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
