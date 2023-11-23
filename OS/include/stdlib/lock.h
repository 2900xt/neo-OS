#pragma once

typedef bool spinlock_t;

#define SPIN_LOCKED true
#define SPIN_UNLOCKED false

inline void acquire_spinlock(spinlock_t *lock)
{
    while(*lock == SPIN_LOCKED);
    *lock = SPIN_LOCKED;
}

#define release_spinlock(lock) lock = SPIN_UNLOCKED;