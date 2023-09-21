#pragma once

#include "stdlib/lock.h"
#include "stdlib/string.h"
struct stream
{
    std::string data;
    spinlock_t* rw_lock;
    bool ack_update;
};