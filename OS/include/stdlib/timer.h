#pragma once
#include <types.h>

typedef void (*timer_handler)();

void register_timer(timer_handler handler, uint64_t delta_ms);
void call_timers();
void update_timers();