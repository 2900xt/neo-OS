#pragma once
#include <types.h>

void unmap_page(uint64_t virtualAddr);
void map_page(uint64_t virtualAddr, uint64_t physicalAddress);
uint64_t getPhysicalAddress(uint64_t virtualAddr);