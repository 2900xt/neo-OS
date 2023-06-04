#include "kernel/mem/paging.h"
#include "limine/limine.h"
#include "stdlib/stdio.h"
#include "types.h"
#include <stdlib/stdlib.h>
#include <kernel/mem/mem.h>
#include <stdlib/lock.h>

static volatile limine::limine_hhdm_request hhdm_request = {LIMINE_HHDM_REQUEST, 0};
static const char * heap_tag = "Kernel Heap";

uint64_t getHHDM(void) { return hhdm_request.response->offset; }
int avalibleMemoryRegionsCount = 0;
uint64_t heapOffset, 
         heapBlkcount = 16 * 10,
         heapBlksize = 256; // Heap is 10 pages long initially
uint8_t *memoryBitmap;

enum MEMORY_TYPES { FREE = 0, USED = 1, BORDER = 2 };

//TODO : Move the heap into a dynamically allocated page
//TODO : Fix bug with bitmap overwriting the memory

spinlock_t heap_lock = SPIN_UNLOCKED;

void heapInit(void) 
{
  // Heap offset = Higher half address + offset + bitmap size(blkcount)

  heapOffset = getHHDM() + 0x10000;

  // Bitmap is to be located at the start of the heap, with the bitmap being
  // the size of the heap block count

  memoryBitmap = (uint8_t *)heapOffset;
  memoryBitmap[0] = BORDER;

  heapOffset += heapBlksize;
}

void *kmalloc(uint64_t size) 
{
  acquire_spinlock(&heap_lock);
  // Convert Bytes into heap blocks

  uint16_t remainder = size % heapBlksize;
  size -= remainder;
  if (remainder) {
    size += heapBlksize;
  }
  size /= heapBlksize;
  size++;

  // Scan through the memory blocks until we find a free one that is greater
  // than or equal to the size

  uint64_t currentIndex = 0;
  uint64_t freeBlocksFound = 0;
  uint8_t currentEntry = 0;
  uint64_t memoryAddr = 0;
  while (currentIndex < heapBlkcount) {

    currentEntry = memoryBitmap[currentIndex];

    switch (currentEntry) {
    case BORDER:
      freeBlocksFound = 1;
      if (freeBlocksFound == size)
        goto blockFound;
      break;
    case FREE:
      freeBlocksFound++;
      if (freeBlocksFound == size)
        goto blockFound;
      break;
    case USED:
      freeBlocksFound = 0;
    default:
      break;
    }

    currentIndex++;
  }

  // Not enough memory
  release_spinlock(&heap_lock);
  Log.e(heap_tag, "Requested size exceeds remaining heap size");
  return NULL;

blockFound:

  // Declare the memory blocks as used

  int borderIndex = currentIndex;

  currentIndex -= freeBlocksFound - 2;

  while (currentIndex < borderIndex) {
    memoryBitmap[currentIndex] = USED;
    currentIndex++;
  }

  memoryBitmap[currentIndex] = BORDER;

  currentIndex -= freeBlocksFound - 2;

  // Return the actual address on the heap

  memoryAddr = heapOffset;

  memoryAddr += currentIndex * heapBlksize;

  release_spinlock(&heap_lock);
  return (void *)memoryAddr;
}

void *kcalloc(uint64_t count, uint64_t size) {
  uint64_t totalSize = count * size;
  void *retPtr = kmalloc(totalSize);
  if (size % 64 == 0) {
    memset_64(retPtr, totalSize, 0);
  } else {
    memset_8(retPtr, totalSize, 0);
  }

  return retPtr;
}

void kfree(void *ptr) {

  acquire_spinlock(&heap_lock);
  // Get the index into the bitmap

  uint64_t bitmapIndex = (uint64_t)ptr - heapOffset;
  if ((bitmapIndex % heapBlksize != 0) ||
      (((uint64_t)ptr > heapOffset + heapBlkcount * heapBlksize) &&
       ((uint64_t)ptr < heapOffset))) {
    goto ERROR;
  }

  bitmapIndex /= heapBlksize;
  bitmapIndex--;

  if (memoryBitmap[bitmapIndex] != BORDER) {
    goto ERROR;
  }

  // Free the memory untill the next border

  bitmapIndex++;

  while (memoryBitmap[bitmapIndex] != BORDER &&
         memoryBitmap[bitmapIndex] == USED) {
    memoryBitmap[bitmapIndex] = FREE;
    bitmapIndex++;
  }

  // Free the border if there is no used memory on the other side

  if (memoryBitmap[bitmapIndex] == BORDER &&
      memoryBitmap[bitmapIndex + 1] == FREE) {
    memoryBitmap[bitmapIndex] = FREE;
  }

  release_spinlock(&heap_lock);
  return;

ERROR:

  Log.e(heap_tag, "Invalid Heap Pointer: %x", ptr);
  release_spinlock(&heap_lock);
}

void *krealloc(void *old_ptr, uint64_t size) {
  // round up size to heapblksize

  uint16_t remainder = size % heapBlksize;
  size -= remainder;
  if (remainder) {
    size += heapBlksize;
  }

  // return new pointer

  void *new_ptr = kcalloc(1, size);

  kfree(old_ptr);

  return new_ptr;
}

void* operator new(size_t size)
{
    return kcalloc(1, size);
}

void* operator new[](size_t size)
{
    return kcalloc(1, size);
}

void operator delete(void *addr, uint64_t)
{
    kfree(addr);
}

void operator delete[](void *addr)
{
  kfree(addr);
}