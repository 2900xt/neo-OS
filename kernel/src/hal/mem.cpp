#include <mem.h>
#include <stdout.h>

static volatile limine::limine_hhdm_request hhdm_request = {LIMINE_HHDM_REQUEST, 0};

uint64_t getHDDM(void){
    return hhdm_request.response->offset;
}

static volatile limine::limine_memmap_request memmap_request = {LIMINE_MEMMAP_REQUEST, 0};

int avalibleMemoryRegionsCount = 0;
limine::limine_memmap_entry* avalibleMemoryRegions[10];

void readMemoryMap(void){
    limine::limine_memmap_entry* currentEntry;
    for(int i = 0; i < memmap_request.response->entry_count; i++){
        currentEntry = memmap_request.response->entries[i];
        neoOS_STD::printf("Entry: %d\tBase: 0x%x\tLength: 0x%x\tType: %d\n", i, currentEntry->base, currentEntry->length, currentEntry->type);
        if(currentEntry->type == 0){
            avalibleMemoryRegions[avalibleMemoryRegionsCount++] = currentEntry;
        }
    }
}
