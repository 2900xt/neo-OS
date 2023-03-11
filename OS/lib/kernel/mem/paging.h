#pragma once
#include <types.h>

namespace kernel
{

void unmap_page(uint64_t virtualAddr);
void map_page(uint64_t virtualAddr, uint64_t physicalAddress);
uint64_t getPhysicalAddress(uint64_t virtualAddr);
void initialize_page_allocator();

void *allocate_pages(uint64_t requested_count);
void free_pages(void *page);

}