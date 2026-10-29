# Bootable VM release status

**No bootable VM ZIP is currently published from this repository.** The automatically generated GitHub source archives are not OS installers.

An older clean experimental VM was booted privately. A subsequent redistribution review found specific issues that remain distinct from the source repository. Lab/personal files being absent and a VM booting successfully do not resolve these items.

| Item | Current state | Required before public VM upload |
| --- | --- | --- |
| Original code license | Apache-2.0 in this source repo | Match the exact binary sources; preserve per-file exceptions |
| Old launcher medium-only UUID collision | Fixed/tested in later presentation packaging | Integrate and test in a new clean package |
| Photograph/legacy raster provenance | Unknown old assets excluded from source | Owner selected existing procedural Graphite wave for the new public image; rebuild/retest that image |
| MuPDF linked worker | Explicit AGPL source notice preserved | Exact combined-work/corresponding-source compliance for binaries |
| glibc/runtime source and relink materials | Incomplete old binary inventory | Complete exact-version source/build/relink delivery |
| Kernel/libxcb and external libraries | Source/build mapping not complete for old VM | Match all shipped binaries, patches and notices |
| Firefox | Older version in old VM | Review/update official distribution and complete guest compatibility tests |
| User home | Old test artifact trace found in raw filesystem | Build a new empty home; keep test-generated files outside release data |
| Guest acceptance | Historical evidence only | New exact-source boot, security chain, input, apps, shutdown, extracted-ZIP/UUID tests |

The author requested a public clean VM. The old archive is intentionally not uploaded as a supposedly compliant or current release. No new signing keys, GPT/ESP, firmware/TPM or VM state were created for this documentation/media update. Those operations require the separate isolated acceptance process established for this project.

## Planned downloadable layout

Once the above gates pass, a pre-release should include the clean VM ZIP, checksums, exact source/version mapping, third-party notices and corresponding source/build materials. It should explain VirtualBox/host prerequisites, RAM, extraction/launcher usage, test certificate/TPM scope, normal shutdown and known limits. The package must not include private signing keys, personal files, class exercises, Windows remote tools or test logs.

There is no claim of a physical-PC installer, production Secure Boot trust or broad hardware support.
