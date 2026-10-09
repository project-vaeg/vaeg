#!/usr/bin/env python3
"""Inspect vaeg statsave containers without importing emulator state types."""
import argparse, hashlib, json, struct

HEADER_SIZE = 48
SECTION_SIZE = 16

def inspect(path):
    data = open(path, "rb").read()
    if len(data) < HEADER_SIZE:
        raise ValueError("truncated container header")
    sections = []
    pos = HEADER_SIZE
    seen = set()
    while pos < len(data):
        if len(data) - pos < SECTION_SIZE:
            raise ValueError("truncated section header")
        tag = data[pos:pos+10].rstrip(b"\0").decode("ascii", "strict")
        ver, size = struct.unpack_from("<HI", data, pos + 10)
        if tag in seen:
            raise ValueError("duplicate section: " + tag)
        seen.add(tag)
        start = pos + SECTION_SIZE
        end = start + size
        padded = start + ((size + 15) & ~15)
        if end > len(data) or padded > len(data):
            raise ValueError("truncated section: " + tag)
        payload = data[start:end]
        sections.append({"tag": tag, "version": ver, "declared_size": size,
                         "actual_size": len(payload),
                         "sha256": hashlib.sha256(payload).hexdigest()})
        pos = padded
    if pos != len(data):
        raise ValueError("trailing container bytes")
    required = {"UPD9CPU", "UPD9002"}
    if not required.issubset(seen):
        raise ValueError("missing required section")
    return sections

def replace(path, tag, replacement, output):
    data = bytearray(open(path, "rb").read())
    pos = HEADER_SIZE
    while pos < len(data):
        name = data[pos:pos+10].rstrip(b"\0").decode("ascii", "strict")
        _, size = struct.unpack_from("<HI", data, pos + 10)
        start = pos + SECTION_SIZE
        if name == tag:
            if len(replacement) != size:
                raise ValueError("replacement size mismatch")
            data[start:start + size] = replacement
            open(output, "wb").write(data)
            return
        pos = start + ((size + 15) & ~15)
    raise ValueError("section not found: " + tag)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("state")
    p.add_argument("--replace-tag")
    p.add_argument("--replacement")
    p.add_argument("--output")
    args = p.parse_args()
    if args.replace_tag:
        if not args.replacement or not args.output:
            p.error("--replace-tag requires --replacement and --output")
        replace(args.state, args.replace_tag, open(args.replacement, "rb").read(), args.output)
        inspect(args.output)
        return
    print(json.dumps(inspect(args.state), sort_keys=True, indent=2))

if __name__ == "__main__":
    main()
