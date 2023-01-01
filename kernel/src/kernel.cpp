#include <limine/limine.h>
#include <stdout.h>
#include <arch/amd64/io.h>
#include <mem.h>

static volatile limine::limine_terminal_request terminal_request {LIMINE_TERMINAL_REQUEST, 0};


void neoOS_STD::done(void) {
    for (;;) {
        asm("hlt");
    }
}

//Entry point ->

extern "C" void _start(void) {

    //Check if we got a console

    if (terminal_request.response == NULL
    || terminal_request.response->terminal_count == 0) {
        neoOS_STD::done();
    }

    //Set global variables
    
    neoOS_STD::write = terminal_request.response->write;
    neoOS_STD::console = terminal_request.response->terminals[0];

    fillIDT();

    heapInit(0x100000, 0x100000, 0x100);

    neoOS_STD::done();
}
