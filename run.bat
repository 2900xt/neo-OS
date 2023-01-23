wsl make
"C:\Program Files\qemu\qemu-system-x86_64" -M q35 -m 2G -bios ovmf-x64/OVMF.fd -hda neo-OS.hdd -smp cpus=4 -cpu Skylake-Client