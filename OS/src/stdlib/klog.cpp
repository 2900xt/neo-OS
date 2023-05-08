#include <types.h>
#include <config.h>
#include <limine/limine.h>
#include <stdlib/stdlib.h>
#include <drivers/serial/serial.h>

static volatile limine::limine_terminal_request terminal_request = {LIMINE_TERMINAL_REQUEST, 0};

namespace std 
{

limine::limine_terminal* console;
limine::limine_terminal_write write;
static bool serial_output;

void tty_init(void)
{
    if (terminal_request.response == NULL || terminal_request.response->terminal_count == 0)
    {
        write = (limine::limine_terminal_write)SERIAL::serial_write;
        console = NULL;
        serial_output = true;
        return;
    }

    write = (SERIAL_OUTPUT_ENABLE ? (limine::limine_terminal_write)SERIAL::serial_write : terminal_request.response->write);
    console = (SERIAL_OUTPUT_ENABLE ? NULL : terminal_request.response->terminals[0]);
    serial_output = SERIAL_OUTPUT_ENABLE;
}

void puts(const char *src){
    write(console, src, strlen(src));
}

//Print unicode-16
void puts_16(const char *src)
{
    while(*src != '\0')
    {
        putc(*src);
        src += 2;
    }
}

void putc(char c){
    write(console, &c, 1);
    if(serial_output && c == '\n') {
        putc('\r');
    }
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
            case 'c':   //Char
            case 'C':
                argFound = true;
                num = va_arg(args, int);
                putc(num);
                break;
            case 's':   //String
            case 'S':
                argFound = true;
                str = va_arg(args, char*);
                puts(str);
                break;
            case 'u':   //Unsigned Integer
            case 'U':
                argFound = true;
                num = va_arg(args, uint64_t);
                str = utoa(num, 10);
                puts(str);
                break;
            case 'd':
            case 'D':   //Signed integer
                argFound = true;
                num = va_arg(args, int64_t);
                str = itoa(num, 10);
                puts(str);
                break;
            case 'x':   //Hexadecimal unsigned int
            case 'X':
                argFound = true;
                num = va_arg(args, uint64_t);
                str = utoa(num, 16);
                puts(str);
                break;
            case 'b':   //Binary
            case 'B':
                argFound = true;
                num = va_arg(args, uint64_t);
                str = utoa(num, 2);
                puts(str);
                break;
            case 'l':
            case 'L':
                str = va_arg(args, char*);
                puts_16(str);
                argFound = true;
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
