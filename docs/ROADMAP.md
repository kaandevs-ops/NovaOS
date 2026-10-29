# Engineering roadmap

## Reproducible source-to-guest build

- Replace stage-specific scripts with a dependency lock and isolated build entry point.
- Map runtime binaries to source, license and build/relink materials.
- Restore photo/icon assets only with verifiable provenance, or create original replacements.
- Produce a separately approved clean guest and record boot/input/shutdown against its exact source hash.

## Daily-use foundations

- Maintained signed network repository, interrupted install and rollback coverage.
- Broader application compatibility without generic filesystem/process permissions.
- Persistent preferences with atomic updates and recovery tests.
- Terminal ergonomics, text input and desktop file operations.
- Audio, networking/device management and accessibility acceptance.

## Performance and releases

- Paired host/guest input-to-render and present measurements.
- Damage tracking and idle redraw reductions with unchanged security/visual tests.
- Explicit GPU/HiDPI acceptance; no assumed 60 FPS guarantee.
- Security updates, reproducible artifacts and real hardware coverage before production readiness.
