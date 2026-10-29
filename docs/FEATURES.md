# NovaOS feature and platform guide

This inventory describes the implemented source and its boundaries. **Implemented**, **host-tested**, **historically guest-tested** and **accepted in a new public VM** are different states. Only the checks in [VALIDATION.md](VALIDATION.md) are current acceptance evidence for this publication.

## 1. Desktop and window system

| Capability | Source entry point | Scope |
| --- | --- | --- |
| Native CPU composition, panel and dock | `src/services/compositor.c` | Own C drawing path, not a themed external desktop |
| Shell scene and application tasks | `src/services/shell.c` | Task/window state integration |
| Focus, stacking, move and resize | `src/services/compositor.c` | Input routing and constrained window geometry |
| Minimize, maximize and restore | compositor and runtime window/input code | Native client/window lifecycle; not a general external window-manager ABI |
| App icons and running indicators | dock and software client code | Derived from supported session/catalog state |
| Desktop context menu and selection | `src/runtime/nova_desktop_workspace.h` | Supported workspace operations only |
| Folder open and text preview | `src/runtime/nova_desktop_file_ui.h`, desktop/session protocol | Bounded file preview; not an arbitrary document editor |
| Reversible trash and restore | desktop/session helpers | Workspace-scoped; collision/error handling exists |
| Path dragging/insertion | desktop and terminal input code | Supported paths are quoted/validated; not a generic cross-app drag/drop standard |

The public default is the existing procedural **Graphite wave** background. Old undocumented photograph/raster inputs are not supplied as reusable assets. The gallery uses this actual renderer, with no app processes or guest attached.

## 2. Files

The engine and UI live in `files/src`. Features include directory navigation, bounded entry lists, filename filtering, name/type/size sorting, back/forward history, breadcrumb paths, keyboard row navigation, scrolling and available metadata. Guest adapters connect this UI to scoped directory authority and surface IPC.

The Files access model remains read-only and bounded. It does not grant unrestricted filesystem access. Desktop trash/workspace operations use their own controlled session path; they do not imply a general Files write/delete permission. Permissions, unavailable metadata, long names and finite directory capacities must remain visible failure/bounds cases rather than fabricated results.

## 3. Terminal and command execution

`src/terminal` contains session, shell/package helper and terminal integration code. This is a real process/filesystem path, not static command output. The runtime handles a grid, keyboard events, session state and supported child programs. Turkish-Q handling includes Shift/Caps/AltGr and punctuation paths.

When integrated in a built guest with its userspace tools, examples such as `pwd`, `ls`, `cat` and `grep` operate inside that guest. They are not installed on the host by cloning this repository. Available commands and privileges depend on the rootfs/tool bundle, UID and per-session limits. "Every Linux command works" is not an accepted claim.

The public copy removes the classroom-specific privileged worker channels. It does not ship the course exercises, slides, personal home or remote Windows setup.

## 4. Settings and utilities

- **Calm Light / Graphite Pro / Adaptive:** shared palette and component state.
- **Nova Settings:** shell-integrated appearance, Overview and Graphics sections; not an extra unbounded application connection.
- **Quick Settings:** theme choices, actual available session state and normal-power-off entry.
- **Theme events:** authenticated subscription/state mechanisms and generation/replay checks; host negative tests cover spoofed/invalid/replayed events and closed/backpressured peers.
- **Preference lifetime:** theme choice is session-only. Persistent settings are not claimed.
- **Calendar and clock:** shared native views in `src/apps/nova_calendar.h` and `nova_time_view.h`.
- **Power off:** confirmation and authorized normal-shutdown path. Rendering its button is not a new guest shutdown acceptance test.

## 5. Applications and software catalog

The actual local manifest is [`configs/apps/local-packages.json`](../configs/apps/local-packages.json).

