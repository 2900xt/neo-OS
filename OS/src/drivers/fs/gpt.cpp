#include "drivers/fs/gpt.h"
#include "kernel/mem/paging.h"



namespace filesystem
{

    bool cmp_guid(guid_t a, guid_t b)
    {
        for (int i = 0; i < 8; i++)
        {
            if (a[i] != b[i])
                return false;
        }

        return true;
    }

}