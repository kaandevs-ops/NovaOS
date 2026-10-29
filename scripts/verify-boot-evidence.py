#!/usr/bin/env python3
"""Strict NovaOS Stage-4D boot-evidence schema validator.

The evidence file is an output/attestation input, never an input that authorizes
NovaOS boot. Consumers must reject malformed, truncated, duplicate-key, unknown-
field, or internally inconsistent schema-3 evidence rather than guessing.
"""
from __future__ import annotations

import hashlib
import re
import sys
from pathlib import Path

SCHEMA3_FIELDS = (
    "schema", "evidence_kind", "evidence_status", "evidence_error", "version",
    "boot_stage", "normal_boot_authorized", "slot", "slot_descriptor_format",
    "slot_descriptor_kind", "rollback_index", "tpm_rollback_counter", "trust_epoch",
    "trust_epoch_state", "tpm_trust_epoch_counter", "secure_boot", "firmware_trust",
    "release_policy_verified", "release_policy_format", "release_policy_state",
    "release_policy_sha256", "preboot_release_policy_verified",
    "preboot_release_policy_sha256", "release_key_id", "release_active_cert_sha256",
    "release_next_key_id", "release_next_cert_sha256", "release_revoked_key_ids",
    "system_root", "recovery_root", "event_log_sha256", "event_log_bytes",
    "event_log_events", "event_log_pcr7_events", "event_log_error", "event_log_replay",
    "pcr7_live", "pcr7_source", "pcr7_replayed", "pcr14_before",
    "pcr14_before_source", "event_digest", "pcr14_after", "pcr14_after_source",
)
FIELD_SET = set(SCHEMA3_FIELDS)
TOKEN = re.compile(r"^[A-Za-z0-9._,-]+$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")
UINT = re.compile(r"^(0|[1-9][0-9]*)$")

class EvidenceError(ValueError):
    pass


def _token(value: str, name: str) -> None:
    if not value or not TOKEN.fullmatch(value):
        raise EvidenceError(f"{name}: invalid token")


def _uint(value: str, name: str) -> int:
    if not UINT.fullmatch(value):
        raise EvidenceError(f"{name}: invalid unsigned integer")
    return int(value)


def _hash_or_state(value: str, name: str, states=("unavailable", "none")) -> None:
    if value not in states and not HEX64.fullmatch(value):
        raise EvidenceError(f"{name}: expected lowercase SHA-256 or explicit unavailable state")


