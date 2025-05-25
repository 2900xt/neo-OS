#include <stdlib/stdlib.h>
#include <kernel/kernel.h>
#include <stdlib/lock.h>

namespace kernel
{

    static volatile limine::limine_hhdm_request hhdm_request = {LIMINE_HHDM_REQUEST, 0};
    static const char *heap_tag = "Heap";

    enum class HEAP_MEMORY_TYPES : uint8_t
    {
        FREE = 0,
        USED = 1,
        BORDER = 2
    };

    uint64_t getHHDM(void) { return hhdm_request.response->offset; }
    uint64_t heapOffset;
    uint64_t heapBlkcount = 16 * 10;
    uint64_t heapBlksize = 256; // Heap is 10 pages long initially
    HEAP_MEMORY_TYPES *memoryBitmap;

    // TODO : Move the heap into a dynamically allocated page
    // TODO : Fix bug with bitmap overwriting the memory

    stdlib::spinlock_t heap_lock = stdlib::SPIN_UNLOCKED;

    void heapInit(void)
    {
        // Heap offset = Higher half address + offset + bitmap size(blkcount)

        heapOffset = getHHDM() + 0x10000;

        // Bitmap is to be located at the start of the heap, with the bitmap being
        // the size of the heap block count

        memoryBitmap = (HEAP_MEMORY_TYPES *)heapOffset;
        memoryBitmap[0] = HEAP_MEMORY_TYPES::BORDER;

        heapOffset += heapBlksize;
    }

    void *kmalloc(uint64_t size)
    {
        stdlib::acquire_spinlock(&heap_lock);
        // Convert Bytes into heap blocks

        uint16_t remainder = size % heapBlksize;
        size -= remainder;
        if (remainder)
        {
            size += heapBlksize;
        }
        size /= heapBlksize;
        size++;

        // Scan through the memory blocks until we find a free one that is greater
        // than or equal to the size

        uint64_t currentIndex = 0;
        uint64_t freeBlocksFound = 0;
        HEAP_MEMORY_TYPES currentEntry = HEAP_MEMORY_TYPES::FREE;
        uint64_t memoryAddr = 0;
        while (currentIndex < heapBlkcount)
        {

            currentEntry = memoryBitmap[currentIndex];

            switch ((HEAP_MEMORY_TYPES)currentEntry)
            {
            case HEAP_MEMORY_TYPES::BORDER:
                freeBlocksFound = 1;
                if (freeBlocksFound == size)
                    goto blockFound;
                break;
            case HEAP_MEMORY_TYPES::FREE:
                freeBlocksFound++;
                if (freeBlocksFound == size)
                    goto blockFound;
                break;
            case HEAP_MEMORY_TYPES::USED:
                freeBlocksFound = 0;
            default:
                break;
            }

            currentIndex++;
        }

        // Not enough memory
        stdlib::release_spinlock(&heap_lock);
        log::e(heap_tag, "Requested size exceeds remaining heap size");
        return NULL;

    blockFound:

        // Declare the memory blocks as used

        int borderIndex = currentIndex;

        currentIndex -= freeBlocksFound - 2;

        while (currentIndex < borderIndex)
        {
            memoryBitmap[currentIndex] = HEAP_MEMORY_TYPES::USED;
            currentIndex++;
        }

        memoryBitmap[currentIndex] = HEAP_MEMORY_TYPES::BORDER;

        currentIndex -= freeBlocksFound - 2;

        // Return the actual address on the heap

        memoryAddr = heapOffset;

        memoryAddr += currentIndex * heapBlksize;

        stdlib::release_spinlock(&heap_lock);
        return (void *)memoryAddr;
    }

    void *kcalloc(uint64_t count, uint64_t size)
    {
        uint64_t totalSize = count * size;
        void *retPtr = kmalloc(totalSize);
        if (size % 64 == 0)
        {
            memset_64(retPtr, totalSize, 0);
        }
        else
        {
            memset_8(retPtr, totalSize, 0);
        }

        return retPtr;
    }

    void kfree(void *ptr)
    {

        stdlib::acquire_spinlock(&heap_lock);
        // Get the index into the bitmap

        uint64_t bitmapIndex = (uint64_t)ptr - heapOffset;
        if ((bitmapIndex % heapBlksize != 0) ||
            (((uint64_t)ptr > heapOffset + heapBlkcount * heapBlksize) &&
             ((uint64_t)ptr < heapOffset)))
        {
            goto ERROR;
        }

        bitmapIndex /= heapBlksize;
        bitmapIndex--;

        if (memoryBitmap[bitmapIndex] != HEAP_MEMORY_TYPES::BORDER)
        {
            goto ERROR;
        }

        // Free the memory untill the next border

        bitmapIndex++;

        while (memoryBitmap[bitmapIndex] != HEAP_MEMORY_TYPES::BORDER &&
               memoryBitmap[bitmapIndex] == HEAP_MEMORY_TYPES::USED)
        {
            memoryBitmap[bitmapIndex] = HEAP_MEMORY_TYPES::FREE;
            bitmapIndex++;
        }

        // Free the border if there is no used memory on the other side

        if (memoryBitmap[bitmapIndex] == HEAP_MEMORY_TYPES::BORDER &&
            memoryBitmap[bitmapIndex + 1] == HEAP_MEMORY_TYPES::FREE)
        {
            memoryBitmap[bitmapIndex] = HEAP_MEMORY_TYPES::FREE;
        }

        stdlib::release_spinlock(&heap_lock);
        return;

    ERROR:

        log::e(heap_tag, "Invalid Heap Pointer: %x", ptr);
        stdlib::release_spinlock(&heap_lock);
    }

    void *krealloc(void *old_ptr, uint64_t size)
    {
        // round up size to heapblksize

        uint16_t remainder = size % heapBlksize;
        size -= remainder;
        if (remainder)
        {
            size += heapBlksize;
        }

        // return new pointer

        void *new_ptr = kcalloc(1, size);

        kfree(old_ptr);

        return new_ptr;
    }
}

void *operator new(size_t size)
{
    return kernel::kcalloc(1, size);
}

void *operator new[](size_t size)
{
    return kernel::kcalloc(1, size);
}

void operator delete(void *addr, uint64_t)
{
    kernel::kfree(addr);
}

void operator delete[](void *addr)
{
    kernel::kfree(addr);
}