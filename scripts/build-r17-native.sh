#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-x86_64}"
case "$ARCH" in x86_64) TARGET=x86_64-linux-gnu;; arm64) TARGET=aarch64-linux-gnu;; *) exit 2;; esac
OUT="$ROOT/build/$ARCH/r17-native"
mkdir -p "$OUT/System/Apps" "$OUT/System/Core" "$OUT/System/Services"
python3 "$ROOT/scripts/build-r25-terminal.py" "$ARCH"
cp -a "$ROOT/build/$ARCH/r25-terminal/System/Terminal" "$OUT/System/"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-terminal-session" "$OUT/System/Core/nova-terminal-session"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-user-workspace" "$OUT/System/Core/nova-user-workspace"
BUILD_ID="$(python3 "$ROOT/tools/r24-build-id.py")"
COMMON=("-DNOVA_BUILD_ID=\"$BUILD_ID\"" --target="$TARGET" -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -ffunction-sections -fdata-sections -nostdlib -static -Os -Wall -Wextra -Werror -I"$ROOT/src/runtime")
RUNTIME=("$ROOT/src/init/start.S" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_sha256.c" "$ROOT/src/runtime/nova_tpm2.c" "$ROOT/src/runtime/nova_tcg2.c" "$ROOT/src/runtime/nova_font.c" "$ROOT/src/runtime/nova_textedit.c" "$ROOT/src/runtime/nova_widget.c")
link(){ local source="$1"; local output="$2"; shift 2; clang "${COMMON[@]}" "${RUNTIME[@]}" "$@" "$ROOT/$source" -fuse-ld=lld -Wl,--gc-sections -Wl,-e,_start -Wl,-z,noexecstack -o "$OUT/$output"; }
link src/init/main.c init
for service in compositor shell resource_broker assets heartbeat; do
 name="${service//_/-}";link "src/services/$service.c" "System/Core/nova-$name"
done
link src/services/demo_app.c System/Apps/nova-demo-app "$ROOT/files/src/nova_files_font.c"
link src/services/demo_app.c System/Apps/nova-demo-app-b -DNOVA_DEMO_APP_B=1 "$ROOT/files/src/nova_files_font.c"
bash "$ROOT/scripts/build-r16-files.sh" "$ARCH"
install -m 0755 "$ROOT/build/$ARCH/r16-files/nova-files" "$OUT/System/Apps/nova-files"
for spec in a:demo-app b:demo-app-b files:nova-files; do
 name="${spec%%:*}";manifest="${spec#*:}"
 link tests/r17-launch.c "launch-$name" "-DR17_MANIFEST=\"/System/Services/$manifest.service\""
 install -m 0644 "$ROOT/configs/services/$manifest.service" "$OUT/System/Services/$manifest.service"
done
install -m 0644 "$ROOT/configs/services/services.list" "$OUT/System/Services/services.list"
echo "R17_NATIVE_USERLAND_LINK=PASS $TARGET (unsigned, UNPINNED development links)"

cp -a "$ROOT/build/$ARCH/r25-terminal/System/Network" "$OUT/System/"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-net-broker" "$OUT/System/Core/nova-net-broker"
install -m 0755 "$ROOT/build/$ARCH/r25-terminal/System/Core/nova-net-setup" "$OUT/System/Core/nova-net-setup"
