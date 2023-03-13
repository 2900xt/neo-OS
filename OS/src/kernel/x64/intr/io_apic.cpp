#include "types.h"
#include <kernel/x64/intr/apic.h>
#include <kernel/mem/paging.h>
#include <stdlib/stdio.h>

extern kernel::IOAPIC_ENTRY*    ioapic;

namespace kernel
{

static volatile uint32_t *ioapic_addr;

enum IOAPIC_REGISTER_OFFSETS : uint32_t
{

    /**
     * This register has index 0 (you write 0 to IOREGSEL and then read from IOREGWIN).  
     * It's a Read-Only register with almost all bits reserved.
     * The only interesting field is in bits 24 - 27: the APIC ID for this device.
     * You shall find this ID in ACPI/MP Tables as well.
     */

    IOAPIC_ID               = 0x0,

    /**
     * This register (index 1) contains the I/O APIC Version in bits 0 - 8.
     * It also contains the Max Redirection Entry which is "how many IRQs can this I/O APIC handle - 1". It is encoded in bits 16 - 23. 
     */

    IOAPIC_VERSION          = 0x1,

    /**
     * Undefined / Unreiable on most systems.
     * This register (index 2) contains in bits 24 - 27 the APIC Arbitration ID
     */

    IOAPIC_ARB              = 0x2,

    #define IOAPIC_RED_TBL_LOW(x)   (0x10 + 2 * (x))
    #define IOAPIC_RED_TBL_HI(x)   (IOAPIC_RED_TBL_LOW((x)) + 1)

};

struct ioapic_redir_table_entry
{
    uint8_t vector;
    uint8_t delivery_mode : 3;
    uint8_t destination_mode : 1;
    uint8_t delivery_status : 1;
    uint8_t pin_polatiry : 1;
    uint8_t remote_irr : 1;
    uint8_t trigger_mode : 1;
    uint8_t mask : 1;
    uint64_t reserved : 39;
    uint8_t destination;
};

static void write_ioapic_register(uint32_t offset, uint32_t val)
{
    *ioapic_addr = offset;
    *(ioapic_addr + 0x10) = val;
}

static uint32_t read_ioapic_register(uint32_t offset)
{
    *ioapic_addr = offset;
    return *(ioapic_addr + 0x10);
}

void add_io_red_table_entry(uint8_t entry_number, ioapic_redir_table_entry _entry)
{
    uint64_t entry = *(uint64_t*)(&entry);
    write_ioapic_register(IOAPIC_RED_TBL_HI(entry_number), entry >> 32);
    write_ioapic_register(IOAPIC_RED_TBL_LOW(entry_number), entry);

}

void ioapic_init()
{
    ioapic_addr = (volatile uint32_t*)kernel::allocate_pages(2);  //Allocating extra pages, in case IOAPIC registers cross a page boundary
    kernel::map_pages((uint64_t)ioapic_addr , (uint64_t)ioapic_addr + 0x1000 * 2, ioapic->ioapic_base, ioapic->ioapic_base + 0x1000 * 2);
    for(int i = 0; i < 256; i++)
    {
        write_ioapic_register(IOAPIC_RED_TBL_LOW(i), 0xFFFF);
        write_ioapic_register(IOAPIC_RED_TBL_HI(i), 0xFFFF);
    }
    std::klogf("IO: 0x%x\n", (uint64_t)read_ioapic_register(IOAPIC_RED_TBL_HI(0)));
}

};