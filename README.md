<p align="center"><img src="docs/media/banner.png" alt="Kaan Tekin — AI, Robotics and Autonomous Systems" width="100%"></p>

<h1 align="center">NovaOS</h1>

<p align="center"><strong>An independent desktop and userspace project on the Linux kernel.</strong><br>Native C compositor · own shell and service manager · scoped application integration</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/original_code-Apache_2.0-9cd5c2?style=flat-square" alt="Original code: Apache 2.0"></a>
  <img src="https://img.shields.io/badge/status-experimental-e6c787?style=flat-square" alt="Status: experimental">
  <img src="https://img.shields.io/badge/core-C-9dc5dc?style=flat-square" alt="Core: C">
  <a href="https://github.com/kaandevs-ops/NovaOS/actions/workflows/components.yml"><img src="https://github.com/kaandevs-ops/NovaOS/actions/workflows/components.yml/badge.svg" alt="Component checks"></a>
</p>

<p align="center"><a href="README.tr.md">Türkçe</a> · <a href="#the-project">Project</a> · <a href="#explore-the-source">Source</a> · <a href="#quick-start">Build checks</a> · <a href="docs/ARCHITECTURE.md">Architecture</a> · <a href="docs/ROADMAP.md">Roadmap</a></p>

> **Development source, with explicit boundaries.** This repository is a curated, classroom-free source snapshot. It is not a bootable VM download or a production-ready operating system. Linux and integrated upstream programs are not authored by NovaOS. The banner is the author's existing profile banner, also used in the Unity repository.

## Screenshots and video

These are **actual outputs from this repository's native C CPU renderer on a Linux host**, at 1600 × 900, using the existing procedural Graphite wave background. They are not VirtualBox screenshots or HTML mockups. This host gallery has no connected application processes; Software deliberately shows a waiting state rather than invented installation results.

[![Native renderer walkthrough: edited sequence of seven frames](docs/media/native-renderer-preview.HOST-NOT-VM.gif)](docs/media/native-renderer-walkthrough.HOST-NOT-VM.mp4)

**[Watch/download the 21-second renderer walkthrough](docs/media/native-renderer-walkthrough.HOST-NOT-VM.mp4)** · [Animated preview](docs/media/native-renderer-preview.HOST-NOT-VM.gif) · [Capture provenance and reproduction](docs/MEDIA.md)

The video is an edited sequence of seven native still frames, held for readability. It is not a continuous interaction recording, FPS benchmark or proof of guest performance.

| Desktop | Application menu |
| --- | --- |
| ![Desktop: native host output](docs/media/01-desktop.HOST-NOT-VM.png) | ![Application menu: native host output](docs/media/02-applications.HOST-NOT-VM.png) |

| Calm Light | Graphite Pro |
| --- | --- |
| ![Light appearance: native host output](docs/media/04-calm-light.HOST-NOT-VM.png) | ![Dark appearance: native host output](docs/media/05-graphite.HOST-NOT-VM.png) |

| Quick Settings | Software, no application peer |
| --- | --- |
| ![Quick Settings: native host output](docs/media/06-quick-settings.HOST-NOT-VM.png) | ![Software waiting state: native host output](docs/media/07-software-no-peer.HOST-NOT-VM.png) |

## Feature guide

| Capability | Implemented scope |
| --- | --- |
| **Windows and input** | Focus/z-order, move/resize, minimize/maximize/restore, close/reopen paths and supported keyboard switching |
| **Desktop workspace** | Folder opening, bounded text preview, context menu, selection, workspace trash/restore and collision handling |
| **Nova Files** | Scoped read-only browsing, back/forward history, breadcrumbs, filtering, name/type/size sorting and available metadata |
| **Terminal** | Real session/child-process integration, working-directory/argument support, Turkish-Q Shift/Caps/AltGr and quoted path insertion |
| **Appearance** | Calm Light, Graphite Pro and Adaptive; shared UI tokens; shell Settings and Quick Settings; session theme events |
| **Utilities** | Calendar, clock and confirmation-based normal-power-off interfaces |
| **Local applications** | SQLite, Lua, dynamic Lua and terminal 2048 catalog; install/open/status/remove paths and installed-app dock integration |
| **Internet packages** | Repository client, policy/verification, bounded transfers and publisher/staging tools; no maintained public store endpoint |
| **Browser / documents** | External Linux browser/X compatibility, document/PDF sources and isolated MuPDF worker; runtime binaries not bundled |
| **Platform** | `novad`, manifests/principals, per-app sandbox/limits, memfd/SCM_RIGHTS/surface validation, network and storage/recovery integration |
| **Diagnostics** | Overview/graphics data paths, CPU render metrics and explicit unavailable states; no inferred guest FPS |

