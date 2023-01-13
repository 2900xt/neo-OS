#include <limine/limine.h>
#include <stdout.h>
#include <arch/amd64/io.h>
#include <mem.h>

static volatile limine::limine_terminal_request terminal_request {LIMINE_TERMINAL_REQUEST, 0};


void neoSTL::done(void) {
    for (;;) {
        asm("hlt");
    }
}

//Entry point ->

extern "C" void _start(void) {

    //Check if we got a console

    if (terminal_request.response == NULL
    || terminal_request.response->terminal_count == 0) {
        neoSTL::done();
    }

    //Set global variables
    
    neoSTL::write = terminal_request.response->write;
    neoSTL::console = terminal_request.response->terminals[0];

    fillIDT();

    neoSTL::heapInit(0x100000, 0x100000, 0x100);

    uint64_t* ptr = (uint64_t*)0x1;
    neoSTL::map_page(0x1, 0x1);
    neoSTL::unmap_page(0x1);
    uint64_t physicalAddr = neoSTL::getPhysicalAddress((uint64_t)ptr);
    //*ptr = 1;

    neoSTL::printf("PA:\t0x%x\n", physicalAddr);
    neoSTL::done();
}
