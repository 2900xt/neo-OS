#include "kernel/mem/mem.h"
#include "stdlib/assert.h"
#include "stdlib/math.h"
#include "stdlib/stdio.h"
#include <types.h>
#include <stdlib/string.h>
namespace std 
{

size_t strlen(const char* src){
    size_t length = 0;
    while(*src++ != '\0'){
        length++;
    }
    return length;
}

size_t strclen(const char *src, char term)
{
    size_t length = 0;
    while(*src != '\0' && *src != term){
        length++;
        src++;
    }
    return length;
}

const char *g_HexChars = "0123456789ABCDEF";
static char itoaOutput[64];
char *utoa(uint64_t val, uint8_t radix){

    char buffer[64];

    for(int i = 0; i < 64; i++){
        itoaOutput[i] = 0;
    }

    int pos = 0;
    do {
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

char *itoa(int64_t val, uint8_t radix)
{
    for(int i = 0; i < 64; i++){
        itoaOutput[i] = 0;
    }
    
    char buffer[64];
    bool negative = false;
    if(val < 0)
    {
        negative = true;
        val *= -1;
    }

    int pos = 0;
    do {
        uint64_t remainder = val % radix;
        val /= radix;
        buffer[pos++] = g_HexChars[remainder];
    } while(val > 0);

    int _pos = 0;
    if(negative)
    {
        itoaOutput[_pos++] = '-';
    }
    while(--pos >= 0){
        itoaOutput[_pos++] = buffer[pos];
    }
    return itoaOutput;
}

char toUpper(char c)
{
    if(c >= 97)
    {
        return c - 32;
    }

    return c;
}



bool strcmp(const char* a, const char* b, int count)
{
    for(int i = 0; i < count; i++){
        if(a[i] != b[i]) return false;
    }

    return true;
}

bool strcmp(const char* a, const char* b)
{
    size_t len = strlen(a);
    if(len != strlen(b)) return false;
    return strcmp(a, b, len);
}

const char* strcat(char* dest, const char* src)
{
    char* _dest = dest;
    dest += strlen(dest);
    strcpy(dest, src);
    return _dest;
}

const char* strcpy(char* _dest, const char* _src)
{
    char* dest = _dest;
    while(*_src != '\0')
    {
        *dest = *_src;
        dest++;
        _src++;
    }
    return _dest;
}

}