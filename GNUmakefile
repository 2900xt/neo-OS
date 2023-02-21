.PHONY: run
run: neo-OS.hdd clean
	qemu-system-x86_64 -M q35 -m 2G -bios ovmf-x64/OVMF.fd -hda neo-OS.hdd -smp cpus=4

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

.PHONY: ovmf
ovmf:
	mkdir -p ovmf-x64
	cd ovmf-x64 && curl -o OVMF-X64.zip https://efi.akeo.ie/OVMF/OVMF-X64.zip && unzip OVMF-X64.zip

.PHONY: limine
limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v4.x-branch-binary --depth=1
	make -C limine

neo-OS.hdd: kernel
	rm -f neo-OS.hdd
	dd if=/dev/zero bs=1M count=0 seek=64 of=neo-OS.hdd
	sudo parted -s neo-OS.hdd mklabel gpt
	sudo parted -s neo-OS.hdd mkpart ESP fat32 2048s 100%
	sudo parted -s neo-OS.hdd set 1 esp on
	limine/limine-deploy neo-OS.hdd
	sudo losetup -Pf --show neo-OS.hdd > loopback_dev
	sudo mkfs.fat -F 32 `cat loopback_dev`p1
	mkdir -p img_mount
	sudo mount `cat loopback_dev`p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v kernel/bin/kernel.elf limine.cfg limine/limine.sys wallpaper.bmp img_mount/
	sudo cp -v limine/BOOTX64.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d `cat loopback_dev`
	sudo rm -rf loopback_dev img_mount

.PHONY: clean
clean:
	rm -rf iso_root
	$(MAKE) -C kernel clean
