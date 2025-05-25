#pragma once
#include <types.h>
#include <drivers/acpi/mcfg.h>

namespace pci 
{

struct DEVICE_CONFIG
{
    uint64_t baseAddress;
    uint16_t PCISegGroup;
    uint8_t startBus;
    uint8_t endBus;
    uint32_t reserved;
}__attribute__((packed));

struct pci_command_t
{
    uint16_t io_space_enable : 1;
    uint16_t mem_space_enable : 1;
    uint16_t bus_master_enable : 1;
    uint16_t special_cycle_enable : 1;
    uint16_t mvi_enable : 1;
    uint16_t VGA_pallete_snoop_enable : 1;
    uint16_t parity_err_response_enable : 1;
    uint16_t wait_cycle_enable : 1;
    uint16_t SERR_enable : 1;
    uint16_t fast_back_to_back_enable : 1;
    uint16_t intr_disable : 1;
    uint16_t rsv0 : 5;
}__attribute__((packed));

struct dev_common_hdr
{
    uint16_t vendor_id;        //Identifies the manufacturer of the device
    uint16_t device_id;        //identifies the particular device
    pci_command_t command;     //Provides control over a device's ability to generate and respond to pci cycles. Where the only functionality guaranteed to be supported by all devices is, when a 0 is written to this register, the device is disconnected from the pci bus for all accesses except Configuration Space access. 
    uint16_t status;           //A register used to record status information for pci bus related events
    uint8_t revision_id;       //Specifies a revision identifier for a particular device. Where valid IDs are allocated by the vendor. 
    uint8_t prog_ifb;          //A read-only register that specifies a register-level programming interface the device has, if it has any at all. 
    uint8_t subclass;          //A read-only register that specifies the specific function the device performs. 
    uint8_t class_code;        //A read-only register that specifies the type of function the device performs. 
    uint8_t cache_size;        //Specifies the system cache line size in 32-bit units. A device can limit the number of cacheline sizes it can support, if a unsupported value is written to this field, the device will behave as if a value of 0 was written.
    uint8_t latency_timer;     //Specifies the latency timer in units of pci bus clocks. 
    uint8_t header_type;       //Identifies the layout of the rest of the header beginning at byte 0x10 of the header. If bit 7 of this register is set, the device has multiple functions; otherwise, it is a single function device. Types:  0x0: a general device - 0x1: a pci-to-pci bridge  - 0x2: a pci-to-CardBus bridge. 
    uint8_t BIST;              //Represents that status and allows control of a devices BIST (built-in self test).
}__attribute__((packed));

struct device_t
{
    dev_common_hdr hdr;
    uint32_t BARS[6];
    uint32_t cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_addr;
    uint8_t  capabillities_ptr;
    uint8_t rsv0[7];
    uint8_t intr_line;
    uint8_t intr_pin;
    uint8_t min_grant;
    uint8_t max_latency;
}__attribute__((packed));

struct base_mem_t           //Memory Space BAR Layout 
{
    uint32_t zero : 1;          //set to zero if memory-space BAR
    uint32_t type : 2;
    uint32_t prefetchable : 1; 
    uint32_t addr         : 28; //16-byte alligned base address
}__attribute__((packed));


struct bar_io_t             //I/O Space BAR Layout 
{
    uint32_t one : 1;           //set to one if I/O-space BAR
    uint32_t reserved : 1;
    uint32_t address  : 30;     //4-byte alligned base address
}__attribute__((packed));


enum class CLASS_CODES : uint8_t
{
    UNCLASSIFIED                    = 0x0,
    MASS_STORAGE_CONTROLLER         = 0x1,
    NETWORK_CONTROLLER              = 0x2,
    DISPLAY_CONTROLLER              = 0x3,
    MULTIMEDIA_CONTROLLER           = 0x4,
    MEMORY_CONTROLLER               = 0x5,
    PCI_BRIDGE                      = 0x6,
    SIMPLE_COM_CONTROLLER           = 0x7,
    BASE_SYSTEM_PERIPHERAL          = 0x8,
    INPUT_DEVICE_CONTROILLER        = 0x9,
    DOCKING_STATION                 = 0xA,
    PROCESSOR                       = 0xB,
    SERIAL_BUS_CONTROLLER           = 0xC,
    WIRELESS_CONTROLLER             = 0xD,
    INTELLIGENT_CONTROLLER          = 0xE,
    SATALITE_COM_CONTROLLER         = 0xF,
    ENCRYPTION_CONTROLLER           = 0x10,
    SIGNAL_PROCESSING_CONTROLLER    = 0x11,
    PROCESSING_ACCELERATOR          = 0x12,
    NON_ESESENTIAL_INSTRUMENTATION  = 0x13,
    CO_PROCESSOR                    = 0x40,
    UNASSIGNED_CLASS                = 0xFF
};


void enumerate_pci(void);
device_t* get_pci_dev(uint16_t vendor, uint16_t device);
device_t* get_pci_dev(uint8_t class_code, uint8_t subclass, uint8_t prog_if);

}