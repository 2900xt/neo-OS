#include <limine/limine.h>
#include <types.h>

#ifndef ACPI_TABLES_H
#define ACPI_TABLES_H

struct ACPI_SDT_HEADER
{
    char            signature[4];
    uint32_t        length;
    uint8_t         revision;
    uint8_t         checksum;
    char            OEMID[6];
    char            OEMTableID[8];
    uint32_t        OEMRevision;
    uint32_t        CreatorID;
    uint32_t        CreatorRevision;
}__attribute__((packed));

struct ACPI_XSDT
{
    ACPI_SDT_HEADER     hdr;
    uint64_t            ptr[];
}__attribute__((packed));

struct RSDPDescriptor
{

    //ACPI 1.0

    char        signature[8];
    uint8_t     checksum;
    char        OEMID[6];
    uint8_t     revision;
    uint32_t    RSDTAddress;

    //ACPI 2.0+

    uint32_t    length;
    ACPI_XSDT*  XSDTAddress;
    uint8_t     extendedChecksum;
    uint8_t     reserved[3];
} __attribute__ ((packed));

struct ACPI_MADT_ENTRY_HDR
{
    uint8_t entryType;
    uint8_t entryLength;
}__attribute__ ((packed));

struct ACPI_MADT
{
    ACPI_SDT_HEADER         hdr;
    uint32_t                LAPIC_addr;
    uint32_t                LAPIC_flags;
    ACPI_MADT_ENTRY_HDR     ptr;
}__attribute__ ((packed));


struct LAPIC_ENTRY
{
    ACPI_MADT_ENTRY_HDR hdr;
    uint8_t             processor_id;
    uint8_t             apic_id;
    uint32_t            flags;
}__attribute__ ((packed));

struct IOAPIC_ENTRY
{
    ACPI_MADT_ENTRY_HDR hdr;
    uint8_t             ioapic_id;
    uint8_t             reserved;
    uint32_t            ioapic_base;
    uint32_t            system_interrupt_base;
}__attribute__ ((packed));

struct IOAPIC_INT_SOURCE
{
    ACPI_MADT_ENTRY_HDR hdr;
    uint8_t             bus_source;
    uint8_t             irq_source;
    uint32_t            global_interrupt;
    uint16_t            flags;
}__attribute__ ((packed));

struct IOAPIC_NMI_SOURCE
{
    ACPI_MADT_ENTRY_HDR hdr;
    uint8_t             source;
    uint16_t            flags;
    uint32_t            global_interrupt;
}__attribute__ ((packed));

struct LAPIC_NMI
{
    ACPI_MADT_ENTRY_HDR hdr;
    uint8_t             processor_id;
    uint16_t            flags;
    uint8_t             local_int;
}__attribute__ ((packed));

RSDPDescriptor* getRSDP(void);
void* findACPITable(char* signature);


void initAPIC(uint8_t APICid);

#endif