.PHONY: all
all: os-img

.PHONY: run
run: all
	@echo Running Image...
	@qemu-system-x86_64 -M q35 -m 2G -bios ovmf-x64/OVMF.fd -drive format=raw,file=neo-OS.hdd -smp cpus=4 > /dev/null

.PHONY:
kernel:
	@clear
	@echo Building Kernel...
	@$(MAKE) -C OS

.PHONY: os-img
os-img:
	@echo Building Kernel Image...
	@$(MAKE) -C . neo-OS.hdd
	@echo Cleaning Up...
	@$(MAKE) -C OS clean 1> /dev/null
	@rm -rf iso_root

.PHONY: ovmf
ovmf:
	mkdir -p ovmf-x64
	cd ovmf-x64 && curl -o OVMF-X64.zip https://efi.akeo.ie/OVMF/OVMF-X64.zip && unzip OVMF-X64.zip

.PHONY: limine
limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v4.x-branch-binary --depth=1
	make -C limine

neo-OS.hdd: kernel
	@rm -f neo-OS.hdd
	@echo Generating Image
	@dd if=/dev/zero bs=1M count=0 seek=64 of=neo-OS.hdd >/dev/null 2>&1
	@sudo parted -s neo-OS.hdd mklabel gpt
	@sudo parted -s neo-OS.hdd mkpart ESP fat32 2048s 100%
	@sudo parted -s neo-OS.hdd set 1 esp on
	@limine/limine-deploy neo-OS.hdd >/dev/null 2>&1
	@sudo losetup -Pf --show neo-OS.hdd > loopback_dev
	@sudo mkfs.fat -F 32 `cat loopback_dev`p1 >/dev/null 2>&1
	@mkdir -p img_mount
	@sudo mount `cat loopback_dev`p1 img_mount
	@sudo mkdir -p img_mount/EFI/BOOT
	@sudo cp -v OS/bin/kernel.elf limine.cfg limine/limine.sys img_mount/
	@sudo cp -v limine/BOOTX64.EFI img_mount/EFI/BOOT/
	@sync
	@sudo umount img_mount
	@sudo losetup -d `cat loopback_dev` 
	@sudo rm -rf loopback_dev img_mount
