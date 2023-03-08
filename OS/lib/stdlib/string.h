#pragma once
#include <types.h>


namespace std 
{

size_t      strlen(const char*);
char*       itoa(uint64_t val, uint8_t radix);
bool        strcmp(const char* a, const char* b, int count);
const char* strcpy(char* _dest, const char* _src);
const char* strcat(char* dest, const char* src);
bool        strcmp(const char* a, const char* b);

}