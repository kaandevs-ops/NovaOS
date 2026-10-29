#!/usr/bin/env python3
"""Strict host-side NovaOS Stage-4D release-policy validator.

This is a build-time guard only. Runtime verification in novad remains the
security authority. The validator mirrors the schema-3 structural rules and can
also validate an active->next promotion against an authenticated previous
policy without relying on shell integer arithmetic.
"""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

TOKEN = re.compile(r"^[A-Za-z0-9._-]+$")
HEX64 = re.compile(r"^[0-9A-Fa-f]{64}$")
ULONG_MAX = (1 << 64) - 1  # NovaOS supported Stage-4D x86_64/aarch64 ABIs.

REQUIRED = {
    "format",
    "policy_id",
    "policy_state",
    "signature_algorithm",
    "trust_epoch",
    "active_key_id",
    "active_cert",
    "active_cert_sha256",
    "next_key_id",
    "next_cert",
    "next_cert_sha256",
    "revoked_key_ids",
    "policy_binding",
}
BINDING = "root-detached-signature+root-signed-system-manifest+dm-verity+TPM-trust-epoch"


class PolicyError(ValueError):
    pass


@dataclass(frozen=True)
class Policy:
    epoch: int
    active: str
    active_cert_sha256: str
    next: str
    next_cert_sha256: str | None
    revoked: frozenset[str]


def token(value: str, field: str, *, allow_none: bool = True) -> str:
    if not TOKEN.fullmatch(value):
        raise PolicyError(f"{field}: invalid metadata token")
    if not allow_none and value == "none":
        raise PolicyError(f"{field}: 'none' is not allowed")
    return value


def uint(value: str, field: str, *, minimum: int = 0) -> int:
    if not re.fullmatch(r"[0-9]+", value):
        raise PolicyError(f"{field}: expected unsigned decimal integer")
    # Canonical decimal prevents textual aliases from producing ambiguous
    # signed metadata even when they represent the same numeric value.
    if len(value) > 1 and value[0] == "0":
        raise PolicyError(f"{field}: non-canonical leading zero")
    number = int(value, 10)
    if number < minimum or number > ULONG_MAX:
        raise PolicyError(f"{field}: value outside supported unsigned-long range")
    return number


def parse_revoked(raw: str) -> frozenset[str]:
    if raw == "none":
        return frozenset()
    parts = raw.split(",")
    if not parts or any(not p for p in parts):
        raise PolicyError("revoked_key_ids: empty list member")
    seen: set[str] = set()
    for item in parts:
        token(item, "revoked_key_ids", allow_none=False)
        if item in seen:
            raise PolicyError(f"revoked_key_ids: duplicate key id {item}")
        seen.add(item)
    return frozenset(seen)


