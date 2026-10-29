#!/usr/bin/env python3
"""Create and inspect a deterministic GPT disk image from partition payloads.

This intentionally implements only the small GPT subset NovaOS Stage-3A needs so
we do not require privileged loop devices or sgdisk during image assembly.
Filesystem payloads are prepared separately (currently ext4 via mke2fs).
"""
from __future__ import annotations
import argparse, binascii, json, os, struct, uuid
from pathlib import Path

SECTOR = 512
ENTRY_COUNT = 128
ENTRY_SIZE = 128
PRIMARY_HEADER_LBA = 1
PRIMARY_ENTRIES_LBA = 2
ENTRIES_SECTORS = (ENTRY_COUNT * ENTRY_SIZE + SECTOR - 1) // SECTOR
FIRST_USABLE_LBA = 2048  # 1 MiB alignment
LINUX_FS = uuid.UUID("0fc63daf-8483-4772-8e79-3d69d8477de4")
ESP = uuid.UUID("c12a7328-f81f-11d2-ba4b-00a0c93ec93b")
DISK_GUID = uuid.UUID("7ab58d5a-36a0-5f2c-b896-60ebd8f34e79")


def align_up(v: int, a: int) -> int:
    return (v + a - 1) // a * a


def crc32(data: bytes) -> int:
    return binascii.crc32(data) & 0xFFFFFFFF


def guid_for(name: str) -> uuid.UUID:
    return uuid.uuid5(DISK_GUID, f"NovaOS Stage-3A:{name}")


def pack_name(name: str) -> bytes:
    raw = name.encode("utf-16le")[:72]
    return raw + b"\0" * (72 - len(raw))


def make_entry(type_guid: uuid.UUID, part_guid: uuid.UUID, first: int, last: int, name: str) -> bytes:
    return struct.pack("<16s16sQQQ72s", type_guid.bytes_le, part_guid.bytes_le, first, last, 0, pack_name(name))


def make_header(current: int, backup: int, first_usable: int, last_usable: int,
                entries_lba: int, entries_crc: int) -> bytes:
    base = struct.pack(
        "<8sIIIIQQQQ16sQIII",
        b"EFI PART", 0x00010000, 92, 0, 0,
        current, backup, first_usable, last_usable,
        DISK_GUID.bytes_le, entries_lba, ENTRY_COUNT, ENTRY_SIZE, entries_crc,
    )
    hdr_crc = crc32(base[:16] + b"\0\0\0\0" + base[20:92])
    base = base[:16] + struct.pack("<I", hdr_crc) + base[20:]
    return base + b"\0" * (SECTOR - len(base))


def protective_mbr(total_sectors: int) -> bytes:
    mbr = bytearray(SECTOR)
    count = min(total_sectors - 1, 0xFFFFFFFF)
    # status, CHS start, type, CHS end, first LBA, sector count
    mbr[446:462] = struct.pack("<B3sB3sII", 0, b"\0\x02\0", 0xEE, b"\xff\xff\xff", 1, count)
    mbr[510:512] = b"\x55\xaa"
    return bytes(mbr)


