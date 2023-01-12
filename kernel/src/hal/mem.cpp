#include <mem.h>
#include <stdout.h>

static volatile limine::limine_hhdm_request hhdm_request = {LIMINE_HHDM_REQUEST, 0};

uint64_t getHHDM(void){
    return hhdm_request.response->offset;
}

static volatile limine::limine_memmap_request memmap_request = {LIMINE_MEMMAP_REQUEST, 0};

int avalibleMemoryRegionsCount = 0;
limine::limine_memmap_entry* avalibleMemoryRegions[10];

uint64_t heapOffset, heapBlkcount, heapBlksize;
uint8_t* memoryBitmap;

enum MEMORY_TYPES
{
    FREE        = 0,
    USED        = 1,
    BORDER      = 2
};


//Misc memory functionss

namespace neoSTL{

void memset_64(void* _addr, uint64_t num, uint64_t value){

    //Round up!

    if(num % 8 != 0){
        uint8_t remainder = num % 8;
        num -= remainder;
        num += 8;
    }

    num /= 8;

    uint64_t* addr = (uint64_t*)_addr;

    for(int i = 0; i < num; i++){
        addr[i] = value;
    }
}

void memset_8(void* _addr, uint64_t num, uint8_t value){
    uint8_t* addr = (uint8_t*)_addr;

    for(int i = 0; i < num; i++){
        addr[i] = value;
    }
}

void memcpy(void* _destination, void* _src, uint64_t num){

    if(num % 8 != 0){
        uint8_t remainder = num % 8;
        num -= remainder;
        num += 8;
    }

    uint8_t* destPtr = (uint8_t*)_destination;
    uint8_t* srcPtr = (uint8_t*)_src;

    for(int i = 0; i < num; i++){
        destPtr[i] = srcPtr[i];
    }

}

void heapInit(uint64_t offset, uint64_t size, uint64_t blksize){
    uint64_t HHDM = getHHDM();
    limine::limine_memmap_entry* currentEntry;
    limine::limine_memmap_entry* largestFreeRegion = nullptr;

    for(int i = 0; i < memmap_request.response->entry_count; i++){
        currentEntry = memmap_request.response->entries[i];
        if(currentEntry->type == 0){
            neoSTL::printf("Free Memory Entry %d: \t Offset = 0x%x \t Size = 0x%x\n", avalibleMemoryRegionsCount, currentEntry->base, currentEntry->length);
            avalibleMemoryRegions[avalibleMemoryRegionsCount++] = currentEntry;
            if(largestFreeRegion == nullptr || currentEntry->length > largestFreeRegion->length){
                largestFreeRegion = currentEntry;
            }
        }
    }
    heapBlkcount = size / blksize - 1;
    heapBlksize = blksize;

//Heap offset = Higher half address + offset + bitmap size(blkcount)

    heapOffset = heapBlkcount + offset + HHDM;

//Bitmap is gonna be located at the start of the heap, with the bitmap being the size of the heap block count

    memoryBitmap = (uint8_t*)heapOffset;
    memoryBitmap[0] = BORDER;
    
    neoSTL::printf("\nStarting kernel heap at: 0x%x\tWith length: 0x%x\n", heapOffset, heapBlksize * heapBlkcount);
}


void* kmalloc(uint64_t size)
{

//Convert Bytes into heap blocks

    uint16_t remainder = size % heapBlksize;
    size -= remainder;
    if(remainder){
        size += heapBlksize;
    }
    size /= heapBlksize;


//Scan through the memory blocks until we find a free one that is greater than or equal to the size

    uint64_t currentIndex = 0;
    uint64_t freeBlocksFound = 0;
    uint8_t  currentEntry = 0;
    uint64_t memoryAddr = 0;
    while(currentIndex < heapBlkcount){

        currentEntry = memoryBitmap[currentIndex];

        switch (currentEntry)
        {
        case BORDER:
            freeBlocksFound = 1;
            if(freeBlocksFound == size) goto blockFound;
            break;
        case FREE:
            freeBlocksFound++;
            if(freeBlocksFound == size) goto blockFound;
            break;
        case USED:
            freeBlocksFound = 0;
        default:
            break;
        }

        currentIndex++;
    }

//Not enough memory

    return nullptr;

blockFound:


//Declare the memory blocks as used

    int borderIndex = currentIndex;

    currentIndex -= freeBlocksFound - 2;

    while(currentIndex < borderIndex)
    {
        memoryBitmap[currentIndex] = USED;
        currentIndex++;
    }
    
    memoryBitmap[currentIndex] = BORDER;
    
    currentIndex -= freeBlocksFound - 2;

//Return the actual address on the heap

    memoryAddr = heapOffset;

    memoryAddr += currentIndex * heapBlksize;

    return (void*)memoryAddr;

}

void* kcalloc(uint64_t count, uint64_t size){
    uint64_t totalSize = count * size;
    void* retPtr = kmalloc(totalSize);
    if(size % 64 == 0){
        memset_64(retPtr, totalSize, 0);
    } else {
        memset_8(retPtr, totalSize, 0);
    }

    return retPtr;
}

void kfree(void* ptr){

//Get the index into the bitmap

    uint64_t bitmapIndex = (uint64_t)ptr - heapOffset;
    if((bitmapIndex % heapBlksize != 0 ) || (((uint64_t)ptr > heapOffset + heapBlkcount * heapBlksize) && ((uint64_t)ptr < heapOffset))){
        neoSTL::printf("Invalid Pointer: %x\n", ptr);
        return;
    }

    bitmapIndex /= heapBlksize;
    bitmapIndex--;
    uint8_t currentBlock;

    currentBlock = memoryBitmap[bitmapIndex];

    if(currentBlock != BORDER){
        neoSTL::printf("Expected a border block! %x: %d\n", ptr, currentBlock);
        return;
    }

//Free the memory untill the next border

    currentBlock = memoryBitmap[++bitmapIndex];

    memoryBitmap[bitmapIndex] = FREE;

}

};