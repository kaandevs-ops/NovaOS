# Media provenance

## Existing author banner

`docs/media/banner.png` is copied unchanged from [the author's profile repository](https://github.com/kaandevs-ops/kaandevs-ops/blob/main/banner.png), at the author's request. The same file is used by [unity_multiplayer](https://github.com/kaandevs-ops/unity_multiplayer/blob/main/docs/banner.png). Git blob ID: `5a873b47dcf3a90d95a3733d34e781aa4dce2f9c`. It is author branding, not an OS screenshot. No replacement banner was generated for this revision.

## Native screenshots

The seven `*.HOST-NOT-VM.png` frames were generated from this public tree's production `src/services/compositor.c` using `tools/render-public-gallery.c`. They are real CPU framebuffer outputs, not HTML reconstructions or AI mockups. Resolution: 1600 × 900. The existing procedural Graphite wave background is selected; no inherited photograph is used.

The harness sets bounded host UI state and changes themes using the real theme setter. It does not start a guest, authenticate an application, install a package or run DRM/KMS presentation. The Software frame shows the honest no-peer/waiting state. No connected application state or system telemetry was invented.

Reproduce the lossless framebuffer exports on Linux:

```sh
bash scripts/render-public-gallery.sh
```

Outputs are in ignored `build/public-gallery/`. `tools/export-native-framebuffer.py` converts PPM to PNG without visual retouching. Framebuffer captures were visually inspected before publication.

## Walkthrough video and GIF

The MP4 is a 21-second edited sequence of the seven actual renderer stills, three seconds per state. It is encoded as H.264/yuv420p at 1600 × 900, with no audio. The GIF is an 800-pixel-wide, one-frame-per-second preview of the same sequence. Neither is a continuous interaction recording or a performance benchmark. The container frame rate does not measure the OS renderer or a guest screen.

Example encoding from a concat list of the seven PNGs, each with `duration 3`:

```sh
ffmpeg -f concat -safe 0 -i sequence.txt -t 21 -r 24 \
  -c:v libx264 -pix_fmt yuv420p -crf 20 -movflags +faststart walkthrough.mp4
ffmpeg -i walkthrough.mp4 -vf 'fps=1,scale=800:-1:flags=lanczos' \
  -loop 0 preview.gif
```

Exact file sizes and SHA-256 are in [`media/manifest.json`](media/manifest.json). Historical private VirtualBox photographs, class materials and remote-Windows screenshots are not published as evidence of this modified public source tree.
