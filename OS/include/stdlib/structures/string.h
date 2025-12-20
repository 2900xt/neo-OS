#pragma once
#include "kernel/mem/mem.h"
#include "stdlib/assert.h"
#include <types.h>

namespace stdlib
{
    size_t strlen(const char *);
    char *itoa(int64_t val, uint8_t radix);
    char *utoa(uint64_t val, uint8_t radix);
    bool strcmp(const char *a, const char *b, int count);
    const char *strcpy(char *_dest, const char *_src);
    const char *strcat(char *dest, const char *src);
    bool strcmp(const char *a, const char *b);
    size_t strclen(const char *src, char term);
    char toUpper(char c);
    uint64_t atou(const char *str, uint64_t len);
    char *dtoa(double number, int max_decimals);    
    void sprintf(char* buffer, const char* format, ...);


    class string
    {
        char *data;
        size_t size;
        size_t max_capacity;

    public:
        string(const char *data)
        {
            this->size = strlen(data);
            this->data = new char[size];

            stdlib::strcpy(this->data, data);

            this->max_capacity = size;
        }

        string(size_t size)
        {
            this->size = 0;
            this->max_capacity = size;
            this->data = new char[max_capacity];
        }

        string(const string &other)
        {
            this->size = other.size;
            this->max_capacity = other.max_capacity;
            this->data = new char[max_capacity];

            strcpy(data, other.data);
        }

        string& operator=(const string &other)
        {
            if (this != &other)
            {
                delete[] this->data;

                this->size = other.size;
                this->max_capacity = other.max_capacity;
                this->data = new char[max_capacity];

                strcpy(data, other.data);
            }
            return *this;
        }

        string()
        {
            this->size = 0;
            this->max_capacity = 10;
            this->data = new char[max_capacity];
        }

        ~string()
        {
            if (this->data != NULL)
            {
                delete[] this->data;
            }
        }

        size_t length() const
        {
            return size;
        }

        size_t capacity() const
        {
            return max_capacity;
        }

        char *c_str()
        {
            return data;
        }

        const char *c_str() const
        {
            return data;
        }

        void clear()
        {
            kernel::memset(data, size, 0);
        }

        void resize(size_t requested)
        {
            this->max_capacity = requested;

            char *newData = new char[requested];
            strcpy(newData, data);

            delete[] data;

            this->data = newData;
        }

        char &at(size_t ind)
        {
            assert(ind < size);
            return data[ind];
        }

        char &operator[](size_t ind)
        {
            assert(ind < size);
            return data[ind];
        }

        void set(size_t ind, char c)
        {
            assert(ind < size);
            data[ind] = c;
        }

        void push_back(char c)
        {
            if (size >= max_capacity - 1)
            {
                resize(max_capacity * 2);
            }

            data[size++] = c;
            data[size] = '\0';
        }

        void append(stdlib::string &str)
        {
            if (max_capacity - 1 <= size + str.size)
            {
                resize(str.size + size * 2);
            }

            for (int i = size; i < str.size + size; i++)
            {
                data[i] = str.at(i - size);
            }

            size += str.size;

            data[size] = '\0';
        }

        string **split(char c, int *count)
        {
            // get number of ocurrences
            *count = 0;
            for (int i = 0; i < size; i++)
            {
                if (data[i] == c)
                    (*count)++;
            }

            (*count)++;

            string **out = new string *[*count];
            int outIndex = 0;
            out[outIndex] = new string;

            for (int i = 0; i < size; i++)
            {
                if (data[i] == c)
                {
                    i++;
                    outIndex++;
                    out[outIndex] = new string;
                }

                out[outIndex]->push_back(data[i]);
            }

            return out;
        }

        bool operator==(const string &other) const
        {
            if (size != other.size)
                return false;
            return strcmp(data, other.data);
        }
    };

}