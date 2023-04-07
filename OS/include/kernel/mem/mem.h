#pragma once
#include <types.h>
#include <limine/limine.h>

uint64_t getHHDM(void);

extern limine::limine_memmap_entry* avalibleMemoryRegions[10];

void heapInit();
void* kmalloc(uint64_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, uint64_t size);
void memcpy(void* _destination, void* _src, uint64_t num);
void memset_64(void* _addr, uint64_t num, uint64_t value);
void memset_8(void* _addr, uint64_t num, uint8_t value);
void* kcalloc(uint64_t count, uint64_t size);


void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void *addr);
void operator delete[](void *addr);