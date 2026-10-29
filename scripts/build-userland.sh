#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-arm64}"
VERSION="${NOVA_VERSION:-0.0.12-dev}"
BROWSER="${NOVA_R47_BROWSER_COMPAT:-0}"
[[ "$BROWSER" == 0 || "$BROWSER" == 1 ]] || { echo "Invalid browser build mode" >&2; exit 2; }
[[ "$BROWSER" == 0 || "$ARCH" == x86_64 ]] || { echo "Browser runtime is x86_64 only" >&2; exit 2; }

PHASE16_ACCEPTANCE_PROBES="${NOVA_PHASE16_ACCEPTANCE_PROBES:-0}"
[[ "$PHASE16_ACCEPTANCE_PROBES" == 0 || "$PHASE16_ACCEPTANCE_PROBES" == 1 ]] || {
  echo "Invalid NOVA_PHASE16_ACCEPTANCE_PROBES (expected 0 or 1)" >&2
  exit 2
}
STEP3_ADVERSARIAL="${NOVA_PHASE18_STEP3_ADVERSARIAL:-0}"
[[ "$STEP3_ADVERSARIAL" == 0 || "$STEP3_ADVERSARIAL" == 1 ]] || { echo "Invalid NOVA_PHASE18_STEP3_ADVERSARIAL" >&2; exit 2; }
STEP4_CATALOG_PROBES="${NOVA_PHASE18_STEP4_PROBES:-0}"
[[ "$STEP4_CATALOG_PROBES" == 0 || "$STEP4_CATALOG_PROBES" == 1 ]] || { echo "Invalid NOVA_PHASE18_STEP4_PROBES" >&2; exit 2; }
PHASE18_ACCEPTANCE_PROBES="${NOVA_PHASE18_ACCEPTANCE_PROBES:-0}"
[[ "$PHASE18_ACCEPTANCE_PROBES" == 0 || "$PHASE18_ACCEPTANCE_PROBES" == 1 ]] || { echo "Invalid NOVA_PHASE18_ACCEPTANCE_PROBES" >&2; exit 2; }
STEP7_PROBES="${NOVA_PHASE18_STEP7_PROBES:-0}"
[[ "$STEP7_PROBES" == 0 || "$STEP7_PROBES" == 1 ]] || { echo 'Invalid NOVA_PHASE18_STEP7_PROBES' >&2; exit 2; }
STEP6_PROBES="${NOVA_PHASE18_STEP6_PROBES:-0}"
[[ "$STEP6_PROBES" == 0 || "$STEP6_PROBES" == 1 ]] || { echo 'Invalid NOVA_PHASE18_STEP6_PROBES' >&2; exit 2; }
STEP5_PROBES="${NOVA_PHASE18_STEP5_PROBES:-0}"
[[ "$STEP5_PROBES" == 0 || "$STEP5_PROBES" == 1 ]] || { echo "Invalid NOVA_PHASE18_STEP5_PROBES" >&2; exit 2; }
PHASE17_ACCEPTANCE_PROBES="${NOVA_PHASE17_ACCEPTANCE_PROBES:-0}"
[[ "$PHASE17_ACCEPTANCE_PROBES" == 0 || "$PHASE17_ACCEPTANCE_PROBES" == 1 ]] || {
  echo "Invalid NOVA_PHASE17_ACCEPTANCE_PROBES (expected 0 or 1)" >&2
  exit 2
}
if [[ "$STEP3_ADVERSARIAL" == 1 && "$PHASE18_ACCEPTANCE_PROBES" != 1 ]]; then
  echo "Step-3 adversarial mode requires Phase-18 acceptance probes" >&2
  exit 2
fi
if [[ "$STEP4_CATALOG_PROBES" == 1 && ( "$PHASE18_ACCEPTANCE_PROBES" != 1 || "$STEP3_ADVERSARIAL" == 1 ) ]]; then
  echo "Step-4 catalog probes require Phase-18 acceptance-only without Step-3 adversarial" >&2
  exit 2
fi
if [[ "$STEP5_PROBES" == 1 && ( "$PHASE18_ACCEPTANCE_PROBES" != 1 ||
      "${NOVA_PHASE18_STEP3_ADVERSARIAL:-0}" != 0 || "${NOVA_PHASE18_STEP4_PROBES:-0}" != 0 ) ]]; then
  echo 'Step-5 probes require Phase-18 acceptance mode exclusively' >&2
  exit 2
