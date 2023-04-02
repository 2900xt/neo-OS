#pragma once
#include <types.h>

namespace kernel
{

#define mmap(virt, phys) kernel::map_page((uint64_t)virt, (uint64_t)phys)

void unmap_page(uint64_t virtualAddr);
void map_page(uint64_t virtualAddr, uint64_t physicalAddress);
uint64_t getPhysicalAddress(uint64_t virtualAddr);

void map_pages(uint64_t startVirtualAddr, uint64_t startPhysicalAddr, uint64_t count);
void map_pages(uint64_t start_virtual_addr, uint64_t end_virtual_addr, uint64_t start_physical_addr, uint64_t end_physical_addr);  //Maps pages up to, but not including the end_addr
void unmap_pages(uint64_t start_virtual_addr, uint64_t end_virtual_addr);

void initialize_page_allocator();

enum MEMORY_TYPES : uint8_t
{
    FREE                    = 0x0,
    RESERVED                = 0x1,
    ACPI_RECLAIMABLE        = 0x2,
    ACPI_NVS                = 0x3,
    BAD_MEMORY              = 0x4,
    BOOTLOADER_RECLAIMABLE  = 0x5,
    KERNEL_MAPPED           = 0x6,
    FRAMBUFFER              = 0x7,
    DMA                     = 0x8,
    SWAPPED                 = 0x9,
    USED                    = 0xA,
};


struct memory_segment_t
{
    uint64_t        start;
    uint64_t        size;
    MEMORY_TYPES    type;
};

struct page_list_entry_t
{
    memory_segment_t    memory;
    page_list_entry_t*  next;
    page_list_entry_t*  prev;
};

void* allocate_pages(uint64_t requested_count);
void free_pages(void *page);

}