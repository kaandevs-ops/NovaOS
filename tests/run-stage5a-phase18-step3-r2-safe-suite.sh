#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
[[ ! -L "$ROOT/build" ]] || { echo "REFUSED: build/ is a symlink" >&2; exit 2; }
SAVE="$(mktemp -d "$ROOT/../.phase18-step3-r2-hygiene.XXXXXXXX")"
MOVED=0
cleanup(){
  local rc=$?
  trap - EXIT
  if [[ "$MOVED" == 1 ]]; then
    if [[ -e "$ROOT/build" || -L "$ROOT/build" ]]; then
      echo "RESTORE BLOCKED: unexpected build/ appeared; saved build remains at $SAVE/build" >&2
      exit 3
    fi
    mv -- "$SAVE/build" "$ROOT/build"
    echo "Existing build/ restored without modification"
  fi
  rmdir -- "$SAVE" || true
  exit "$rc"
}
trap cleanup EXIT
if [[ -e "$ROOT/build" ]]; then
  mv -- "$ROOT/build" "$SAVE/build"
  MOVED=1
fi
cd "$ROOT"
export PYTHONDONTWRITEBYTECODE=1
bash tests/run-stage5a-phase18-step3-package-suite.sh
bash tests/stage5a-phase18-step3-r2-adversarial-source.sh
printf 'PHASE-18 STEP-3 R2 SOURCE+MODEL+FROZEN REGRESSIONS: PASS (no QEMU yet)
'
