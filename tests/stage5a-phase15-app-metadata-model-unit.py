#!/usr/bin/env python3
from dataclasses import dataclass
import re

MAX_ENTRIES = 64
DESCRIPTION_BYTES = 128
ICON_ID_BYTES = 48
KNOWN_FLAGS = (1 << 0) | (1 << 1)
VALID_CATEGORIES = set(range(0, 8))
TOKEN_RE = re.compile(r"^[A-Za-z0-9._-]+$")

@dataclass(frozen=True)
class Base:
    index: int
    total: int
    generation: int
    app_id: str
    client_id: int

@dataclass(frozen=True)
class Metadata:
    index: int
    total: int
    generation: int
    app_id: str
    client_id: int
    category: int
    flags: int
    description: str
    icon_asset_id: str


def c_string_fits(value: str, size: int) -> bool:
    return 0 < len(value.encode("utf-8")) < size and "\x00" not in value


def description_valid(value: str) -> bool:
    if not c_string_fits(value, DESCRIPTION_BYTES):
        return False
    # Phase-15 Step-1 keeps presentation text conservative until richer font/text work.
    return all(ch == " " or 0x21 <= ord(ch) <= 0x7e for ch in value)


def icon_id_valid(value: str) -> bool:
    if not c_string_fits(value, ICON_ID_BYTES):
        return False
    return bool(TOKEN_RE.fullmatch(value)) and "/" not in value and "\\" not in value and ":" not in value


def metadata_valid(base: Base, meta: Metadata) -> bool:
    if not (0 <= base.index < MAX_ENTRIES):
        return False
    if base.total < 1 or base.total > MAX_ENTRIES or base.index >= base.total:
        return False
    if meta.index != base.index or meta.total != base.total or meta.generation != base.generation:
        return False
    if meta.app_id != base.app_id or meta.client_id != base.client_id or meta.client_id == 0:
        return False
    if meta.category not in VALID_CATEGORIES or meta.flags & ~KNOWN_FLAGS:
        return False
    has_description = bool(meta.flags & (1 << 0))
    has_icon = bool(meta.flags & (1 << 1))
    if has_description != bool(meta.description):
        return False
    if has_icon != bool(meta.icon_asset_id):
        return False
    if meta.description and not description_valid(meta.description):
        return False
    if meta.icon_asset_id and not icon_id_valid(meta.icon_asset_id):
        return False
    return True

base = Base(0, 2, 9, "nova.demo.a", 1)
good = Metadata(0, 2, 9, "nova.demo.a", 1, 3, 3,
                "Development demo application", "nova.icon.demo.a")
assert metadata_valid(base, good)

# Snapshot/identity drift must fail closed.
for bad in (
    Metadata(1, 2, 9, "nova.demo.a", 1, 3, 3, good.description, good.icon_asset_id),
    Metadata(0, 3, 9, "nova.demo.a", 1, 3, 3, good.description, good.icon_asset_id),
    Metadata(0, 2, 10, "nova.demo.a", 1, 3, 3, good.description, good.icon_asset_id),
    Metadata(0, 2, 9, "nova.demo.b", 1, 3, 3, good.description, good.icon_asset_id),
    Metadata(0, 2, 9, "nova.demo.a", 2, 3, 3, good.description, good.icon_asset_id),
):
    assert not metadata_valid(base, bad)

# Unknown category/flags are rejected.
assert not metadata_valid(base, Metadata(0,2,9,"nova.demo.a",1,99,3,good.description,good.icon_asset_id))
assert not metadata_valid(base, Metadata(0,2,9,"nova.demo.a",1,3,7,good.description,good.icon_asset_id))

# Asset identity is deliberately not a path/URI/command carrier.
for bad_icon in (
    "/System/Icons/demo.png",
    "../demo.png",
    "file:demo.png",
    "https://example/icon.png",
    "nova/icon/demo",
    "nova\\icon\\demo",
    "x" * ICON_ID_BYTES,
):
    assert not icon_id_valid(bad_icon)

# Description remains bounded presentation text.
assert not description_valid("x" * DESCRIPTION_BYTES)
assert not description_valid("line1\nline2")

# Presence bits must agree with actual data.
assert metadata_valid(base, Metadata(0,2,9,"nova.demo.a",1,3,2,"",good.icon_asset_id))
assert metadata_valid(base, Metadata(0,2,9,"nova.demo.a",1,3,1,good.description,""))
# Explicit mismatch cases:
assert not metadata_valid(base, Metadata(0,2,9,"nova.demo.a",1,3,0,good.description,""))
assert not metadata_valid(base, Metadata(0,2,9,"nova.demo.a",1,3,0,"",good.icon_asset_id))

print("Stage-5A Phase-15 rich metadata snapshot-binding/category/no-path asset-identity model unit: PASS")
