#include "types.h"
#include <stdlib/timer.h>


timer_handler handlers[10];
uint64_t handler_cooldown[10];
uint64_t reset_cooldown[10];
int handler_count = 0;

extern uint64_t millis_since_boot;

void register_timer(timer_handler handler, uint64_t delta_ms)
{
    handlers[handler_count] = handler;
    handler_cooldown[handler_count] = delta_ms;
    reset_cooldown[handler_count] = delta_ms;
    handler_count++;
}

void update_timers()
{
    for(int i = 0; i < handler_count; i++)
    {
        handler_cooldown[i]--;
    }
}

void call_timers()
{
    for(int i = 0; i < handler_count; i++)
    {
        if(handler_cooldown[i] <= 0)
        {
            handlers[i]();
            handler_cooldown[i] = reset_cooldown[i];
        }
    }
}