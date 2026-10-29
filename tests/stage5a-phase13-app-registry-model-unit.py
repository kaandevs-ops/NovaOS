#!/usr/bin/env python3

"""
Phase-13 application registry manifest model/unit.

This validates the packaged registry metadata and uniqueness rules only.
It is not a QEMU/OVMF runtime acceptance test.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INDEX = ROOT / "configs/services/services.list"


def parse_manifest(path: Path):
    result = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        key, sep, value = line.partition("=")
        assert sep, f"malformed manifest line: {path}: {raw!r}"
        assert key not in result, f"duplicate key: {path}: {key}"
        result[key] = value
    return result


entries = []
for raw in INDEX.read_text(encoding="utf-8").splitlines():
    path = raw.strip()
    if not path or path.startswith("#"):
        continue
    assert path.startswith("/System/Services/")
    local = ROOT / "configs/services" / Path(path).name
    assert local.is_file(), f"missing indexed manifest: {local}"
    entries.append((local, parse_manifest(local)))

apps = [
    (path, manifest)
    for path, manifest in entries
    if manifest.get("kind", "service") == "application"
]

assert len(apps) == 3, f"expected 3 application manifests, got {len(apps)}"

app_ids = set()
client_ids = set()

expected = {
    "nova.demo.a": ("/System/Apps/nova-demo-app", 1),
    "nova.demo.b": ("/System/Apps/nova-demo-app-b", 2),
    "nova.files": ("/System/Apps/nova-files", 3),
}

for path, manifest in apps:
    app_id = manifest.get("app_id", "")
    assert app_id in expected, f"unknown application id in {path}: {app_id!r}"
    assert "/" not in app_id
    assert len(app_id) < 32

    client_id = int(manifest.get("client_id", "0"))
    assert client_id > 0

    assert app_id not in app_ids, f"duplicate app_id: {app_id}"
    assert client_id not in client_ids, f"duplicate client_id: {client_id}"

    app_ids.add(app_id)
    client_ids.add(client_id)

    expected_exec, expected_client = expected[app_id]
    assert manifest.get("exec") == expected_exec
    assert client_id == expected_client

    # Phase-13 cold-launch policy: applications are registered but not boot-autostarted.
    assert manifest.get("autostart") == "no"
    assert manifest.get("restart") == ("never" if app_id=="nova.files" else "always")
    assert manifest.get("enabled") == "yes"

assert app_ids == set(expected)

print(
    "Stage-5A Phase-13 application registry manifest "
    "uniqueness/non-autostart model unit: PASS"
)
