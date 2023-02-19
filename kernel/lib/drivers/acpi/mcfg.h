#pragma once
#include <types.h>
#include <drivers/acpi/sdt.h>
struct ACPI_MCFG_HDR
{
    ACPI_SDT_HEADER hdr;
    uint64_t reserved;
}__attribute__((packed));


struct PCI_DEVICE_CONFIG
{
    uint64_t baseAddress;
    uint16_t PCISegGroup;
    uint8_t startBus;
    uint8_t endBus;
    uint32_t reserved;
}__attribute__((packed));