Window management, desktop file operations, Files navigation/metadata, terminal processes/input, themes, clock/calendar, application/package lifecycle, browser/PDF compatibility, network, storage, diagnostics and security are documented in the [complete feature/source/status inventory](docs/FEATURES.md). It includes keyboard entry points, the actual local app catalog and unsupported areas. Source implementation is distinguished from host tests and guest acceptance.

## Bootable VM availability

A private clean experimental VM has historical boot evidence, but launcher/runtime/source-attribution issues are separate from this repository. It is **not currently published as a Release**. [Release blockers and acceptance checklist](docs/VM_RELEASE.md). GitHub's automatically generated source ZIP is not an installable OS.

## The project

NovaOS explores a desktop that feels coherent across its shell, windows, file manager, terminal and application lifecycle. The project develops its own native C desktop components, `novad` PID 1/service integration and rootfs/initramfs build work on top of Linux. It is not an Ubuntu theme or a renamed GNOME/KDE installation.

The longer-term direction is a refined, responsive desktop with practical applications, extensibility and scoped permissions. That is the engineering goal, not a claim that every Linux application, game or device already works.

## Explore the source

| Area | What is here |
| --- | --- |
| **Desktop** | Native compositor, shell, dock, focus/window management and shared UI code |
| **Files** | Directory navigation, filtering/sorting and bounded guest adapters |
| **Terminal** | Session/process support and Turkish keyboard handling |
| **Applications** | Catalog, manifests, software/package lifecycle and browser compatibility integration |
| **Platform** | Service/principal checks, descriptor transfer, sandbox and boot integration sources |
| **Verification** | Host tests, source hygiene audit and x86_64/AArch64 component build checks |

Start with [the source map and architecture](docs/ARCHITECTURE.md). Directories retain historical R-number names where tests and includes depend on them; they are not a promise of separate supported releases.

## Quick start

On Linux or WSL, install Bash, Python 3, GCC, Clang and lld; then:

```sh
git clone https://github.com/kaandevs-ops/NovaOS.git
cd NovaOS
python3 scripts/audit-public.py
bash scripts/check-public.sh
```

This checks selected native components; it does **not** build or launch a complete OS. See [prerequisites and exact scope](docs/BUILDING.md).

## Verification and limits

The public check covers the keyboard test, terminal-session build, Python syntax, compositor/shell cross-target objects, authenticated theme host tests and Files-view host tests. Sanitizers are enabled for the theme/Files tests. Recorded results and scope are in [VALIDATION.md](docs/VALIDATION.md).

Full public rootfs assembly, signed guest boot, physical-hardware acceptance, complete Unicode, GPU acceleration and production security acceptance remain **pending** for this curated snapshot. Historical private guest results are not transferred to a modified public tree as new PASS results.

## What is intentionally absent?

Classroom exercises, slides, student documents, Windows remote-control tools/configuration, personal test evidence, private keys, user data, VM disks and firmware/TPM state are excluded. An inspected synthetic two-page PDF remains only as a renderer test fixture. The inherited undocumented photograph and legacy raster were replaced by neutral source placeholders; the public tree does not reproduce the old VM wallpaper exactly.

There is no general internet app store endpoint or unrestricted Ubuntu package compatibility claim. A clean, reproducible and license-complete bootable release is a separate milestone in [the roadmap](docs/ROADMAP.md).

## License and attribution

Original NovaOS code/documentation: **Apache-2.0**, matching the author's other repositories. Explicit per-file and upstream licenses take precedence. In particular, the MuPDF-linked worker is **AGPL-3.0-or-later**. Read [NOTICE](NOTICE), [licensing boundaries](docs/LICENSING.md) and [third-party notices](THIRD_PARTY_NOTICES.md).

## Contributing

Small, reproducible improvements are welcome. Explain the behavior, run the checks and distinguish host evidence from real guest observations. See [CONTRIBUTING.md](CONTRIBUTING.md) and [SECURITY.md](SECURITY.md).

Created by [Kaan Tekin / kaandevs-ops](https://github.com/kaandevs-ops).
