#!/usr/bin/env python3
"""Generate a deterministic newc CPIO fragment containing early initramfs device nodes.

No mknod(2) or root privileges are required.  The output is intended to be
concatenated after the normal initramfs archive; Linux explicitly supports a
sequence of newc archives separated by TRAILER!!! records.
"""
from __future__ import annotations

import argparse
from pathlib import Path

MAGIC = b"070701"
S_IFCHR = 0o020000


def pad4(buf: bytearray) -> None:
    while len(buf) & 3:
        buf.append(0)


def field(value: int) -> bytes:
    if value < 0 or value > 0xFFFFFFFF:
        raise ValueError(f"newc field out of range: {value}")
    return f"{value:08x}".encode("ascii")


def entry(name: str, *, ino: int, mode: int, uid: int = 0, gid: int = 0,
          nlink: int = 1, mtime: int = 0, rmajor: int = 0, rminor: int = 0,
          data: bytes = b"") -> bytes:
    name_b = name.encode("utf-8") + b"\0"
    header = b"".join([
        MAGIC,
        field(ino), field(mode), field(uid), field(gid), field(nlink), field(mtime),
        field(len(data)), field(0), field(0), field(rmajor), field(rminor),
        field(len(name_b)), field(0),
    ])
    if len(header) != 110:
        raise AssertionError(f"invalid newc header size: {len(header)}")
    out = bytearray(header)
    out.extend(name_b)
    pad4(out)
    out.extend(data)
    pad4(out)
    return bytes(out)


def build(epoch: int) -> bytes:
    # /dev already exists in the primary archive.  This fragment adds the node
    # the kernel needs before PID 1 can mount devtmpfs over /dev.
    out = bytearray()
    out.extend(entry("dev/console", ino=1, mode=S_IFCHR | 0o600,
                     mtime=epoch, rmajor=5, rminor=1))
    out.extend(entry("TRAILER!!!", ino=2, mode=0, nlink=1, mtime=epoch))
    return bytes(out)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("output", type=Path)
    ap.add_argument("--epoch", type=int, default=0)
    ns = ap.parse_args()
    ns.output.parent.mkdir(parents=True, exist_ok=True)
    ns.output.write_bytes(build(ns.epoch))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
