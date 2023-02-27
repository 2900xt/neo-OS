#pragma once
#include <types.h>


namespace std 
{

size_t      strlen(const char*);
char*       itoa(uint64_t val, uint8_t radix);
bool        strcmp(const char* a, const char* b, int count);
bool        strcmp(const char* a, const char* b);

}