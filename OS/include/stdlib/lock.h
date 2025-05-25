#pragma once

#include <types.h>

namespace stdlib
{
    typedef uint8_t spinlock_t;

    constexpr spinlock_t SPIN_LOCKED = 1;
    constexpr spinlock_t SPIN_UNLOCKED = 0;

    inline void acquire_spinlock(spinlock_t *lock)
    {
        while (*lock == SPIN_LOCKED)
            asm volatile("pause");
        *lock = SPIN_LOCKED;
    }

    inline void release_spinlock(spinlock_t *lock)
    {
        *lock = SPIN_UNLOCKED;
    }
}