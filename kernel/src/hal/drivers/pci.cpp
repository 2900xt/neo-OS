#include "stdout.h"
#include <drivers/pci.h>
#include <arch/amd64/io.h>
#include <types.h>
#include <acpi/tables.h>
#include <mem.h>
#include <stdout.h>
#include <stdlib/SafePointer>

constexpr uint16_t PCI_CONFIG_ADDR = 0xCF8;
constexpr uint16_t PCI_CONFIG_DATA = 0xCFC;

const char* const pci_classes[]
{
    "Unclassifled",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge",
    "Simple Communication Controller",
    "Base System Perhiperal",
    "Input Device Controller",
    "Docking Station",
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent Controller",
    "Satilite Communication Controller",
    "Encryption Controller",
    "Signal Processing Controller",
};

const char* const vendor_ids[]
{
    "Intel",
    "AMD",
    "NVIDIA",
    "QEMU",
};

pci_device_t** pci_devices;
uint64_t pci_devices_index;

const char* getDeviceName(uint16_t vendor_id, uint16_t device_id)
{
    switch (vendor_id) {
        case 0x8086:
        {
            switch (device_id) {
                case 0x29C0:
                    return "Express DRAM Controller";
                case 0x2918:
                    return "LPC Interface Controller";
                case 0x2922:
                    return "6 Port ACHI Controller";
                case 0x2930:
                    return "SMBus Controller";
            }
        }
    }

    return itoa(device_id, 16);
}

const char* getVendorString(uint16_t vendor_id)
{
    switch(vendor_id)
    {
        case 0x8086:
            return vendor_ids[0];
        case 0x1022:
            return vendor_ids[1];
        case 0x10DE:
            return vendor_ids[2];
        case 0x1234:
            return vendor_ids[3];
        default:
            return itoa(vendor_id, 16);
    }
}

pci_device_t* get_pci_dev(uint16_t vendor, uint16_t device)
{
    for(int i = 0; i < pci_devices_index; i++)
    {
        if(pci_devices[i]->hdr.vendor_id == vendor && pci_devices[i]->hdr.device_id == device) return pci_devices[i];
    }
    return nullptr;
}

void enumerate_function(uint64_t device_addr, uint64_t function)
{
    uint64_t offset = function << 12;
    uint64_t func_addr = device_addr + offset;
    map_page(func_addr, func_addr);     //Identity map the address

    pci_dev_common_hdr* pci_func = (pci_dev_common_hdr*)func_addr;
    if (pci_func->device_id == 0 || pci_func->device_id == 0xFFFF) return;    //Function doesn't exist

    pci_device_t* dev = (pci_device_t*)pci_func;

    klogf(LOG_DEBUG, "PCI device found: %s %s %s\nType: %d\n", getVendorString(pci_func->vendor_id), getDeviceName(pci_func->vendor_id, pci_func->device_id), pci_classes[pci_func->class_code], dev->hdr.header_type);
    
    if((dev->hdr.header_type & 0xF) == 0x0)
    {
        pci_devices[pci_devices_index++] = dev;
    }
}

void enumerate_device(uint64_t bus_address, uint64_t device)
{
    uint64_t offset = device << 15;
    uint64_t device_addr = bus_address + offset;
    map_page(device_addr, device_addr);     //Identity map the address

    pci_dev_common_hdr* pci_dev = (pci_dev_common_hdr*)device_addr;
    if (pci_dev->device_id == 0 || pci_dev->device_id == 0xFFFF) return;    //Device doesn't exist

    for(uint64_t function = 0; function < 8 ; function++)
    {
        enumerate_function(device_addr, function);
    }
}

void enumerate_bus(uint64_t base, uint64_t bus)
{
    uint64_t offset = bus << 20;
    uint64_t busAddr = base + offset;
    map_page(busAddr, busAddr);     //Identity map the address

    pci_dev_common_hdr* pci_dev = (pci_dev_common_hdr*)busAddr;
    if (pci_dev->device_id == 0 || pci_dev->device_id == 0xFFFF) return;      //Bus doesn't exist

    for(uint64_t device = 0; device < 32; device++)
    {
        enumerate_device(busAddr, device);
    }

}

void enumerate_pci(ACPI_MCFG_HDR* mcfg)
{
    if(pci_devices != NULL) return;
    pci_devices_index = 0;

    pci_devices = (pci_device_t**)kcalloc(1, sizeof(pci_device_t*) * 128);

    klogf(LOG_DEBUG, "Scanning PCI bus...\n");
    int entries = ((mcfg->hdr.length) - sizeof(ACPI_MCFG_HDR)) / sizeof(PCI_DEVICE_CONFIG);
    for(int i = 0; i < entries; i++)
    {
        PCI_DEVICE_CONFIG* dev_conf = (PCI_DEVICE_CONFIG*)((uint64_t)mcfg + sizeof(ACPI_MCFG_HDR) + (sizeof(PCI_DEVICE_CONFIG) * i));
        for(uint64_t bus = dev_conf->startBus; bus < dev_conf->endBus; bus++)
        {
            enumerate_bus(dev_conf->baseAddress, bus);
        }
    }
}