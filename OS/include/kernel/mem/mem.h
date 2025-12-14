#pragma once
#include <types.h>
#include <limine/limine.h>

namespace kernel
{
    uint64_t getHHDM(void);

    extern limine::limine_memmap_entry *avalibleMemoryRegions[10];

    void heapInit();
    void *kmalloc(uint64_t size);
    void kfree(void *ptr);
    void *krealloc(void *ptr, uint64_t size);
    void memcpy(void *_destination, const void *_src, uint64_t num);
    void memset(void *_addr, uint64_t num, uint8_t value);
    void *kcalloc(uint64_t count, uint64_t size);

}

void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void *addr, uint64_t);
void operator delete[](void *addr, uint64_t);