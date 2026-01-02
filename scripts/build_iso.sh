#!/bin/sh
set -e

mkdir -p iso/boot
cp build/kernel/cincos iso/boot/cincos

cp third_party/limine/bin/limine-bios.sys iso/boot/
cp third_party/limine/bin/limine-bios-cd.bin iso/boot/
cp limine.conf iso/boot/

xorriso -as mkisofs \
  -b boot/limine-bios-cd.bin \
  -no-emul-boot -boot-load-size 4 -boot-info-table \
  iso -o cincos.iso
