#pragma once

#include <types.h>


namespace FS 
{

typedef uint16_t guid_t[8];


struct gpt_part_table_hdr
{
    char            signature[8];
    uint32_t        gpt_revision;
    uint32_t        hdr_size;
    uint32_t        checksum;
    uint32_t        rsv0;
    uint64_t        hdr_lba;
    uint64_t        alt_hdr_lba;
    uint64_t        first_usable_block;
    uint64_t        last_usable_block;
    guid_t          disk_guid;
    uint64_t        lba_part_entry_arr;
    uint32_t        part_entry_count;
    uint32_t        part_entry_size;
    uint32_t        checksum_part_entry;
    uint8_t         pad[512 - 0x5C];
}__attribute__((packed));

struct gpt_part_table_entry
{
    guid_t          partition_guid;
    guid_t          unique_partition_guid;
    uint64_t        starting_lba;
    uint64_t        ending_lba;
    uint64_t        attrib;
    char            parition_name[72];
}__attribute__((packed));


struct gpt_part_data
{
    gpt_part_table_hdr      hdr;
    gpt_part_table_entry    entries[1];
}__attribute__((packed));

}