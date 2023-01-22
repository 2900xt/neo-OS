#include <stdout.h>
#include <mem.h>

enum PAGE_PROPERTIES
{
    PRESENT         = (1 << 0),
    WRITABLE        = (1 << 1),
    USER_ACCESSIBLE = (1 << 2),
    PWT             = (1 << 3),
    PCD             = (1 << 4),
    ACCESSED        = (1 << 5),
    DIRTY           = (1 << 6),
    PAT             = (1 << 7),
    GLOBAL          = (1 << 8)
};

enum PAGE_TABLE_OFFSETS
{
    PT_BITS_OFFSET      = 12,
    PD_BITS_OFFSET      = 21,
    PDPT_BITS_OFFSET    = 30,
    PM4LT_BITS_OFFSET   = 39
};

typedef uint64_t page_t;

uint64_t readCR3(void)
{
    uint64_t cr3;
    asm("mov %%cr3, %0" : "=a"(cr3));
    return cr3;
}

bool getPageEntry(uint64_t virtualAddr, page_t** pageEntry)
{

    uint64_t PDOffset        = (virtualAddr >> PD_BITS_OFFSET)      & 0b111111111;                  //Bits 21 - 29
    uint64_t PDPTOffset      = (virtualAddr >> PDPT_BITS_OFFSET)    & 0b111111111;                  //Bits 30 - 38
    uint64_t PM4LTOffset     = (virtualAddr >> PM4LT_BITS_OFFSET)   & 0b111111111;                  //Bits 39 - 47
    

    page_t* PM4LT   = (page_t*)((readCR3()) & ~0xFFF);
    page_t  PM4LE   = PM4LT[PM4LTOffset];
    if(~PM4LE & PRESENT) return false;

    page_t* PDPT    = (page_t*)((PM4LE) & ~0xFFF);
    page_t  PDPTE   = PDPT[PDPTOffset];

    
    if(~PDPTE & PRESENT) return false;


    page_t* PD      = (page_t*)((PDPTE) & ~0xFFF);
    page_t  PDE     = PD[PDOffset];

    
    if(~PDE & PRESENT) return false;

    page_t* PT      = (page_t*)((PDE)& ~0xFFF);

    *pageEntry = PT;

    return true;
}

uint64_t getPhysicalAddress(uint64_t virtualAddr)
{
    page_t* page;
    uint64_t pageOffset = (virtualAddr >> PT_BITS_OFFSET) & 0b111111111, physOffset = virtualAddr & 0xFFF;

    if(!getPageEntry(virtualAddr, &page)){
        klogf(LOG_WARNING, "Page table(s) is not present in memory!\n");
        return NULL;
    }

    if(~page[pageOffset] & PRESENT){
        klogf(LOG_WARNING, "Page is not present in memory!\n");
        return NULL;
    }

    return (uint64_t)((page[pageOffset] & ~0xFFF) | physOffset);
}




void map_page(uint64_t virtualAddr, uint64_t physicalAddress)
{

    page_t*         PM4LT;
    page_t*         PDPT;
    page_t*         PD;
    page_t*         PT;

    PM4LT = (page_t*)(readCR3() & ~0xFFF);

    uint64_t PTOffset        = (virtualAddr >> PT_BITS_OFFSET)      & 0b111111111;                  //Bits 12 - 20
    uint64_t PDOffset        = (virtualAddr >> PD_BITS_OFFSET)      & 0b111111111;                  //Bits 21 - 29
    uint64_t PDPTOffset      = (virtualAddr >> PDPT_BITS_OFFSET)    & 0b111111111;                  //Bits 30 - 38
    uint64_t PM4LTOffset     = (virtualAddr >> PM4LT_BITS_OFFSET)   & 0b111111111;                  //Bits 39 - 47

    //Make the PDPT if not present
    if(PM4LT[PM4LTOffset] == 0)
    {
        PDPT = (page_t*)kmalloc(32768);
        PM4LT[PM4LTOffset] = (page_t)PDPT | PRESENT | WRITABLE;
    } else {        //If we find already existing PDPT
        PDPT = (page_t*)(PM4LT[PM4LTOffset] & ~0xFFF);
    }

    if(PDPT[PDPTOffset] == 0)
    {
        PD = (page_t*)kmalloc(32768);
        PDPT[PDPTOffset] = (page_t)PD | PRESENT | WRITABLE;
    } else {
        PD = (page_t*)(PDPT[PDPTOffset] & ~0xFFF);
    }

    if(PD[PDOffset] == 0)
    {
        PT = (page_t*)kmalloc(32768);
        PD[PDOffset] = (page_t)PT | PRESENT | WRITABLE;
    } else {
        PT = (page_t*)(PD[PDOffset] & ~0xFFF);
    }

    PT[PTOffset] = (physicalAddress & ~0xFFF) | PRESENT | WRITABLE;
    
}

void unmap_page(uint64_t virtualAddr)
{
    page_t* pageTable;
    if(!getPageEntry(virtualAddr, &pageTable)){ //Page is not im memory, so we don't need to unmap it :)
        return;
    }

    uint64_t offset = (virtualAddr >> PT_BITS_OFFSET) & 0b111111111;
    pageTable[offset] = 0;
}
