#include "drivers/fs/gpt.h"
#include "kernel/mem/paging.h"
#include "stdlib/stdio.h"
#include <types.h>

namespace FS 
{

static guid_t esp_guid = {0x7328, 0xC12A, 0xF81F, 0x11D2, 0x4BBA, 0xA000, 0x3EC9, 0x3BC9};


bool cmp_guid(guid_t a, guid_t b)
{
    for(int i = 0; i < 8; i++)
    {
        if(a[i] != b[i]) return false;
    }

    return true;
}

}