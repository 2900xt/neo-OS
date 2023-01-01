#include <limine/limine.h>
#include <stdout.h>


namespace neoOS_STD{

limine::limine_terminal* console;
limine::limine_terminal_write write;

size_t strlen(char* src){
    size_t length = 0;
    while(*src++ != '\0'){
        length++;
    }
    return length;
}

const char g_HexChars[] = "0123456789ABCDEF";
static char itoaOutput[32];
char* itoa(int64_t val, uint8_t radix){

    char buffer[32];

    for(int i = 0; i < 32; i++){
        buffer[i] = 0;
    }

    int pos = val < 0;
    if(pos){
        val *= -1;
        buffer[0] = '-';
    }

    do{
        uint64_t remainder = val % radix;
        val /= radix;
        buffer[pos++] = g_HexChars[remainder];
    } while(val > 0);

    int _pos = 0;
    while(--pos >= 0){
        itoaOutput[_pos++] = buffer[pos];
    }

    return itoaOutput;
}

void puts(char* src){
    write(console, src, strlen(src));
}

char _c[2] = {0, 0};

void putc(char c){
    _c[0] = c;
    write(console, _c, 1);
}

void printf(const char* fmt, ...){
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
                num = va_arg(args, int);
                str = itoa(num, 10);
                puts(str);
                break;
            case 'x':
            case 'X':
                argFound = true;
                num = va_arg(args, int);
                str = itoa(num, 16);
                puts(str);
                break;
            case 'b':
            case 'B':
                argFound = true;
                num = va_arg(args, int);
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