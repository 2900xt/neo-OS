#include <types.h>
#include <limine/limine.h>
#include <stdlib/stdlib.h>

static volatile limine::limine_terminal_request terminal_request = {LIMINE_TERMINAL_REQUEST, 0};

namespace std 
{

limine::limine_terminal* console;
limine::limine_terminal_write write;

void tty_init(void)
{
    if (terminal_request.response == NULL || terminal_request.response->terminal_count == 0)
    {
        return;
    }

    // Set global variables

    write = terminal_request.response->write;
    console = terminal_request.response->terminals[0];
}

void puts(const char* src){
    write(console, src, strlen(src));
}

void putc(char c){
    write(console, &c, 1);
}

void klogf(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);

    int currentCharacter = 0;
    bool argFound = false;

    char* str;
    uint64_t num;


    while(fmt[currentCharacter] != '\0'){
    
        if(fmt[currentCharacter] != '%'){
            putc(fmt[currentCharacter++]);
            continue;
        }

        switch(fmt[currentCharacter + 1])
        {
            case 'c':
            case 'C':
                argFound = true;
                num = va_arg(args, int);
                putc(num);
                break;
            case 's':
            case 'S':
                argFound = true;
                str = va_arg(args, char*);
                puts(str);
                break;
            case 'd':
            case 'D':
                argFound = true;
                num = va_arg(args, uint64_t);
                str = itoa(num, 10);
                puts(str);
                break;
            case 'x':
            case 'X':
                argFound = true;
                num = va_arg(args, uint64_t);
                str = itoa(num, 16);
                puts(str);
                break;
            case 'b':
            case 'B':
                argFound = true;
                num = va_arg(args, uint64_t);
                str = itoa(num, 2);
                puts(str);
                break;
            default:
                argFound = false;
        }

        if(argFound){
            currentCharacter += 2;
            continue;
        }

        currentCharacter++;
    }

    va_end(args);

}

}