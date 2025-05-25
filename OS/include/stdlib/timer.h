#pragma once
#include <types.h>

namespace stdlib
{
    typedef void (*timer_handler)();

    void register_timer(timer_handler handler, uint64_t delta_ms);
    void call_timers();
    void update_timers();
}