#!/usr/bin/env python3
"""Strict build-time validator for NovaOS Stage-4D embedded Recovery descriptors.

This is a build guard only. Runtime validation in novad remains authoritative.
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

REQUIRED = (
    "format", "artifact_kind", "update_id", "version", "rollback_index",
    "payload", "payload_sha256", "payload_size", "verity_data_block_size",
    "verity_hash_block_size", "verity_data_blocks", "verity_hash_start_block",
    "verity_hash_algorithm", "verity_root_hash", "verity_salt",
)
HEX64 = re.compile(r"^[0-9A-Fa-f]{64}$")
TOKEN = re.compile(r"^[A-Za-z0-9._:+-]+$")
U64_MAX = (1 << 64) - 1


def canonical_u64(name: str, value: str) -> int:
    if not value or not value.isascii() or not value.isdecimal():
        raise ValueError(f"{name}: canonical unsigned decimal required")
    if len(value) > 1 and value[0] == "0":
        raise ValueError(f"{name}: leading zero is not canonical")
    number = int(value, 10)
    if number > U64_MAX:
        raise ValueError(f"{name}: unsigned 64-bit overflow")
    return number


def parse(path: Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise ValueError(f"line {lineno}: expected key=value")
        key, value = (part.strip() for part in line.split("=", 1))
        if key not in REQUIRED:
            raise ValueError(f"line {lineno}: unknown field {key!r}")
        if key in fields:
            raise ValueError(f"line {lineno}: duplicate field {key!r}")
        if not value:
            raise ValueError(f"line {lineno}: empty value for {key!r}")
        fields[key] = value
    missing = [name for name in REQUIRED if name not in fields]
    if missing:
        raise ValueError("missing field(s): " + ", ".join(missing))
    return fields


def validate(fields: dict[str, str]) -> None:
    if canonical_u64("format", fields["format"]) != 3:
        raise ValueError("format must be 3")
    if fields["artifact_kind"] != "recovery-slot":
        raise ValueError("artifact_kind must be recovery-slot")
    for name in ("update_id", "version"):
        if not TOKEN.fullmatch(fields[name]):
            raise ValueError(f"{name}: unsafe metadata token")
    if canonical_u64("rollback_index", fields["rollback_index"]) != 0:
        raise ValueError("Recovery rollback_index must be 0")
    if fields["payload"] != "recovery.slot":
        raise ValueError("payload must be recovery.slot")
    for name in ("payload_sha256", "verity_root_hash", "verity_salt"):
        if not HEX64.fullmatch(fields[name]):
            raise ValueError(f"{name}: expected 64 hexadecimal characters")
    expected = {
        "payload_size": 24 * 1024 * 1024,
        "verity_data_block_size": 4096,
        "verity_hash_block_size": 4096,
        "verity_data_blocks": 20 * 1024 * 1024 // 4096,
        "verity_hash_start_block": 20 * 1024 * 1024 // 4096,
    }
    for name, wanted in expected.items():
        if canonical_u64(name, fields[name]) != wanted:
            raise ValueError(f"{name}: expected {wanted}")
    if fields["verity_hash_algorithm"] != "sha256":
        raise ValueError("verity_hash_algorithm must be sha256")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("descriptor", type=Path)
    args = ap.parse_args()
    try:
        validate(parse(args.descriptor))
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"ERROR: invalid Stage-4D Recovery descriptor: {exc}", file=__import__("sys").stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
