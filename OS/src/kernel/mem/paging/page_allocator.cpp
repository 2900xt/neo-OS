#include "limine/limine.h"
#include "stdlib/assert.h"
#include <stdlib/stdio.h>
#include <types.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/mem.h>


static volatile limine::limine_memmap_request memmap_request = {LIMINE_MEMMAP_REQUEST, 0};
static limine::limine_memmap_entry **memory_entries;

namespace kernel
{


static page_list_entry_t page_list_head;

static constexpr uint64_t page_sz = 0x1000;

void initialize_page_allocator()
{
    memory_entries = memmap_request.response->entries;
    page_list_entry_t *current_page_entry = &page_list_head;
    for(int i = 0; i < memmap_request.response->entry_count; i++)
    {
        limine::limine_memmap_entry *current_mem_entry = memory_entries[i];
        current_page_entry->memory.size = current_mem_entry->length / page_sz;
        current_page_entry->memory.start = current_mem_entry->base;
        current_page_entry->memory.type = (MEMORY_TYPES)current_mem_entry->type;

        current_page_entry->next = (page_list_entry_t*)kcalloc(1, sizeof(page_list_entry_t));
        current_page_entry->next->prev = current_page_entry;
        current_page_entry = current_page_entry->next;
    }
}

void *allocate_pages(uint64_t requested_count)
{
    for(page_list_entry_t *current_page_entry = &page_list_head; current_page_entry != NULL; current_page_entry = current_page_entry->next)
    {
        if(current_page_entry->memory.type == FREE)                     //We can only give FREE memory regions
        {
            if(current_page_entry->memory.size == requested_count)      //No need to insert a page
            {
                current_page_entry->memory.type = USED;
                return (void*)current_page_entry->memory.start;
            }
            if(current_page_entry->memory.size > requested_count)       //Insert a new page in between the list
            {
                page_list_entry_t *new_entry = (page_list_entry_t*)kcalloc(1, sizeof(page_list_entry_t));

                new_entry->next = current_page_entry->next;
                new_entry->prev = current_page_entry;
                current_page_entry->next = new_entry;

                //Calculate new page size and address

                new_entry->memory.size = requested_count;
                current_page_entry->memory.size -= new_entry->memory.size;

                new_entry->memory.start = current_page_entry->memory.start + current_page_entry->memory.size * page_sz;
                new_entry->memory.type = USED;

                return (void*)new_entry->memory.start;
            }
        }
    }

    return NULL;
}

void combine_segments(page_list_entry_t *a, page_list_entry_t *b)
{
    assert(a != b);
    assert(a->memory.type == b->memory.type);

    if(a->next == b && b->prev == a)
    {
        a->memory.size += b->memory.size;
        kfree(b);
        return;
    } 
    if (a->prev == b && b->next == a) {
        b->memory.size += a->memory.size;
        kfree(a);
        return;
    }
}

void free_pages(void *page)
{
    uint64_t start_addr = (uint64_t)page;
    assert(start_addr % page_sz == 0);

    page_list_entry_t *page_entry = NULL;

    for(page_list_entry_t *current_page_entry = &page_list_head; current_page_entry != NULL; current_page_entry = current_page_entry->next)
    {
        if(current_page_entry->memory.start == start_addr) 
        {
            if(current_page_entry->memory.type != USED) 
            {
                std::klogf("Asking to free an unused page!\n");
                return;
            }
            page_entry = current_page_entry;
            break;
        }
    }

    assert(page_entry);

    page_entry->memory.type = FREE;

    if(page_entry->prev->memory.type == FREE)
    {
        combine_segments(page_entry, page_entry->prev);
    } 
}

};