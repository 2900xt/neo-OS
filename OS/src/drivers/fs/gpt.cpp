#include "drivers/fs/gpt.h"
#include "kernel/mem/paging.h"
#include "stdlib/stdio.h"
#include <types.h>

namespace FS 
{

bool cmp_guid(guid_t a, guid_t b)
{
    for(int i = 0; i < 8; i++)
    {
        if(a[i] != b[i]) return false;
    }

    return true;
}

}