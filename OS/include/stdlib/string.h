#pragma once
#include "kernel/mem/mem.h"
#include "stdlib/assert.h"
#include <types.h>
#include <stdlib/stdlib.h>


namespace std 
{

size_t      strlen(const char*);
char       *itoa(int64_t val, uint8_t radix);
char       *utoa(uint64_t val, uint8_t radix);
bool        strcmp(const char* a, const char* b, int count);
const char* strcpy(char* _dest, const char* _src);
const char* strcat(char* dest, const char* src);
bool        strcmp(const char* a, const char* b);
size_t      strclen(const char *src, char term);
char        toUpper(char c);
uint64_t    atou(const char *str, uint64_t len);
char*       dtoa(double number, int max_decimals);

class string 
{
    char   *data;
    size_t  size;
    size_t  max_size;

public:

    string(const char *data)
    {
        this->size = strlen(data);
        this->data = new char[size];

        std::strcpy(this->data, data);

        this->max_size = size;
    }

    string(size_t size)
    {
        this->size = 0;
        this->max_size = size;
        this->data = new char[max_size];
    }

    string(string& other)
    {
        this->size = other.size;
        this->max_size = other.max_size;
        this->data = new char[max_size];
        
        strcpy(data, other.c_str());
    }

    string()
    {
        this->size = 0;
        this->max_size = 10;
        this->data = new char[max_size];
    }

    ~string()
    {
        if(this->data != NULL)
        {
            delete[] this->data;
        }
    }

    size_t length()
    {
        return size;
    }

    size_t capacity()
    {
        return max_size;
    }

    char *c_str()
    {
        return data;
    }

    void clear()
    {
        memset_8(data, size, 0);
    }

    void resize(size_t requested)
    {
        this->max_size = requested;
        
        char* newData = new char[requested];
        strcpy(newData, data);
        this->data = newData;
    }

    char at(size_t ind)
    {
        assert(ind < size && ind >= 0);
        return data[ind];
    }

    void set(size_t ind, char c)
    {
        assert(ind < size && ind >= 0);
        data[ind] = c;
    }

    void append(char c)
    {
        if(size >= max_size - 1)
        {
            resize(max_size + 10);
        }
        
        data[size++] = c;
        data[size] = '\0';
    }

    void append(std::string& str)
    {
        if(max_size - 1 <= size + str.size)
        {
            resize(str.size + size + 10);
        }
        
        for(int i = size; i < str.size + size; i++)
        {
            //Log.e("s", "c:%d", data[i]);
            data[i] = str.at(i - size);
        }

        size += str.size;

        data[size] = '\0';
    }

    string** split(char c, int* count)
    {
        //get number of ocurrences
        *count = 0;
        for(int i = 0; i < size; i++)
        {
            if(data[i] == c) (*count)++;
        }

        (*count)++;

        string **out = new string*[*count];
        int outIndex = 0;
        out[outIndex] = new string;

        for(int i = 0; i < size; i++)
        {
            if(data[i] == c)
            {
                i++;
                outIndex++;
                out[outIndex] = new string;
            }

            out[outIndex]->append(data[i]);
        }

        return out;
    }

};

}