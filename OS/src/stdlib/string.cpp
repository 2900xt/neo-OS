#include <types.h>

namespace std 
{

size_t strlen(const char* src){
    size_t length = 0;
    while(*src++ != '\0'){
        length++;
    }
    return length;
}

const char g_HexChars[] = "0123456789ABCDEF";
static char itoaOutput[64];
char* itoa(uint64_t val, uint8_t radix){

    char buffer[64];

    for(int i = 0; i < 32; i++){
        itoaOutput[i] = 0;
    }

    int pos = 0;
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


bool strcmp(const char* a, const char* b, int count)
{

    for(int i = 0; i < count; i++){
        if(a[i] != b[i]) return false;
    }

    return true;
}

}