#include <types.h>

namespace kernel
{
    // Set num_bytes to value starting from addr
    void memset(void *_addr, uint64_t num, uint8_t value)
    {
        uint8_t *addr = (uint8_t *)_addr;

        for (int i = 0; i < num; i++)
        {
            *(addr + i) = value;
        }
    }

    void memcpy(void *_destination, const void *_src, uint64_t num)
    {
        uint8_t *destPtr = (uint8_t *)_destination;
        uint8_t *srcPtr = (uint8_t *)_src;

        for (int i = 0; i < num; i++)
        {
            destPtr[i] = srcPtr[i];
        }
    }
}