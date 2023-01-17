#include <limine/limine.h>
#include <stdout.h>
#include <arch/amd64/io.h>
#include <mem.h>
#include <acpi/tables.h>
#include <vga/vga.h>
#include <math.h>
#include <vga/gfx.h>
#include <drivers/mouse.h>


static volatile limine::limine_terminal_request terminal_request {LIMINE_TERMINAL_REQUEST, 0};


void neoSTL::done(void) {
    for (;;) {
        processMousePacket();
    }
}

//Entry point ->

extern "C" void _start(void) {

    enableSSE();

    //Check if we got a console

    if (terminal_request.response == NULL
    || terminal_request.response->terminal_count == 0) {
        neoSTL::done();
    }

    //Set global variables
    
    neoSTL::write = terminal_request.response->write;
    neoSTL::console = terminal_request.response->terminals[0];

    mouseINIT();
    
    fillIDT();

    neoSTL::heapInit(0x100000, 0x100000, 0x100);

    void* FADT = findACPITable("APIC");
    neoSTL::printf("MADT: 0x%x\n", FADT);

    fbuf_init();


    neoSTL::done();
}