def build(output: Path, spec_path: Path) -> None:
    spec = json.loads(spec_path.read_text())
    parts = spec["partitions"]
    cursor = FIRST_USABLE_LBA
    records = []
    for p in parts:
        size_bytes = int(p["size_bytes"])
        sectors = align_up(size_bytes, SECTOR) // SECTOR
        cursor = align_up(cursor, 2048)
        first = cursor
        last = first + sectors - 1
        cursor = last + 1
        records.append({**p, "first_lba": first, "last_lba": last})

    backup_header_lba = align_up(cursor + ENTRIES_SECTORS + 1, 2048) - 1
    backup_entries_lba = backup_header_lba - ENTRIES_SECTORS
    last_usable_lba = backup_entries_lba - 1
    if records[-1]["last_lba"] > last_usable_lba:
        raise SystemExit("partition layout overlaps backup GPT")
    total_sectors = backup_header_lba + 1

    entries = bytearray(ENTRY_COUNT * ENTRY_SIZE)
    for i, p in enumerate(records):
        type_guid = ESP if p["type"] == "esp" else LINUX_FS
        ent = make_entry(type_guid, guid_for(p["name"]), p["first_lba"], p["last_lba"], p["name"])
        entries[i*ENTRY_SIZE:(i+1)*ENTRY_SIZE] = ent
    entries_crc = crc32(entries)

    primary = make_header(PRIMARY_HEADER_LBA, backup_header_lba, FIRST_USABLE_LBA,
                          last_usable_lba, PRIMARY_ENTRIES_LBA, entries_crc)
    backup = make_header(backup_header_lba, PRIMARY_HEADER_LBA, FIRST_USABLE_LBA,
                         last_usable_lba, backup_entries_lba, entries_crc)

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as f:
        f.truncate(total_sectors * SECTOR)
        f.seek(0); f.write(protective_mbr(total_sectors))
        f.seek(PRIMARY_HEADER_LBA * SECTOR); f.write(primary)
        f.seek(PRIMARY_ENTRIES_LBA * SECTOR); f.write(entries)
        f.seek(backup_entries_lba * SECTOR); f.write(entries)
        f.seek(backup_header_lba * SECTOR); f.write(backup)
        for p in records:
            payload = Path(p["payload"])
            data_size = payload.stat().st_size
            capacity = (p["last_lba"] - p["first_lba"] + 1) * SECTOR
            if data_size > capacity:
                raise SystemExit(f"payload too large for {p['name']}: {data_size}>{capacity}")
            f.seek(p["first_lba"] * SECTOR)
            with payload.open("rb") as src:
                while True:
                    chunk = src.read(1024 * 1024)
                    if not chunk: break
                    f.write(chunk)

    meta = {
        "format": "nova-gpt-v1",
        "sector_size": SECTOR,
        "disk_guid": str(DISK_GUID),
        "total_sectors": total_sectors,
        "partitions": [
            {k: p[k] for k in ("name", "type", "first_lba", "last_lba", "size_bytes")}
            | {"part_guid": str(guid_for(p["name"]))}
            for p in records
        ],
    }
    output.with_suffix(output.suffix + ".json").write_text(json.dumps(meta, indent=2) + "\n")
    print(f"Built GPT disk: {output} ({total_sectors * SECTOR} bytes)")
    for p in records:
        print(f"  {p['name']:<12} LBA {p['first_lba']}-{p['last_lba']} ({p['size_bytes']//(1024*1024)} MiB)")


def inspect(image: Path) -> None:
    with image.open("rb") as f:
        mbr = f.read(SECTOR)
        if mbr[510:512] != b"\x55\xaa" or mbr[450] != 0xEE:
            raise SystemExit("invalid protective MBR")
        f.seek(SECTOR)
        hdr = f.read(SECTOR)
        if hdr[:8] != b"EFI PART": raise SystemExit("invalid GPT signature")
        revision, hsize, stored_crc = struct.unpack_from("<III", hdr, 8)
        if revision != 0x00010000 or hsize < 92: raise SystemExit("unsupported GPT header")
        tmp = bytearray(hdr[:hsize]); struct.pack_into("<I", tmp, 16, 0)
        if crc32(tmp) != stored_crc: raise SystemExit("primary GPT header CRC mismatch")
        current, backup, first_use, last_use = struct.unpack_from("<QQQQ", hdr, 24)
        entries_lba = struct.unpack_from("<Q", hdr, 72)[0]
        count, esize, entries_crc = struct.unpack_from("<III", hdr, 80)
        f.seek(entries_lba * SECTOR); entries = f.read(count * esize)
        if crc32(entries) != entries_crc: raise SystemExit("partition array CRC mismatch")
        found = []
        for i in range(count):
            e = entries[i*esize:(i+1)*esize]
            if e[:16] == b"\0"*16: continue
            t = uuid.UUID(bytes_le=e[:16]); pg = uuid.UUID(bytes_le=e[16:32])
            first, last = struct.unpack_from("<QQ", e, 32)
            name = e[56:128].decode("utf-16le").rstrip("\0")
            found.append((name, t, pg, first, last))
        if current != 1 or not found: raise SystemExit("GPT has no usable partitions")
        f.seek(backup * SECTOR); bh = f.read(SECTOR)
        if bh[:8] != b"EFI PART": raise SystemExit("backup GPT missing")
        bsize = struct.unpack_from("<I", bh, 12)[0]; bcrc = struct.unpack_from("<I", bh, 16)[0]
        btmp = bytearray(bh[:bsize]); struct.pack_into("<I", btmp, 16, 0)
        if crc32(btmp) != bcrc: raise SystemExit("backup GPT header CRC mismatch")
    print(f"GPT verification passed: {image}")
    print(f"  usable LBA: {first_use}-{last_use}")
    for name, t, pg, first, last in found:
        print(f"  {name:<12} {first}-{last} type={t} guid={pg}")


def main():
    ap = argparse.ArgumentParser()
    sp = ap.add_subparsers(dest="cmd", required=True)
    b = sp.add_parser("build"); b.add_argument("output", type=Path); b.add_argument("spec", type=Path)
    i = sp.add_parser("inspect"); i.add_argument("image", type=Path)
    a = ap.parse_args()
    build(a.output, a.spec) if a.cmd == "build" else inspect(a.image)

if __name__ == "__main__": main()
