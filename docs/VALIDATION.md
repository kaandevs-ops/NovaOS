# Public source validation

Date: 29 October 2026. These results apply to this curated source publication, not to an unmodified historical VM.

| Check | Result | Boundary |
| --- | --- | --- |
| Bounded payload/credential/path audit | PASS | Includes source archives; pattern checks are not a universal secret detector |
| Turkish keyboard program | PASS | Native host test |
| Terminal session | BUILD PASS | GCC warnings treated as errors; not a complete terminal interaction test |
| Python build-script syntax | PASS | Parse only; historical scripts are not all executed |
| Compositor and shell, x86_64 | BUILD PASS | Freestanding object compilation |
| Compositor and shell, AArch64 | BUILD PASS | Cross-target objects; not ARM guest boot |
| Authenticated theme protocol | PASS | Host integration with ASan/UBSan; replay, spoof, backpressure, closed-peer cases |
| Files directory/filter/sort/bounds | PASS | Host test with ASan/UBSan |
| Complete public rootfs and guest boot | PENDING | Requires separate image build and acceptance |
| Real hardware and production readiness | PENDING | Not inferred from source checks |

The first payload audit rejected a synthetic PDF fixture by extension. Its plain test content was inspected; only the exact SHA-256 of `tests/fixtures/r44-two-pages.pdf` is allowlisted. Other PDF/presentation documents remain rejected. No test expectation was relaxed to conceal a component failure.

## Source provenance

The publication derives from the classroom-free R60 clean export. Compared read-only with the latest R62 development tree, **126 of 128 files under `src/` are byte-identical**. The two intentional differences are `src/services/nova_r12_assets.h` (neutral replacement for undocumented image/raster) and `src/terminal/nova_terminal_session.c` (classroom worker integration removed). R62's presentation-only privacy pages are intentionally excluded.

Therefore this is a current-core, curated **source snapshot**, not a byte-for-byte R62 VM or a newly accepted clean guest. Historical personal evidence and VM screenshots are not imported into this repository.

## Reproduce

```sh
python3 scripts/audit-public.py
bash scripts/check-public.sh
```

GitHub Actions executes the same checks on push and pull request. Read the workflow run for the actual remote result; a configured workflow alone is not a PASS.
