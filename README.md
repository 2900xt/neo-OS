# NEO-OS - an open source, fast, and secure amd64 operating system

Built on the LIMINE protocol, NEO-OS supports UEFI booting

## Current Version - 0.001A

## Features

* modular page frame allocator
* stdlib with basic string, math and i/o functions
* heap
* virtual file system
* custom icon file format
* timers
* simultaneous multiprocessing
* APIC, ACPI, AHCI, PCI, serial & VGA drivers

## Goals

* Processes
* Scheduler
* Shell
* Standard Library
* USB
* Port BusyBox

## How to use

- build it yourself 
- or use the prebuilt binary "neo-OS.hdd" in the repository

### To build it yourself

#### Prerequisites

- libmpfr
- libgmp
- libmpc
- texinfo
- gcc
- nasm

### To build and test in QEMU

1. clone this repo
2. build the toolchain with "./toolchain.sh"
2. execute "make run"

