#include <types.h>
#ifndef MEM_H
#define MEM_H

uint64_t getHHDM(void);

namespace neoSTL{

    void heapInit(uint64_t offset, uint64_t size, uint64_t blksize);
    void* kmalloc(uint64_t size);
    void kfree(void* ptr);
    void memcpy(void* _destination, void* _src, uint64_t num);
    void memset_64(void* _addr, uint64_t num, uint64_t value);
    void memset_8(void* _addr, uint64_t num, uint8_t value);
    void* kcalloc(uint64_t count, uint64_t size);

};

#endif // !MEM_H