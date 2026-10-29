#!/usr/bin/env python3
from dataclasses import dataclass

CAP_UI_SURFACE = 1 << 0
CAP_INPUT_TEXT = 1 << 1
CAP_INPUT_TAB = 1 << 2
CAP_CLIPBOARD_READ = 1 << 3
CAP_CLIPBOARD_WRITE = 1 << 4
KNOWN = CAP_UI_SURFACE | CAP_INPUT_TEXT | CAP_INPUT_TAB | CAP_CLIPBOARD_READ | CAP_CLIPBOARD_WRITE

INPUT_TEXT = 1 << 0
INPUT_TAB = 1 << 1
INPUT_CLIPBOARD = 1 << 2

@dataclass(frozen=True)
class Principal:
    pid: int
    app_id: str
    client_id: int
    generation: int
    declared: int
    effective: int


def principal_valid(p: Principal) -> bool:
    if p.pid <= 0 or p.client_id <= 0 or p.generation <= 0 or not p.app_id:
        return False
    if (p.declared | p.effective) & ~KNOWN:
        return False
    if p.effective & ~p.declared:
        return False
    return True


def lookup_by_kernel_pid(registry, peer_pid):
    # The lookup key is the PID observed by a trusted service from the kernel,
    # not an application-provided app_id/client_id/PID claim.
    matches = [p for p in registry if p.pid == peer_pid and principal_valid(p)]
    return matches[0] if len(matches) == 1 else None


def hello_allowed(principal, claimed_client_id):
    return bool(principal and principal.client_id == claimed_client_id and
                principal.effective & CAP_UI_SURFACE)


def input_flags_allowed(principal, flags):
    if principal is None:
        return False
    known_flags = INPUT_TEXT | INPUT_TAB | INPUT_CLIPBOARD
    if flags & ~known_flags:
        return False
    if flags & INPUT_TEXT and not (principal.effective & CAP_INPUT_TEXT):
        return False
    if flags & INPUT_TAB and not (principal.effective & CAP_INPUT_TAB):
        return False
    # The surface may opt into clipboard IPC if it has either direction;
    # SET/GET operations are authorized separately below.
    if flags & INPUT_CLIPBOARD and not (principal.effective & (CAP_CLIPBOARD_READ | CAP_CLIPBOARD_WRITE)):
        return False
    return True


def clipboard_read_allowed(principal):
    return bool(principal and principal.effective & CAP_CLIPBOARD_READ)


def clipboard_write_allowed(principal):
    return bool(principal and principal.effective & CAP_CLIPBOARD_WRITE)


good = Principal(
    pid=101,
    app_id="nova.demo.a",
    client_id=1,
    generation=3,
    declared=KNOWN,
    effective=KNOWN,
)
assert principal_valid(good)
assert lookup_by_kernel_pid([good], 101) == good
assert hello_allowed(good, 1)
assert not hello_allowed(good, 2)  # caller cannot spoof another client_id

# Authorization is PID/principal based, not client-id lookup based.
attacker = Principal(202, "nova.attacker", 9, 1, CAP_UI_SURFACE, CAP_UI_SURFACE)
assert lookup_by_kernel_pid([good, attacker], 202) == attacker
assert not hello_allowed(attacker, 1)

# Effective permissions may only narrow declared permissions.
assert not principal_valid(Principal(1, "nova.bad", 4, 1, CAP_UI_SURFACE, CAP_UI_SURFACE | CAP_CLIPBOARD_READ))
assert not principal_valid(Principal(1, "nova.bad", 4, 1, KNOWN | (1 << 40), KNOWN))

# Input capabilities are independent and fail closed for unknown flags.
text_only = Principal(303, "nova.text", 3, 1,
                      CAP_UI_SURFACE | CAP_INPUT_TEXT,
                      CAP_UI_SURFACE | CAP_INPUT_TEXT)
assert input_flags_allowed(text_only, INPUT_TEXT)
assert not input_flags_allowed(text_only, INPUT_TAB)
assert not input_flags_allowed(text_only, INPUT_CLIPBOARD)
assert not input_flags_allowed(text_only, 1 << 12)

# Clipboard read/write are deliberately separate authorization decisions.
reader = Principal(401, "nova.reader", 4, 1,
                   CAP_UI_SURFACE | CAP_CLIPBOARD_READ,
                   CAP_UI_SURFACE | CAP_CLIPBOARD_READ)
writer = Principal(402, "nova.writer", 5, 1,
                   CAP_UI_SURFACE | CAP_CLIPBOARD_WRITE,
                   CAP_UI_SURFACE | CAP_CLIPBOARD_WRITE)
assert clipboard_read_allowed(reader) and not clipboard_write_allowed(reader)
assert clipboard_write_allowed(writer) and not clipboard_read_allowed(writer)
assert input_flags_allowed(reader, INPUT_CLIPBOARD)
assert input_flags_allowed(writer, INPUT_CLIPBOARD)

# Ambiguous PID records fail closed rather than selecting an arbitrary principal.
assert lookup_by_kernel_pid([good, good], 101) is None
assert lookup_by_kernel_pid([good], 9999) is None

print("Stage-5A Phase-16 kernel-peer-principal/capability fail-closed model unit: PASS")
