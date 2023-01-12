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

typedef uint64_t page_t;

uint64_t readCR3(void)
{
    uint64_t cr3;
    asm("mov %%cr3, %0" : "=a"(cr3));
    return cr3;
}


bool getPageEntry(uint64_t virtualAddr, page_t* pageEntry)
{

    neoSTL::printf("InputAddr: 0x%x\n", virtualAddr);

    uint64_t physicalOffset  = virtualAddr & 0xFFF;                                   //Bits 0 - 11
    uint64_t PTOffset        = (virtualAddr >> 12) & 0b111111111;                     //Bits 12 - 20
    uint64_t PDOffset        = (virtualAddr >> 21) & 0b111111111;                     //Bits 21 - 29
    uint64_t PDPTOffset      = (virtualAddr >> 30) & 0b111111111;                     //Bits 30 - 38
    uint64_t PM4LTOffset     = (virtualAddr >> 39) & 0b111111111;                     //Bits 39 - 47
    

    page_t* PM4LT   = (page_t*)((readCR3()) & ~0xFFF);
    page_t  PM4LE   = PM4LT[PM4LTOffset];
    if(~PM4LE & PRESENT) return false;

    /*neoSTL::printf
    (
        "PM4LT\t0x%x\nPM4LE\t0x%x\n",
        PM4LT,
        PM4LE
    );*/

    page_t* PDPT    = (page_t*)((PM4LE) & ~0xFFF);
    page_t  PDPTE   = PDPT[PDPTOffset];

    
    if(~PDPTE & PRESENT) return false;

    /*neoSTL::printf
    (
        "PDPT\t0x%x\nPDPTE\t0x%x\n",
        PDPT,
        PDPTE
    );*/

    page_t* PD      = (page_t*)((PDPTE) & ~0xFFF);
    page_t  PDE     = PD[PDOffset];

    
    if(~PDE & PRESENT) return false;

    /*neoSTL::printf
    (
        "PD\t0x%x\nPDE\t0x%x\n",
        PD,
        PDE
    );*/

    page_t* PT      = (page_t*)((PDE)& ~0xFFF);
    page_t  PTE     = PT[PTOffset];

    if(~PTE & PRESENT) return false;

    /*neoSTL::printf
    (
        "PT\t0x%x\nPTE\t0x%x\n",
        PT,
        PTE
    );*/

    *pageEntry      = (PTE & ~0xFFF) | physicalOffset;


    return true;
}

uint64_t getPhysicalAddress(uint64_t virtualAddress)
{
    page_t page = 0;
    if(!getPageEntry(virtualAddress, &page)){
        neoSTL::printf("Page is not present in memory!\n");
        return NULL;
    }

    return page;
}