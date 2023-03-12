# NEO-OS - an open source, fast, and secure amd64 operating system

Built on the LIMINE protocol, NEO-OS supports UEFI booting

## Current Version - 0.001A

## Features

* modular page frame allocator
* stdlib with basic string, math and i/o functions
* heap
* virtual file system
* timers
* simultaneous multiprocessing
* LAPIC, ACPI, AHCI, PCI, serial & VGA drivers

## Goals

* Finish AHCI Driver
* Processes
* Scheduler
* Files / Streams
* Standard Library
* USB
* Config

## How to use

Currently, to use the OS, you can either build it yourself or just use the prebuilt binary "neo-OS.hdd" in the repository

To build it yourself, clone this repo, then type "make all". To build and test in QEMU, type "make run". Make note of the prerequisites. If you don't have an c++ cross compiler at "/usr/local/x86_64elfgcc/bin/x86_64-elf-g++", you can build the toolchain with "./toolchain.sh"

### Prerequisites

libmpfr
libgmp
libmpc
texinfos