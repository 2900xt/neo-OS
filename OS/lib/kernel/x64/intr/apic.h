#pragma once

#include <types.h>
#include <drivers/acpi/madt.h>

namespace kernel
{


void initAPIC(uint8_t APICid);
void apicSendEOI(void);
void sleep(int64_t millis);



void ioapic_init();

struct ioapic_redir_table_entry
{
    uint8_t vector;
    uint8_t delivery_mode : 3;
    uint8_t destination_mode : 1;
    uint8_t delivery_status : 1;
    uint8_t pin_polatiry : 1;
    uint8_t remote_irr : 1;
    uint8_t trigger_mode : 1;
    uint8_t mask : 1;
    uint64_t reserved : 39;
    uint8_t destination;
}__attribute__((packed));

void add_io_red_table_entry(uint8_t entry_number, ioapic_redir_table_entry* _entry);

}