def parse_policy(path: Path) -> Policy:
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise PolicyError(f"{path}: cannot read policy: {exc}") from exc

    values: dict[str, str] = {}
    for lineno, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise PolicyError(f"{path}:{lineno}: malformed line")
        key, value = (part.strip() for part in line.split("=", 1))
        if key not in REQUIRED:
            raise PolicyError(f"{path}:{lineno}: unknown field {key!r}")
        if key in values:
            raise PolicyError(f"{path}:{lineno}: duplicate field {key!r}")
        if not value:
            raise PolicyError(f"{path}:{lineno}: empty value for {key!r}")
        values[key] = value

    missing = REQUIRED - values.keys()
    if missing:
        raise PolicyError(f"{path}: missing fields: {', '.join(sorted(missing))}")
    extra = values.keys() - REQUIRED
    if extra:
        raise PolicyError(f"{path}: unknown fields: {', '.join(sorted(extra))}")

    if values["format"] != "3":
        raise PolicyError("format: Stage-4D build artifacts require schema 3")
    if values["policy_id"] != "novaos-release-policy":
        raise PolicyError("policy_id: unexpected policy identity")
    if values["signature_algorithm"] != "rsa-pkcs1-sha256":
        raise PolicyError("signature_algorithm: unsupported algorithm")
    if values["policy_binding"] != BINDING:
        raise PolicyError("policy_binding: unexpected trust binding")

    epoch = uint(values["trust_epoch"], "trust_epoch", minimum=1)
    active = token(values["active_key_id"], "active_key_id", allow_none=False)
    if values["active_cert"] != "release-active.der":
        raise PolicyError("active_cert: expected release-active.der")
    if not HEX64.fullmatch(values["active_cert_sha256"]):
        raise PolicyError("active_cert_sha256: expected SHA-256 hex digest")
    active_cert_sha256 = values["active_cert_sha256"].lower()

    next_id = token(values["next_key_id"], "next_key_id")
    revoked = parse_revoked(values["revoked_key_ids"])
    expected_state = "stable" if next_id == "none" else "rotation-prepared"
    if values["policy_state"] != expected_state:
        raise PolicyError(f"policy_state: expected {expected_state}")

    if active in revoked:
        raise PolicyError("active_key_id: active key is revoked")
    next_cert_sha256: str | None = None
    if next_id == "none":
        if values["next_cert"] != "none" or values["next_cert_sha256"] != "none":
            raise PolicyError("next key absent but next certificate metadata is present")
    else:
        if next_id == active:
            raise PolicyError("next_key_id: next key must differ from active key")
        if next_id in revoked:
            raise PolicyError("next_key_id: next key is revoked")
        if values["next_cert"] != "release-next.der":
            raise PolicyError("next_cert: expected release-next.der")
        if not HEX64.fullmatch(values["next_cert_sha256"]):
            raise PolicyError("next_cert_sha256: expected SHA-256 hex digest")
        next_cert_sha256 = values["next_cert_sha256"].lower()

    return Policy(epoch=epoch, active=active, active_cert_sha256=active_cert_sha256,
                  next=next_id, next_cert_sha256=next_cert_sha256, revoked=revoked)


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("policy", type=Path)
    ap.add_argument("--expect-epoch")
    ap.add_argument("--expect-active")
    ap.add_argument("--previous", type=Path)
    ap.add_argument("--promote-next", action="store_true")
    ns = ap.parse_args(argv[1:])

    try:
        current = parse_policy(ns.policy)
        if ns.expect_epoch is not None:
            expected = uint(ns.expect_epoch, "expected trust_epoch", minimum=1)
            if current.epoch != expected:
                raise PolicyError(f"target trust_epoch={current.epoch} does not match expected {expected}")
        if ns.expect_active is not None:
            expected_active = token(ns.expect_active, "expected active key", allow_none=False)
            if current.active != expected_active:
                raise PolicyError(f"target active_key_id={current.active} does not match expected {expected_active}")

        if ns.promote_next:
            if ns.previous is None:
                raise PolicyError("--promote-next requires --previous")
            previous = parse_policy(ns.previous)
            if previous.epoch == ULONG_MAX:
                raise PolicyError("previous trust_epoch exhausted; promotion cannot advance monotonically")
            if current.epoch != previous.epoch + 1:
                raise PolicyError("promote-next target epoch must be previous epoch + 1")
            if previous.next == "none" or previous.next != current.active:
                raise PolicyError("promote-next target active key is not the previous policy next key")
            if previous.next_cert_sha256 is None or current.active_cert_sha256 != previous.next_cert_sha256:
                raise PolicyError("promote-next target active certificate is not the exact previous next certificate")
            if previous.active == current.active:
                raise PolicyError("promote-next target active key unexpectedly equals previous active key")
            if previous.active not in current.revoked:
                raise PolicyError("promote-next target policy must revoke the previous active key")
            missing = previous.revoked - current.revoked
            if missing:
                raise PolicyError("promote-next target policy attempted to un-revoke: " + ",".join(sorted(missing)))
        elif ns.previous is not None:
            raise PolicyError("--previous is meaningful only with --promote-next")
    except PolicyError as exc:
        print(f"NovaOS release policy: REJECTED: {exc}", file=sys.stderr)
        return 1

    print(f"NovaOS release policy: VALID schema=3 epoch={current.epoch} active={current.active} next={current.next}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
