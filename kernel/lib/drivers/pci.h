#pragma once
#include <types.h>


struct pci_config_addr_t
{
    uint32_t register_offset : 8;
    uint32_t function_number : 3;
    uint32_t device_number   : 5;
    uint32_t bus_number      : 8;
    uint32_t reserved        : 7;
    uint32_t enabled         : 1;
}__attribute__((packed));

struct pci_dev_common_hdr
{
    uint16_t vendor_id;        //Identifies the manufacturer of the device
    uint16_t device_id;        //identifies the particular device
    uint16_t status;           //A register used to record status information for PCI bus related events
    uint16_t command;          //Provides control over a device's ability to generate and respond to PCI cycles. Where the only functionality guaranteed to be supported by all devices is, when a 0 is written to this register, the device is disconnected from the PCI bus for all accesses except Configuration Space access. 
    uint8_t revision_id;       //Specifies a revision identifier for a particular device. Where valid IDs are allocated by the vendor. 
    uint8_t prog_ifb;          //A read-only register that specifies a register-level programming interface the device has, if it has any at all. 
    uint8_t subclass;          //A read-only register that specifies the specific function the device performs. 
    uint8_t class_code;        //A read-only register that specifies the type of function the device performs. 
    uint8_t cache_size;        //Specifies the system cache line size in 32-bit units. A device can limit the number of cacheline sizes it can support, if a unsupported value is written to this field, the device will behave as if a value of 0 was written.
    uint8_t latency_timer;     //Specifies the latency timer in units of PCI bus clocks. 
    uint8_t header_type;       //Identifies the layout of the rest of the header beginning at byte 0x10 of the header. If bit 7 of this register is set, the device has multiple functions; otherwise, it is a single function device. Types:  0x0: a general device - 0x1: a PCI-to-PCI bridge  - 0x2: a PCI-to-CardBus bridge. 
    uint8_t BIST;              //Represents that status and allows control of a devices BIST (built-in self test).
};


struct pci_base_mem_t           //Memory Space BAR Layout 
{
    uint32_t zero : 1;          //set to zero if memory-space BAR
    uint32_t type : 2;
    uint32_t prefetchable : 1; 
    uint32_t addr         : 28; //16-byte alligned base address
}__attribute__((packed));


struct pci_bar_io_t             //I/O Space BAR Layout 
{
    uint32_t one : 1;           //set to one if I/O-space BAR
    uint32_t reserved : 1;
    uint32_t address  : 30;     //4-byte alligned base address
}__attribute__((packed));

uint32_t pci_config_read(pci_config_addr_t* word);

enum PCI_COMMAND_REGISTER_FLAGS : uint16_t
{
    IO_SPACE                = (1 << 0),     //If set to 1 the device can respond to I/O Space accesses; otherwise, the device's response is disabled. 
    MEMORY_SPACE            = (1 << 1),     //If set to 1 the device can respond to Memory Space accesses; otherwise, the device's response is disabled.
    BUS_MASTER              = (1 << 2),     //If set to 1 the device can behave as a bus master; otherwise, the device can not generate PCI accesses. 
    SPECIAL_CYCLES          = (1 << 3),     //If set to 1 the device can monitor Special Cycle operations; otherwise, the device will ignore them. 
    MEM_WRITE_INV_ENABLE    = (1 << 4),     //If set to 1 the device can generate the Memory Write and Invalidate command; otherwise, the Memory Write command must be used. 
    VGA_PALETTE_SNOOP       = (1 << 5),     //If set to 1 the device does not respond to palette register writes and will snoop the data; otherwise, the device will trate palette write accesses like all other accesses. 
    PARITY_ERROR_RESPONSE   = (1 << 6),     //If set to 1 the device will take its normal action when a parity error is detected; otherwise, when an error is detected, the device will set bit 15 of the Status register (Detected Parity Error Status Bit), but will not assert the PERR# (Parity Error) pin and will continue operation as normal. 
    SERR_ENABLE             = (1 << 8),     //If set to 1 the SERR# driver is enabled; otherwise, the driver is disabled. 
    FAST_ENABLE             = (1 << 9),     //If set to 1 indicates a device is allowed to generate fast back-to-back transactions; otherwise, fast back-to-back transactions are only allowed to the same agent. 
    INTERRUPT_DISABLE       = (1 << 10)     //If set to 1 the assertion of the devices INTx# signal is disabled; otherwise, assertion of the signal is enabled. 
};

enum PCI_STATUS_REGISTER_FLAGS : uint16_t
{
    INTERRUPT_STATUS            = (1 << 3),     //Represents the state of the device's INTx# signal. If set to 1 and bit 10 of the Command register (Interrupt Disable bit) is set to 0 the signal will be asserted; otherwise, the signal will be ignored. 
    CAPABILITIES_LIST           = (1 << 4),     //If set to 1 the device implements the pointer for a New Capabilities Linked list at offset 0x34; otherwise, the linked list is not available. 
    TRANSFER_SPEED              = (1 << 5),     //If set to 1 the device is capable of running at 66 MHz; otherwise, the device runs at 33 MHz. 
    FAST_BACK_TO_BACK_CAPABLE   = (1 << 7),     //If set to 1 the device can accept fast back-to-back transactions that are not from the same agent; otherwise, transactions can only be accepted from the same agent. 
    MASTER_DATA_PARITY_ERROR    = (1 << 8),     //This bit is only set when the following conditions are met. The bus agent asserted PERR# on a read or observed an assertion of PERR# on a write, the agent setting the bit acted as the bus master for the operation in which the error occurred, and bit 6 of the Command register (Parity Error Response bit) is set to 1. 
    DEVSEL_TIMING_LOW           = (1 << 9),     //Read only bits that represent the slowest time that a device will assert DEVSEL# for any bus command except Configuration Space read and writes. Where a value of 0x0 represents fast timing, a value of 0x1 represents medium timing, and a value of 0x2 represents slow timing. 
    DEVSEL_TIMING_HIGH          = (1 << 10),    //Read only bits that represent the slowest time that a device will assert DEVSEL# for any bus command except Configuration Space read and writes. Where a value of 0x0 represents fast timing, a value of 0x1 represents medium timing, and a value of 0x2 represents slow timing.  
    SIGNALED_TARGET_ABORT       = (1 << 11),    //This bit will be set to 1 whenever a target device terminates a transaction with Target-Abort. 
    RECEIVED_TARGET_ABORT       = (1 << 12),    //This bit will be set to 1, by a master device, whenever its transaction is terminated with Target-Abort.  
    RECIEVED_MASTER_ABORT       = (1 << 13),    //This bit will be set to 1, by a master device, whenever its transaction (except for Special Cycle transactions) is terminated with Master-Abort.        
    SIGNALED_SYSTEM_ERROR       = (1 << 14),    //This bit will be set to 1 whenever the device asserts SERR#.        
    DETECTED_PARITY_ERROR       = (1 << 15),    //This bit will be set to 1 whenever the device detects a parity error, even if parity error handling is disabled.        
};

enum class PCI_CLASS_CODES : uint8_t
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


int checkDevice(uint16_t bus, uint8_t device, PCI_CLASS_CODES class_code, uint8_t subclass, uint8_t prog_if);         //Returns 0 if matching device found