#include "types.h"
#include <kernel/x64/intr/apic.h>
#include <kernel/mem/paging.h>
#include <stdlib/stdio.h>

extern ACPI::IOAPIC_ENTRY*    ioapic;

namespace kernel
{

static uint32_t *ioapic_addr;

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



static void write_ioapic_register(uint32_t offset, uint32_t val)
{
    *ioapic_addr = offset;
    *(ioapic_addr + 0x4) = val;
}

static uint32_t read_ioapic_register(uint32_t offset)
{
    *ioapic_addr = offset;
    return *(ioapic_addr + 0x4);
}

void add_io_red_table_entry(uint8_t entry_number, ioapic_redir_table_entry* _entry)
{
    uint64_t entry = *((uint64_t*)_entry);
    write_ioapic_register(IOAPIC_RED_TBL_HI(entry_number), entry >> 32);
    write_ioapic_register(IOAPIC_RED_TBL_LOW(entry_number), entry);

}

uint8_t get_ioapic_max_entries()
{
    return (read_ioapic_register(IOAPIC_VERSION) >> 16) + 1;
}

void ioapic_init()
{
    ioapic_addr = (uint32_t*)((uint64_t)ioapic->ioapic_base);
    kernel::map_page(ioapic->ioapic_base, ioapic->ioapic_base);
}

};