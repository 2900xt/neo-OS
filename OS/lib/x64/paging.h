#pragma once
#include <types.h>

namespace AMD64
{

void unmap_page(uint64_t virtualAddr);
void map_page(uint64_t virtualAddr, uint64_t physicalAddress);
uint64_t getPhysicalAddress(uint64_t virtualAddr);


void initialize_allocator(void *base, void *end);
void* next_page(void);
void free_page(void *_pg);

}