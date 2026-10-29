# Build and test the published components

Use Linux or WSL with Bash, Python 3, GCC, Clang and lld. On Ubuntu:

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends clang lld gcc python3
git clone https://github.com/kaandevs-ops/NovaOS.git
cd NovaOS
python3 scripts/audit-public.py
bash scripts/check-public.sh
```

The audit examines the public source payload, including archive members. The component check builds/runs the Turkish keyboard test, builds the terminal session, parses Python scripts, compiles compositor/shell objects for x86_64 and AArch64, then runs theme and Files view tests with AddressSanitizer/UndefinedBehaviorSanitizer.

Outputs stay in ignored `build/`. Cross-target object compilation does not prove an ARM64 guest boot. Host integration tests are not a compositor displayed in a VM.

## Full OS images

Historical build scripts are engineering references with additional source, toolchain and staging requirements. They are **not** a verified one-command public OS installer. No signed disk, browser runtime, private key or firmware/TPM state is supplied. Never point development scripts at an accepted image or existing VM.

Optional upstream MuPDF source fetch, not required by the component checks:

```sh
python3 scripts/fetch-mupdf-public.py
```

It verifies a pinned upstream hash and does not execute downloaded code. The fetch and a complete MuPDF-linked build are outside this publication's accepted tests.