def parse_evidence(text: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for line_no, raw in enumerate(text.splitlines(), 1):
        if not raw:
            continue
        if raw.startswith("#"):
            raise EvidenceError(f"line {line_no}: comments are not part of schema 3")
        if "=" not in raw:
            raise EvidenceError(f"line {line_no}: malformed field")
        key, value = raw.split("=", 1)
        if key not in FIELD_SET:
            raise EvidenceError(f"line {line_no}: unknown field {key!r}")
        if key in values:
            raise EvidenceError(f"line {line_no}: duplicate field {key!r}")
        if value == "":
            raise EvidenceError(f"line {line_no}: empty value for {key!r}")
        values[key] = value
    missing = FIELD_SET - values.keys()
    if missing:
        raise EvidenceError("missing fields: " + ", ".join(sorted(missing)))
    if len(values) != len(SCHEMA3_FIELDS):
        raise EvidenceError("schema field cardinality mismatch")

    if values["schema"] != "3" or values["evidence_kind"] != "novaos-boot-evidence":
        raise EvidenceError("unsupported evidence schema/kind")
    if values["evidence_status"] not in {"VERIFIED", "REJECTED"}:
        raise EvidenceError("invalid evidence_status")
    if values["normal_boot_authorized"] not in {"yes", "no"}:
        raise EvidenceError("invalid normal_boot_authorized")
    if values["slot"] not in {"A", "B"}:
        raise EvidenceError("invalid slot")
    if values["secure_boot"] not in {"enabled", "disabled"}:
        raise EvidenceError("invalid secure_boot")
    if values["release_policy_verified"] not in {"yes", "no"} or values["preboot_release_policy_verified"] not in {"yes", "no"}:
        raise EvidenceError("invalid policy verification flag")
    if values["event_log_replay"] not in {"VERIFIED", "REJECTED"}:
        raise EvidenceError("invalid event_log_replay")
    if values["trust_epoch_state"] not in {"COMMITTED", "PENDING_TPM_COMMIT", "STALE", "FUTURE", "UNAVAILABLE"}:
        raise EvidenceError("invalid trust_epoch_state")

    for name in ("version", "boot_stage", "slot_descriptor_kind", "firmware_trust", "release_policy_state",
                 "release_key_id", "release_next_key_id", "release_revoked_key_ids", "event_log_error",
                 "pcr7_source", "pcr14_before_source", "pcr14_after_source", "evidence_error"):
        _token(values[name], name)
    for name in ("slot_descriptor_format", "rollback_index", "tpm_rollback_counter", "trust_epoch",
                 "tpm_trust_epoch_counter", "release_policy_format", "event_log_bytes", "event_log_events",
                 "event_log_pcr7_events"):
        _uint(values[name], name)
    for name in ("release_policy_sha256", "preboot_release_policy_sha256", "release_active_cert_sha256",
                 "release_next_cert_sha256", "system_root", "recovery_root", "event_log_sha256", "pcr7_live",
                 "pcr7_replayed", "pcr14_before", "event_digest", "pcr14_after"):
        _hash_or_state(values[name], name)

    # A readable event log must retain a concrete raw digest even when its
    # structural replay is rejected. This is diagnostic provenance only.
    if int(values["event_log_bytes"]) > 0 and not HEX64.fullmatch(values["event_log_sha256"]):
        raise EvidenceError("readable event log is missing its raw SHA-256 forensic digest")

    verified = values["evidence_status"] == "VERIFIED"
    if verified:
        if values["evidence_error"] != "none":
            raise EvidenceError("VERIFIED evidence must carry evidence_error=none")
        if values["boot_stage"] != "READY" or values["normal_boot_authorized"] != "yes":
            raise EvidenceError("VERIFIED evidence must correspond to READY authorization")
        if values["secure_boot"] != "enabled" or values["firmware_trust"] != "VERIFIED":
            raise EvidenceError("VERIFIED evidence requires verified firmware Secure Boot")
        if values["release_policy_verified"] != "yes" or values["event_log_replay"] != "VERIFIED":
            raise EvidenceError("VERIFIED evidence requires verified release policy and event-log replay")
        for name in ("release_policy_sha256", "release_active_cert_sha256", "system_root", "recovery_root",
                     "event_log_sha256", "pcr7_live", "pcr7_replayed", "pcr14_before", "event_digest", "pcr14_after"):
            if not HEX64.fullmatch(values[name]):
                raise EvidenceError(f"VERIFIED evidence requires concrete {name}")
        if values["pcr7_live"] != values["pcr7_replayed"]:
            raise EvidenceError("VERIFIED evidence PCR7 live/replayed mismatch")
        expected_pcr14 = hashlib.sha256(bytes.fromhex(values["pcr14_before"]) + bytes.fromhex(values["event_digest"])).hexdigest()
        if expected_pcr14 != values["pcr14_after"]:
            raise EvidenceError("VERIFIED evidence PCR14 extend relationship mismatch")
        rollback=int(values["rollback_index"]); rollback_tpm=int(values["tpm_rollback_counter"])
        if rollback < rollback_tpm or rollback > rollback_tpm + 1:
            raise EvidenceError("VERIFIED evidence rollback generation is inconsistent with TPM state")
        epoch=int(values["trust_epoch"]); trust_tpm=int(values["tpm_trust_epoch_counter"])
        expected_epoch_state = "COMMITTED" if epoch == trust_tpm + 1 else "PENDING_TPM_COMMIT" if epoch == trust_tpm + 2 else None
        if expected_epoch_state is None or values["trust_epoch_state"] != expected_epoch_state:
            raise EvidenceError("VERIFIED evidence trust epoch is inconsistent with TPM state")
        if int(values["event_log_bytes"]) <= 0 or int(values["event_log_events"]) <= 0:
            raise EvidenceError("VERIFIED evidence requires a non-empty parsed firmware event log")
        if values["release_next_key_id"] == "none":
            if values["release_next_cert_sha256"] != "none":
                raise EvidenceError("next release key/certificate state mismatch")
        elif not HEX64.fullmatch(values["release_next_cert_sha256"]):
            raise EvidenceError("authorized next release key requires a concrete certificate digest")
        revoked=set() if values["release_revoked_key_ids"] == "none" else set(values["release_revoked_key_ids"].split(','))
        if values["release_key_id"] in revoked or values["release_next_key_id"] in revoked:
            raise EvidenceError("active/next release identity appears in revoked set")
        if int(values["slot_descriptor_format"]) == 5:
            if values["preboot_release_policy_verified"] != "yes":
                raise EvidenceError("format-5 VERIFIED evidence requires preboot policy verification")
            if values["preboot_release_policy_sha256"] != values["release_policy_sha256"]:
                raise EvidenceError("format-5 preboot/System policy digest mismatch")
    else:
        if values["normal_boot_authorized"] != "no" or values["boot_stage"] == "READY":
            raise EvidenceError("REJECTED evidence must not authorize READY normal boot")
        if values["evidence_error"] == "none":
            raise EvidenceError("REJECTED evidence requires a diagnostic evidence_error")

    return values


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {Path(argv[0]).name} <boot-evidence.env>", file=sys.stderr)
        return 2
    path = Path(argv[1])
    try:
        values = parse_evidence(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, EvidenceError) as exc:
        print(f"NovaOS boot evidence: REJECTED: {exc}", file=sys.stderr)
        return 1
    print(f"NovaOS boot evidence: {values['evidence_status']} schema=3 slot={values['slot']} stage={values['boot_stage']}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
