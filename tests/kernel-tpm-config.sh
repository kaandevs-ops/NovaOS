#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
for n in 'CONFIG_TCG_TPM=y' 'CONFIG_SYSFS=y' 'CONFIG_DEVTMPFS=y' 'CONFIG_DEVTMPFS_MOUNT=y'; do grep -Fqx "$n" "$ROOT/configs/kernel/common.config"; done
for n in 'CONFIG_ACPI=y' 'CONFIG_PNP=y' 'CONFIG_PNPACPI=y' 'CONFIG_TCG_TIS_CORE=y' 'CONFIG_TCG_TIS=y' 'CONFIG_TCG_CRB=y'; do grep -Fqx "$n" "$ROOT/configs/kernel/x86_64.config"; done
for n in CONFIG_TCG_TPM CONFIG_SYSFS CONFIG_DEVTMPFS CONFIG_DEVTMPFS_MOUNT; do grep -Fq "$n" "$ROOT/scripts/build-kernel.sh"; done
for n in CONFIG_ACPI CONFIG_PNP CONFIG_PNPACPI CONFIG_TCG_TIS_CORE CONFIG_TCG_TIS CONFIG_TCG_CRB; do grep -Fq "$n" "$ROOT/scripts/build-kernel.sh"; done
echo 'Stage-4C TPM kernel transport + sysfs/devtmpfs prerequisites passed.'
