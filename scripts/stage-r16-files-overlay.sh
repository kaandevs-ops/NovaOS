#!/usr/bin/env bash
# Prepare reviewable unsigned rootfs overlay only. No mount, signing, boot, or image mutation.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-x86_64}"
case "$ARCH" in x86_64|arm64);; *) exit 2;; esac
BIN="$ROOT/build/$ARCH/r16-integration/System/Apps/nova-files"
[[ -f "$BIN" ]] || { echo 'Build Files first' >&2;exit 2; }
OUT="$(mktemp -d "$ROOT/build/$ARCH/r16-files/overlay.XXXXXXXX")"
mkdir -p "$OUT/System/Apps" "$OUT/System/Core" "$OUT/System/Services" "$OUT/review-only"
install -m 0755 "$BIN" "$OUT/System/Apps/nova-files"
install -m 0755 "$ROOT/build/$ARCH/r16-integration/init" "$OUT/init"
install -m 0755 "$ROOT/build/$ARCH/r16-integration/System/Core/nova-compositor" "$OUT/System/Core/nova-compositor"
install -m 0644 "$ROOT/configs/services/nova-files.service" "$OUT/System/Services/nova-files.service"
install -m 0644 "$ROOT/configs/services/services.list" "$OUT/System/Services/services.list"
printf '%s\n' 'UNSIGNED_REVIEW_ONLY: production source integration exists; full verified image, actual PID1 guest boot and VM acceptance pending. Trust pins in isolated integration build are UNPINNED; not a release image.' > "$OUT/review-only/ACTIVATION_BLOCKED.txt"
(cd "$OUT" && find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
echo "R16_UNSIGNED_REVIEW_OVERLAY=$OUT"
