#pragma once
#include <limine/limine.h>
#include <types.h>

namespace ACPI 
{

struct SDT_HEADER
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

struct XSDT
{
    SDT_HEADER     hdr;
    uint64_t       ptr[];
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
    XSDT*       XSDTAddress;
    uint8_t     extendedChecksum;
    uint8_t     reserved[3];
} __attribute__ ((packed));

RSDPDescriptor* getRSDP(void);
void* findACPITable(const char* signature);

}