fi
if [[ "$STEP7_PROBES" == 1 && ( "$PHASE18_ACCEPTANCE_PROBES" != 1 ||
      "$STEP6_PROBES" != 0 || "${NOVA_PHASE18_STEP5_PROBES:-0}" != 0 ||
      "${NOVA_PHASE18_STEP4_PROBES:-0}" != 0 ||
      "${NOVA_PHASE18_STEP3_ADVERSARIAL:-0}" != 0 ||
      "${NOVA_PHASE16_ACCEPTANCE_PROBES:-0}" != 0 ||
      "${NOVA_PHASE17_ACCEPTANCE_PROBES:-0}" != 0 ) ]]; then
  echo 'Step-7 probes require Phase-18 acceptance-only mode' >&2
  exit 2
fi
if [[ "$STEP6_PROBES" == 1 && ( "$PHASE18_ACCEPTANCE_PROBES" != 1 ||
      "$STEP5_PROBES" != 0 || "${NOVA_PHASE18_STEP4_PROBES:-0}" != 0 ||
      "${NOVA_PHASE18_STEP3_ADVERSARIAL:-0}" != 0 ) ]]; then
  echo 'Step-6 probes require Phase-18 acceptance mode exclusively' >&2
  exit 2
fi
if (( PHASE16_ACCEPTANCE_PROBES + PHASE17_ACCEPTANCE_PROBES + PHASE18_ACCEPTANCE_PROBES > 1 )); then
  echo "Phase-16 and Phase-17 acceptance probes are mutually exclusive" >&2
  exit 2
fi

case "$ARCH" in
  arm64) TARGET=aarch64-linux-gnu; ARCH_MACRO=aarch64 ;;
  x86_64) TARGET=x86_64-linux-gnu; ARCH_MACRO=x86_64 ;;
  *) echo "Unsupported ARCH: $ARCH" >&2; exit 2 ;;
esac

CLANG="${CLANG:-clang}"
OUT="$ROOT/build/$ARCH"
ROOTFS="$OUT/rootfs"
# Phase-18 Step-3 R3: fail closed if an acceptance-only rootfs is reused as
# a normal image. NEVER silently carry probe executables/service manifests
# into an image with acceptance flags disabled. Use a fresh build tree.
if [[ "$PHASE16_ACCEPTANCE_PROBES" == 0 &&
      "$PHASE17_ACCEPTANCE_PROBES" == 0 &&
      "$PHASE18_ACCEPTANCE_PROBES" == 0 ]]; then
  for probe_path in \
    "$ROOTFS"/System/Apps/nova-security-probe-* \
    "$ROOTFS"/System/Apps/nova-sandbox-probe-* \
    "$ROOTFS"/System/Apps/nova-resource-probe-* \
    "$ROOTFS"/System/Services/phase16-*.service \
    "$ROOTFS"/System/Services/phase17-*.service \
    "$ROOTFS"/System/Services/phase18-*.service; do
    if [[ -e "$probe_path" || -L "$probe_path" ]]; then
      printf 'FAIL-CLOSED: acceptance-only artifact in normal build: %s\n' "$probe_path" >&2
      echo 'Use a fresh isolated build tree; no existing artifacts were removed.' >&2
      exit 2
    fi
  done
  if [[ -f "$ROOTFS/System/Core/nova-resource-broker" ]] &&
     LC_ALL=C grep -aFq 'step3_fixture_mode=adversarial-only' "$ROOTFS/System/Core/nova-resource-broker"; then
    echo 'FAIL-CLOSED: acceptance-only resource-broker binary in normal build.' >&2
    exit 2
  fi
fi
# R44: after fail-closed probe checks, retain the entire prior staging tree.
# cp -a merges directories and otherwise keeps retired files in signed images.
python3 "$ROOT/scripts/retain-build-files.py" "$ROOTFS"
mkdir -p "$ROOTFS/System/Core" "$ROOTFS/System/Apps" "$OUT/obj"
# R25 private terminal helper + source-available static command bundle.
python3 "$ROOT/scripts/build-r25-terminal.py" "$ARCH"
cp -a "$ROOT/build/$ARCH/r25-terminal/System/Terminal" "$ROOTFS/System/"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-terminal-session" "$ROOTFS/System/Core/nova-terminal-session"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-user-workspace" "$ROOTFS/System/Core/nova-user-workspace"