| Catalog item | Command | Kind |
| --- | --- | --- |
| SQLite | `sqlite3` | Local SQL/CLI program |
| Lua | `lua` | Isolated script runtime |
| Lua dynamic runtime | `lua-dynamic` | Dynamic-runtime integration |
| 2048 | `2048` | Real terminal puzzle, not a general game platform |

Graphical Software sources expose supported install/open/remove/status flows and installed-app dock integration. `src/software` adds repository metadata/policy, bounded transfers and package verification. Publisher/staging utilities are in `tools/nova_repository_publish.py` and related package tools.

The public repository has **no maintained general internet store endpoint** and does not bundle executable packages. Existing test repositories do not establish a global production store. Arbitrary DEB/RPM installation, Steam/Proton, commercial Windows games and "any application" support remain outside accepted scope.

Attributed Tetris web source and a local-file music page are in `assets/webapps`. They do not establish a video/audio hardware acceptance result or distribution rights to music files; no music is bundled.

## 6. Browser and document integration

`src/compat` and browser/session build references support integration of an external Linux browser using a private X-compatible graphics path. Mozilla Firefox remains an upstream program, not a NovaOS-authored browser. No browser binary/profile, password, certificate exception or first-use consent is shipped in this source repository.

`src/pdf` contains a PDF client, protocol and bounded worker integration. The MuPDF-linked worker retains its explicit AGPL notice. A synthetic two-page PDF fixture is included for renderer tests; course/personal PDFs are not.

Browser setup, TLS policy, available codecs, audio, accelerated graphics and executable compatibility require a complete guest/runtime. Source presence is not evidence of unrestricted YouTube/media/browser functionality.

## 7. OS and platform engineering

| Area | Source | Boundary |
| --- | --- | --- |
| PID 1 / boot and service control | `src/init/main.c`, service configs | Linux kernel underneath; own userspace initialization |
| Rootfs/initramfs integration | `scripts`, `configs` | Additional build inputs required |
| Principal/generation and app manifests | init/runtime/catalog code | No automatic authority for arbitrary binaries |
| Surface protocol and connection resources | runtime and compositor | Authenticated scoped app resources and finite capacity |
| Memfd/SCM_RIGHTS transfer | runtime and adapters | Descriptor ownership/type/size validation paths |
| Sandbox policies and limits | runtime, terminal and package helpers | Least-privilege intent; no production security guarantee |
| Network setup/broker/HTTP/TLS | `src/network` | Bounded integration; not a full network-control desktop |
| User workspace / storage discovery | `src/init/nova_home_directory_worker.inc`, `phase19_storage_discovery.inc` | Development storage model; no public data-migration promise |
| A/B/recovery/boot verification | storage/recovery configs and init/runtime sources | Complete new image acceptance pending |
| Overview and graphics diagnostics | compositor and `src/init/r24_diagnostics.inc` | Unavailable data must stay unavailable; host metrics are not guest FPS |
| Font/input/UI support | `src/runtime`, licensed `assets/fonts` | Turkish subset is not complete Unicode/shaping/accessibility support |

## 8. Keyboard entry points in the compositor

| Shortcut | Intended action |
| --- | --- |
| `Ctrl+Alt+T` | Show/focus or toggle the integrated terminal |
| `Ctrl+Alt+S` | Appearance settings |
| `Ctrl+Alt+I` | System Overview |
| `Ctrl+Alt+Q` | Quick Settings |
| Meta key | Application menu |
| `Alt+Tab` | Supported focus/application switching |
| Escape | Dismiss the current supported menu/overlay |

These describe source routes. A virtual-machine host can intercept shortcuts, and application capture/focus matters. They are not a claim that every guest/host keyboard combination has passed this publication's acceptance tests.

## 9. What remains before daily use?

Reproducible full builds and licensed binary distribution; maintained package repositories; wider runtime/driver support; persistent preferences; audio/device management; accessibility/text shaping; real guest latency/present measurements; updates/recovery acceptance and long-duration security/resource testing. See [ROADMAP.md](ROADMAP.md).
