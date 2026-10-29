#!/usr/bin/env python3
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
main=(ROOT/'src/init/main.c').read_text()
NAME=int(re.search(r'^#define SERVICE_NAME_MAX\s+(\d+)$',main,re.M).group(1))
PATH=int(re.search(r'^#define SERVICE_PATH_MAX\s+(\d+)$',main,re.M).group(1))

def bounded(value: str, capacity: int) -> bool:
    return bool(value) and len(value.encode()) < capacity

assert bounded('phase16-security-probe-foreign', NAME)
assert not bounded('phase16-security-probe-unregistered', NAME)
assert bounded('/System/Apps/nova-security-probe-unregistered', PATH)
assert not bounded('x' * NAME, NAME)
assert bounded('x' * (NAME-1), NAME)
assert not bounded('x' * PATH, PATH)
assert bounded('x' * (PATH-1), PATH)
print('Stage-5A Phase-16 bounded service-manifest identity/path fail-closed model unit: PASS')
