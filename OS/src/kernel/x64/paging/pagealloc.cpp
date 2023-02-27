#include "stdlib/stdio.h"
#include "types.h"
#include <kernel/x64/paging.h>
#include <stdlib/structures/bitmap.h>
#include <kernel/mem.h>

namespace AMD64
{

static constexpr uint64_t page_sz = 0x1000;
static size_t allocator_sz;

static std::bitmap *page_map;

static uint64_t allocator_base;
static uint64_t allocator_limit;

void initialize_allocator(void *base, void *end)
{
    allocator_base = (uint64_t)base;
    allocator_limit = (uint64_t)end;
    allocator_sz = allocator_limit - allocator_base;

    std::klogf("Starting Page Allocater at Address 0x%x\tLength: 0x%x\n", allocator_base, allocator_sz);

    assert(allocator_base % page_sz == 0);
    assert(allocator_limit % page_sz == 0);

    page_map = (std::bitmap*)kcalloc(1, sizeof(std::bitmap));
    page_map->init(allocator_sz / page_sz);

}

void* next_page(void)
{
    uint64_t current_index = 0;
    while(current_index < page_map->get_size())
    {
        if(!page_map->get(current_index))
        {
            break;
        }
        current_index++;
    }

    if(current_index == page_map->get_size() - 1) return NULL;

    page_map->set(current_index, true);

    uint64_t address = ((current_index * page_sz) + allocator_base);
    map_page(address, address);

    return (void*)address;
}

void free_page(void *_pg)
{
    uint64_t page = (uint64_t)_pg;
    assert(page % 4000 == 0);
    assert(page > allocator_base && page < allocator_limit);
    
    uint64_t index = (page - allocator_base) / page_sz;
    page_map->set(index, false);
}

}