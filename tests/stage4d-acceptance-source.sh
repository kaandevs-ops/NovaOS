#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/scripts/windows/final-stage4d-check.py"
S="$ROOT/scripts/windows/final-stage4d-check.sh"
F="$ROOT/scripts/build-stage4d-rotation-fixtures.sh"
for needle in \
  "event_log_verified=yes" "event_log_replay=VERIFIED" "cat /run/nova/boot-evidence.env" \
  "release_policy_verified=yes" "release_trust_epoch=1" "release_active_key_id=nova-development-release-2026-a" \
  "release_next_key_id=nova-development-release-2026-b" "stage4d-rotation-2" \
  "tpm_trust_epoch_counter=1" "tpm_counter_commit_recovered=yes" "tpm_trust_epoch_commit_recovered=yes" \
  "corrupt-policy" "missing-key" "revoked release signer rejected" "unknown release signer rejected" \
  "TPM missing/unavailable fails closed" "tpm_device_node=missing" "power loss after inactive descriptor commit but before journal schedule leaves confirmed A authoritative" \
  "power loss after journal confirmation recovered both TPM monotonic commits" "measurement preimage + PCR14 host recomputation" \
  "STALE_UPDATE" "release-policy.sig" "release-active.der" "active_slot_descriptor_format=5" "preboot_release_policy_verified=yes" \
  "slot-policy-tamper" "slot-key-missing" "evidence_status=REJECTED" \
  "persist: mutation refused outside fully authorized normal boot" "Mode                 : TRUSTED DIAGNOSTIC" \
  "recovery_payload_trust=TRUSTED" "persistent_mutation_authorized=no"; do
  grep -Fq "$needle" "$P" || { echo "missing Stage-4D acceptance check: $needle" >&2; exit 1; }
done
for needle in 'run-stage4d-suite.sh' 'format=5' 'trust_epoch=2' 'signer_key_id=$B_ID' 'transition=promote-next' 'uefi-db.crt.pem' 'uefi-roles.conf' 'dbx-policy.conf' 'production_key_ceremony=no' 'missing-active-key' 'CONFIG_SECURITYFS' 'build-stage4d-rotation-fixtures.sh' 'final-stage4d-check.py'; do
  grep -Fq "$needle" "$S" || { echo "missing Stage-4D wrapper check: $needle" >&2; exit 1; }
done
for needle in 'stale-b-update' 'corrupt-policy' 'missing-active-key' 'release-policy.conf' 'NOVA_UPDATE_SIGNER_KEY_ID="$B_ID"' 'NOVA_PREVIOUS_RELEASE_POLICY="$FIX/base-system/Trust/release-policy.conf"'; do
  grep -Fq "$needle" "$F" || { echo "missing Stage-4D failure fixture: $needle" >&2; exit 1; }
done

for needle in 'inject_committed_inactive_slot_without_journal' 'measurement_format' 'NovaOS Stage-4D measured boot evidence v2' 'expected_event=hashlib.sha256(preimage).hexdigest()'; do
  grep -Fq "$needle" "$P" || { echo "missing strengthened Stage-4D acceptance measurement/power-loss check: $needle" >&2; exit 1; }
done

# Runtime fixture builder must remain safe under `set -u`: derived locals are
# assigned in separate shell commands rather than referencing a sibling local
# from the same `local` builtin invocation.
grep -Fq 'local dir srcroot envfile label' "$F"
grep -Fq 'dir="$FIX/$name"' "$F"
grep -Fq 'envfile="$dir/system.env"' "$F"
if grep -Eq '^[[:space:]]*local[[:space:]].*dir=.*envfile="\$dir/' "$F"; then
  echo "unsafe dependent local assignment remains in Stage-4D fixture builder" >&2
  exit 1
fi

# Every transcript passed to verify_measurement() must explicitly emit the
# runtime measurement record.  This is checked structurally so future boot
# sessions cannot accidentally validate a transcript that only contains the
# boot-evidence/state output with overlapping field names.
python3 - "$P" <<'PY_MEASURE'
import ast, sys
from pathlib import Path
src=Path(sys.argv[1]).read_text()
tree=ast.parse(src)
sessions={}
for node in ast.walk(tree):
    if not isinstance(node, ast.Assign) or len(node.targets)!=1 or not isinstance(node.targets[0], ast.Name):
        continue
    call=node.value
    if not isinstance(call, ast.Call) or not isinstance(call.func, ast.Name) or call.func.id!='run_session' or len(call.args)<3:
        continue
    cmds=call.args[2]
    if isinstance(cmds, (ast.List, ast.Tuple)):
        sessions[node.targets[0].id]=[e.value for e in cmds.elts if isinstance(e, ast.Constant) and isinstance(e.value,str)]
checked=[]
for node in ast.walk(tree):
    if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Name) or node.func.id!='verify_measurement' or not node.args:
        continue
    arg=node.args[0]
    if not isinstance(arg, ast.Name) or arg.id not in sessions:
        raise SystemExit('verify_measurement input is not a statically auditable run_session transcript')
    checked.append(arg.id)
    if 'cat /run/nova/measurements.log' not in sessions[arg.id]:
        raise SystemExit(f'{arg.id}: measurement verifier lacks /run/nova/measurements.log transcript input')
if set(checked)!={'pf','b1','b2','b3','fin'}:
    raise SystemExit(f'unexpected Stage-4D measurement-verification session set: {sorted(set(checked))}')
print('Stage-4D measurement transcript producer/consumer wiring: PASS')
PY_MEASURE

python3 -m py_compile "$P"
rm -rf "$ROOT/scripts/windows/__pycache__"
echo "Stage-4D fail-fast evidence/release-rotation/failure acceptance source contract passed."

# Stage-4D Recovery descriptor is firmware-bound and acceptance destroys Data in a clone to prove independence.
for needle in 'data-unavailable' 'zero_partition(data_unavailable_disk' 'recovery_descriptor_source=firmware-initramfs' 'storage_persistent=no' 'Data-unavailable Recovery independence'; do
  grep -Fq "$needle" "$ROOT/scripts/windows/final-stage4d-check.py"
done
