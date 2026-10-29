#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"
P="$ROOT/scripts/windows/stage5a-phase4-check.py"
for marker in \
  'rendered_visual_generation' 'presented_visual_generation' 'visual_generation' \
  'buffers[back].rendered_visual_generation = g_scene.visual_generation' \
  'g_render.presented_visual_generation=buffers[front].rendered_visual_generation' \
  'put_key_u64("presented_visual_generation",g_render.presented_visual_generation)'; do
  grep -Fq "$marker" "$C" || { echo "missing presented-scene compositor marker: $marker" >&2; exit 1; }
done
for marker in \
  "def parse_state_chunk" "def read_graphics_state" "def wait_graphics_state" \
  "def wait_visual_presented" "cat /run/nova/graphics.state" \
  "wait_visual_presented(proc,buf,sel,'initial shell scene scanout'" \
  "wait_visual_presented(proc,buf,sel,'focused scene scanout'" \
  "wait_visual_presented(proc,buf,sel,'moved-window scene scanout'" \
  "wait_visual_presented(proc,buf,sel,'cursor scene scanout'" \
  "wait_visual_presented(proc,buf,sel,'restarted shell scene scanout'"; do
  grep -Fq "$marker" "$P" || { echo "missing presented-scene acceptance marker: $marker" >&2; exit 1; }
done
echo 'Stage-5A Phase-4 visual-generation/presented-scanout synchronization contract: PASS'
