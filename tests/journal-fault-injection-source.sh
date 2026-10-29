#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PY="$ROOT/scripts/windows/final-tpm-check.py"
for needle in \
  'def fsck_ext4(path):' \
  "subprocess.run(['e2fsck','-fy',str(path)]" \
  'def inject_hostile_journal_rollback():' \
  "for name,seq in (('state.A',900),('state.B',901))" \
  "debugfs','-w','-R',f'rm /Nova/Boot/{name}" \
  "debugfs','-w','-R',f'write {host} /Nova/Boot/{name}" \
  'valid-checksum hostile boot-journal rollback'; do grep -Fq "$needle" "$PY"; done
python3 -m py_compile "$PY"
echo "Stage-4C deterministic hostile-journal injection source test passed."
