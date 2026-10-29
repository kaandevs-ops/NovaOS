#!/usr/bin/env python3
from pathlib import Path

CAPS = [
    ("ui.surface", 1 << 0),
    ("input.text", 1 << 1),
    ("input.tab-capture", 1 << 2),
    ("clipboard.read", 1 << 3),
    ("clipboard.write", 1 << 4),
]
KNOWN = sum(bit for _, bit in CAPS)
RANK = {name: i + 1 for i, (name, _) in enumerate(CAPS)}
BIT = dict(CAPS)


def parse_caps(value: str):
    if value == "none":
        return 0
    if not value:
        return None
    mask = 0
    previous = 0
    for token in value.split(","):
        if not token or token not in BIT:
            return None
        rank = RANK[token]
        bit = BIT[token]
        if rank <= previous or mask & bit:
            return None
        mask |= bit
        previous = rank
    if mask & ~KNOWN:
        return None
    return mask


def parse_manifest(text: str):
    pairs = []
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            continue
        k, v = line.split("=", 1)
        pairs.append((k.strip(), v.strip()))
    values = {}
    caps_seen = 0
    declared = None
    for k, v in pairs:
        if k == "capabilities":
            caps_seen += 1
            if caps_seen != 1:
                return None
            declared = parse_caps(v)
            if declared is None:
                return None
        else:
            values[k] = v
    is_app = values.get("kind") == "application"
    if is_app and caps_seen != 1:
        return None
    if not is_app and caps_seen:
        return None
    if is_app:
        effective = declared & KNOWN
        if effective & ~declared:
            return None
        return declared, effective
    return 0, 0

canonical = "ui.surface,input.text,input.tab-capture,clipboard.read,clipboard.write"
assert parse_caps(canonical) == KNOWN
assert parse_caps("none") == 0
assert parse_caps("ui.surface,clipboard.read") == ((1 << 0) | (1 << 3))

# Canonical ordering and uniqueness are part of the signed-manifest format.
assert parse_caps("input.text,ui.surface") is None
assert parse_caps("ui.surface,ui.surface") is None
assert parse_caps("ui.surface,") is None
assert parse_caps(",ui.surface") is None
assert parse_caps("ui.surface,network.client") is None
assert parse_caps("ui.surface, clipboard.read") is None

base = """\
name=demo
exec=/System/Apps/demo
kind=application
app_id=nova.demo
client_id=1
capabilities={caps}
"""
assert parse_manifest(base.format(caps=canonical)) == (KNOWN, KNOWN)
assert parse_manifest(base.format(caps="none")) == (0, 0)
assert parse_manifest(base.replace("capabilities={caps}\n", "").format(caps=canonical)) is None
assert parse_manifest((base + "capabilities=ui.surface\n").format(caps=canonical)) is None
service = "name=svc\nexec=/System/Core/svc\nkind=service\ncapabilities=ui.surface\n"
assert parse_manifest(service) is None

# Check the actual demo manifests carry exactly the capabilities their accepted
# UI/text/TAB/clipboard behavior requires and no undeclared vocabulary.
root = Path(__file__).resolve().parents[1]
for name in ("demo-app.service", "demo-app-b.service"):
    text = (root / "configs/services" / name).read_text(encoding="utf-8")
    result = parse_manifest(text)
    assert result == (KNOWN, KNOWN), (name, result)
    assert text.count("capabilities=") == 1

print("Stage-5A Phase-16 canonical manifest declared/effective capability-policy model unit: PASS")
