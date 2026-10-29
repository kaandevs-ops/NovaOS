#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
COMMON=(-std=c11 -O2 -g -Wall -Wextra -Werror -Wno-misleading-indentation -Isrc/runtime -Isrc/compat -Ibuild/browser-host/sysroot/usr/include)
SOURCES=(src/compat/nova_browser_bridge.c src/compat/nova_browser_frame.c src/compat/nova_x11_pixels.c)
clang "${COMMON[@]}" src/compat/nova_browser_main.c "${SOURCES[@]}" -l:libX11.so.6 -l:libXtst.so.6 -o build/browser-host/nova-browser-bridge
clang "${COMMON[@]}" -O1 -ffunction-sections -fdata-sections -fsanitize=address,undefined tests/r47-browser-compositor.c "${SOURCES[@]}" src/runtime/nova_runtime.c src/runtime/nova_font.c -Wl,--gc-sections -l:libX11.so.6 -l:libXtst.so.6 -o build/browser-host/browser-compositor-test

gcc -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -static src/compat/nova_browser_session.c -o build/browser-host/nova-browser-session
