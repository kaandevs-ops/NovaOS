#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="6.18.54"
SHA256="9df30b02dd8102bbd0be52556288ef6889ddbe7f1ddb96fbf847d0becf3eacac"
CACHE="$ROOT/build/cache"
SRC="$ROOT/build/src"
ARCHIVE="$CACHE/linux-$VERSION.tar.xz"
URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-$VERSION.tar.xz"

mkdir -p "$CACHE" "$SRC"
if [[ ! -f "$ARCHIVE" ]]; then
  curl --fail --location --proto '=https' --tlsv1.2 "$URL" -o "$ARCHIVE"
fi

echo "$SHA256  $ARCHIVE" | sha256sum -c -

if [[ ! -d "$SRC/linux-$VERSION" ]]; then
  tar -C "$SRC" -xf "$ARCHIVE"
fi

echo "$SRC/linux-$VERSION"
