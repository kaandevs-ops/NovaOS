#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/public-gallery
clang -std=c11 -O2 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation \
  -ffunction-sections -fdata-sections -Isrc/runtime \
  tools/render-public-gallery.c src/runtime/nova_runtime.c src/runtime/nova_font.c \
  -Wl,--gc-sections -o build/public-gallery/render
build/public-gallery/render build/public-gallery
for frame in build/public-gallery/*.ppm; do
  python3 tools/export-native-framebuffer.py "$frame" "${frame%.ppm}.png"
done
