#include <drivers/rtc/rtc.h>
#include <kernel/x64/io.h>
#include <kernel/io/log.h>

namespace rtc
{
    // RTC/CMOS I/O ports
    const uint8_t CMOS_ADDRESS_PORT = 0x70;
    const uint8_t CMOS_DATA_PORT = 0x71;
    
    // CMOS registers
    const uint8_t CMOS_SECONDS = 0x00;
    const uint8_t CMOS_MINUTES = 0x02;
    const uint8_t CMOS_HOURS = 0x04;
    const uint8_t CMOS_DAY = 0x07;
    const uint8_t CMOS_MONTH = 0x08;
    const uint8_t CMOS_YEAR = 0x09;
    const uint8_t CMOS_STATUS_A = 0x0A;
    const uint8_t CMOS_STATUS_B = 0x0B;
    
    uint8_t read_cmos(uint8_t reg)
    {
        kernel::outb(CMOS_ADDRESS_PORT, reg);
        return kernel::inb(CMOS_DATA_PORT);
    }
    
    bool is_update_in_progress()
    {
        kernel::outb(CMOS_ADDRESS_PORT, CMOS_STATUS_A);
        return (kernel::inb(CMOS_DATA_PORT) & 0x80);
    }
    
    uint8_t bcd_to_binary(uint8_t bcd)
    {
        return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
    }
    
    void rtc_init()
    {
        log::d("RTC", "Initializing RTC driver...");
        
        // Check if RTC is working by reading status register
        uint8_t status = read_cmos(CMOS_STATUS_B);
        log::d("RTC", "RTC Status B register: 0x%x", status);
        
        log::d("RTC", "RTC driver initialized successfully");
    }
    
    DateTime get_datetime()
    {
        DateTime dt;
        
        // Wait for any update in progress to finish
        while (is_update_in_progress());
        
        // Read time values
        uint8_t second = read_cmos(CMOS_SECONDS);
        uint8_t minute = read_cmos(CMOS_MINUTES);
        uint8_t hour = read_cmos(CMOS_HOURS);
        uint8_t day = read_cmos(CMOS_DAY);
        uint8_t month = read_cmos(CMOS_MONTH);
        uint8_t year = read_cmos(CMOS_YEAR);
        
        // Check status register B to see if time is in BCD or binary format
        uint8_t status_b = read_cmos(CMOS_STATUS_B);
        bool is_bcd = !(status_b & 0x04);
        
        // Convert BCD to binary if necessary
        if (is_bcd)
        {
            dt.second = bcd_to_binary(second);
            dt.minute = bcd_to_binary(minute);
            dt.hour = bcd_to_binary(hour);
            dt.day = bcd_to_binary(day);
            dt.month = bcd_to_binary(month);
            dt.year = bcd_to_binary(year);
        }
        else
        {
            dt.second = second;
            dt.minute = minute;
            dt.hour = hour;
            dt.day = day;
            dt.month = month;
            dt.year = year;
        }
        
        // Convert 2-digit year to 4-digit year
        // Assume years 00-79 are 2000-2079, 80-99 are 1980-1999
        if (dt.year < 80)
            dt.year += 2000;
        else
            dt.year += 1900;
        
        return dt;
    }
}