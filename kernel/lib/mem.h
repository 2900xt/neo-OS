#include <types.h>
#ifndef MEM_H
#define MEM_H

uint64_t getHDDM(void);
void heapInit(uint64_t offset, uint64_t size, uint64_t blksize);
void* kmalloc(uint64_t size);

#endif // !MEM_H