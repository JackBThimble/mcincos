#!/usr/bin/env bash
set -euo pipefail
ISO="$(cd "$(dirname "$0")/.." && pwd)/cincos.iso"

OVMF_CODE="$(cd "$(dirname "$0")/.." && pwd)/ovmf.fd"
OVMF_VARS="$(cd "$(dirname "$0")/.." && pwd)/ovmf_vars.fd"

qemu-system-x86_64 \
  -enable-kvm \
  -cpu host,+invtsc \
  -m 1G \
  -machine q35 \
  -serial stdio \
  -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
  -drive if=pflash,format=raw,file="$OVMF_VARS" \
  -cdrom "$ISO"
  # -smp $(nproc)\

