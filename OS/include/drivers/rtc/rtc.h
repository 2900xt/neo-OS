#ifndef RTC_H
#define RTC_H

#include <types.h>

namespace rtc
{
    struct DateTime
    {
        uint8_t second;
        uint8_t minute;
        uint8_t hour;
        uint8_t day;
        uint8_t month;
        uint16_t year;
    };

    void rtc_init();
    DateTime get_datetime();
    uint8_t bcd_to_binary(uint8_t bcd);
}

#endif