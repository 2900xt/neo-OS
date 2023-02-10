#include "stdout.h"
#include <drivers/pci.h>
#include <arch/amd64/io.h>
#include <types.h>
#include <acpi/tables.h>

constexpr uint16_t PCI_CONFIG_ADDR = 0xCF8;
constexpr uint16_t PCI_CONFIG_DATA = 0xCFC;

struct pci_device_t
{
    pci_dev_common_hdr hdr;
    uint32_t BARS[6];
    uint32_t cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_addr;
    uint8_t  capabillities_ptr;
    uint8_t  max_latency;
    uint8_t  interrupt_pin;
    uint8_t  min_grant;
    uint8_t  interrupt_line;
};

uint32_t pci_config_read(pci_config_addr_t* word)
{
    outl(PCI_CONFIG_ADDR, *((uint32_t*)word));
    return inl(PCI_CONFIG_DATA);
}

uint32_t pci_read_register(uint8_t reg, uint8_t bus, uint8_t slot, uint8_t function)
{
    pci_config_addr_t data 
    {
    reg, 
    function, 
    slot, 
    bus, 
    0, 
    true
    };
    
    return pci_config_read(&data);

}

void enumerate_bus(uint64_t base, uint64_t bus)
{
    uint64_t offset = 
}

void enumerate_pci(ACPI_MCFG_HDR* mcfg)
{
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