TRUST_PROFILE="${NOVA_TRUST_PROFILE:-development}"
TRUST_KEY_ID="${NOVA_TRUST_KEY_ID:-nova-development-root-2026}"
[[ "$TRUST_PROFILE" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid NOVA_TRUST_PROFILE" >&2; exit 2; }
[[ "$TRUST_KEY_ID" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid NOVA_TRUST_KEY_ID" >&2; exit 2; }
ROOT_CERT_DER="$("$ROOT/scripts/prepare-trust.sh" "$ARCH")"
ROOT_CERT_SHA256="$(sha256sum "$ROOT_CERT_DER" | awk '{print $1}')"
UEFI_CERT_PEM="$($ROOT/scripts/uefi/prepare-uefi-trust.sh "$ARCH")"
UEFI_CERT_DER="$ROOT/build/$ARCH/uefi-trust/uefi-db.crt.der"
UEFI_CERT_SHA256="$(sha256sum "$UEFI_CERT_DER" | awk '{print $1}')"

BUILD_ID="$(python3 "$ROOT/tools/r24-build-id.py")"
COMMON=(
  "-DNOVA_R47_BROWSER_COMPAT=$BROWSER"
  "-DNOVA_R24_OPTIMIZED=${NOVA_R24_OPTIMIZED:-1}"
  "-DNOVA_BUILD_ID=\"$BUILD_ID\""
  --target="$TARGET"
  -ffreestanding -fno-builtin -fno-stack-protector -fno-pic
  -nostdlib -static -Os
  -Wall -Wextra -Werror
  -I"$ROOT/src/runtime"
  -DNOVA_VERSION=\"$VERSION\"
  -DNOVA_BUILD_ARCH=\"$ARCH_MACRO\"
  -DNOVA_ROOT_CERT_SHA256=\"$ROOT_CERT_SHA256\"
  -DNOVA_TRUST_PROFILE=\"$TRUST_PROFILE\"
  -DNOVA_TRUST_KEY_ID=\"$TRUST_KEY_ID\"
  -DNOVA_UEFI_CERT_SHA256=\"$UEFI_CERT_SHA256\"
  -DNOVA_RELEASE_KEY_ID=\"${NOVA_RELEASE_KEY_ID:-nova-development-release-2026-a}\"
  -DNOVA_TRUST_EPOCH=${NOVA_TRUST_EPOCH:-1}
)

build_native() {
  local source="$1"
  local output="$2"
  shift 2
  "$CLANG" "${COMMON[@]}" \
    "$ROOT/src/init/start.S" \
    "$ROOT/src/runtime/nova_runtime.c" \
    "$ROOT/src/runtime/nova_font.c" \
    "$ROOT/src/runtime/nova_textedit.c" \
    "$ROOT/src/runtime/nova_widget.c" \
    "$ROOT/src/runtime/nova_sha256.c" \
    "$ROOT/src/runtime/nova_tcg2.c" \
    "$@" \
    "$source" \
    -fuse-ld=lld \
    -Wl,-e,_start -Wl,--build-id=none -Wl,-z,noexecstack \
    -o "$output"
  chmod 0755 "$output"
  echo "Built: $output"
}

build_native "$ROOT/src/init/main.c" "$ROOTFS/init" "$ROOT/src/runtime/nova_tpm2.c"
build_native "$ROOT/src/services/heartbeat.c" "$ROOTFS/System/Core/nova-heartbeat"
build_native "$ROOT/src/services/compositor.c" "$ROOTFS/System/Core/nova-compositor"
build_native "$ROOT/src/services/assets.c" "$ROOTFS/System/Core/nova-assets"
STEP3_TEST_DEFINES=()
if [[ "$STEP3_ADVERSARIAL" == 1 ]]; then
  STEP3_TEST_DEFINES=(-DNOVA_PHASE18_STEP3_ADVERSARIAL=1)
  echo "ACCEPTANCE-ONLY: Phase-18 Step-3 adversarial fixtures enabled (NEVER release this image)"
fi
build_native "$ROOT/src/services/resource_broker.c" "$ROOTFS/System/Core/nova-resource-broker" "${STEP3_TEST_DEFINES[@]}"
build_native "$ROOT/src/services/shell.c" "$ROOTFS/System/Core/nova-shell"
build_native "$ROOT/src/services/demo_app.c" "$ROOTFS/System/Apps/nova-demo-app" "$ROOT/files/src/nova_files_font.c"
build_native "$ROOT/src/services/demo_app.c" "$ROOTFS/System/Apps/nova-demo-app-b" -DNOVA_DEMO_APP_B=1 "$ROOT/files/src/nova_files_font.c"
# R16 native Files: same freestanding runtime and verified rootfs chain.
build_native "$ROOT/files/src/nova_files_app.c" "$ROOTFS/System/Apps/nova-files" \
  -I"$ROOT/files/src" "$ROOT/files/src/nova_files_core.c" "$ROOT/files/src/nova_files_ui.c" \
  "$ROOT/files/src/nova_files_font.c" "$ROOT/files/src/nova_files_surface_bridge.c" \
  "$ROOT/files/src/nova_files_memory.c"


if [[ "$PHASE16_ACCEPTANCE_PROBES" == 1 ]]; then
  PROBE_SRC="$ROOT/tests/fixtures/stage5a-phase16/security_probe.c"
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-security-probe-read" -DNOVA_SECURITY_PROBE_READ_ONLY=1
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-security-probe-no-ui" -DNOVA_SECURITY_PROBE_NO_UI=1
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-security-probe-write" -DNOVA_SECURITY_PROBE_WRITE_ONLY=1
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-security-probe-unregistered" -DNOVA_SECURITY_PROBE_UNREGISTERED=1
fi

if [[ "$PHASE17_ACCEPTANCE_PROBES" == 1 ]]; then
  PROBE_SRC="$ROOT/tests/fixtures/stage5a-phase17/sandbox_probe.c"
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-sandbox-probe-a" -DNOVA_SANDBOX_PROBE_A=1
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-sandbox-probe-b" -DNOVA_SANDBOX_PROBE_B=1
fi
if [[ "$STEP7_PROBES" == 1 ]]; then
  PROBE_SRC="$ROOT/tests/fixtures/stage5a-phase18-step7/idle_stress_probe.c"
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-step7-idle-stress"
  echo 'ACCEPTANCE-ONLY: Phase-18 Step-7 real-guest monotonic idle session probe built'
elif [[ "$STEP6_PROBES" == 1 ]]; then
  PROBE_SRC="$ROOT/tests/fixtures/stage5a-phase18-step6/session_stress_probe.c"
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-step6-session-stress"
  echo 'ACCEPTANCE-ONLY: Phase-18 Step-6 broker session saturation probe built'
elif [[ "$STEP5_PROBES" == 1 ]]; then
  PROBE_SRC="$ROOT/tests/fixtures/stage5a-phase18-step5/role_probe.c"
  for role_spec in demo:1 notes:2 both:3 none:4; do
    role="${role_spec%%:*}"
    id="${role_spec##*:}"
    build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-resource-probe-$role" \
      -DNOVA_STEP5_ROLE="$id" -DNOVA_RESOURCE_PROBE_GRANTED=0
  done
  echo 'ACCEPTANCE-ONLY: Phase-18 Step-5 four independent capability probes built'
elif [[ "$PHASE18_ACCEPTANCE_PROBES" == 1 ]]; then
  PROBE_SRC="$ROOT/tests/fixtures/stage5a-phase18/resource_probe.c"
  if [[ "$STEP4_CATALOG_PROBES" == 1 ]]; then
    PROBE_SRC="$ROOT/tests/fixtures/stage5a-phase18/catalog_probe.c"
    echo "ACCEPTANCE-ONLY: Phase-18 Step-4 numeric catalog probes enabled"
  fi
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-resource-probe-grant" -DNOVA_RESOURCE_PROBE_GRANTED=1 "${STEP3_TEST_DEFINES[@]}"
  build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-resource-probe-deny" -DNOVA_RESOURCE_PROBE_GRANTED=0 "${STEP3_TEST_DEFINES[@]}"
fi


cp -a "$ROOT/build/$ARCH/r25-terminal/System/Network" "$ROOTFS/System/"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-net-broker" "$ROOTFS/System/Core/nova-net-broker"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-net-setup" "$ROOTFS/System/Core/nova-net-setup"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-software-session" "$ROOTFS/System/Core/nova-software-session"

# R44 signed System footprint: only unused symbols, with PT_LOAD invariants.
python3 "$ROOT/tools/r44-strip-runtime.py" "$ARCH"

# Signed-System browser runtime; source preparation never signs by itself.
if [[ "$BROWSER" == 1 ]]; then
  bash "$ROOT/tools/r47-build-native-bridge.sh"
  python3 "$ROOT/tools/r47-stage-browser-runtime.py"
  python3 "$ROOT/tools/r47-install-browser-runtime.py" "$ROOTFS"
fi

# Optional R59 downloader: separate runtime; no inherited broad app network grant.
if [[ "${NOVA_R59_INTERNET_STORE:-0}" == 1 ]]; then
  python3 "$ROOT/tools/r59-build-store.py" "$ARCH"
fi
