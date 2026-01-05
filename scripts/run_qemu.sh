qemu-system-x86_64 \
  -enable-kvm \
  -cpu Skylake-Client,+invtsc,+tsc,+vmware-cpuid-freq \
  -machine q35 \
  -smp 1 \
  -serial stdio \
  -cdrom cincos.iso \
  -m 1G \
  -no-reboot
