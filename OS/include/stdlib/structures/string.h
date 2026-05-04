#pragma once
#include "kernel/mem/mem.h"
#include "stdlib/assert.h"
#include "stdlib/structures/list.h"
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


    int strcasecmp(const char *s1, const char *s2);
    int strncasecmp(const char *s1, const char *s2, size_t num);

    class string
    {
        char *data;
        size_t size;
        size_t max_capacity;

    public:
        string(const char *data)
        {
            this->size = strlen(data);
            this->max_capacity = this->size;
            this->data = new char[this->size+1];

            stdlib::strcpy(this->data, data);

        }

        string(size_t size, char c = ' ')
        {
            this->size = size;
            this->max_capacity = this->size;
            this->data = new char[this->size+1];
            for(int i = 0; i < size; i++)
            {
                this->data[i] = c;
            }
            this->data[size] = '\0';
        }

        string(const string &other)
        {
            this->size = other.size;
            this->max_capacity = other.max_capacity;
            this->data = new char[max_capacity+1];

            strcpy(data, other.data);
        }

        // Move Constructor
        string(string &&other) noexcept
        {
            this->size = other.size;
            this->max_capacity = other.max_capacity;
            this->data = other.data;
            other.data = NULL;
            other.size = 0;
            other.max_capacity = 0;
        }

        string& operator=(const string &other) 
        {
            if (this != &other)
            {
                delete[] this->data;

                this->size = other.size;
                this->max_capacity = other.max_capacity;
                this->data = new char[max_capacity+1];

                strcpy(data, other.data);
            }
            return *this;
        }

        string& operator=(string &&other) noexcept
        {
            if(this != &other) 
            {
                delete [] this->data;

                this->size = other.size;
                this->max_capacity = other.max_capacity;
                this->data = other.data;
                other.data = NULL;
                other.size = 0;
                other.max_capacity = 0;
            }
            return *this;
        }

        string()
        {
            this->size = 0;
            this->max_capacity = 10;
            this->data = new char[max_capacity+1];
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
            size = 0;
            data[0] = '\0';
        }

        void resize(size_t requested)
        {
            this->max_capacity = requested;

            char *newData = new char[requested+1];
            strcpy(newData, data);

            delete[] data;

            this->data = newData;
        }

        char &at(size_t ind)
        {
            assert(ind < size);
            return data[ind];
        }

        const char &at(size_t ind) const
        {
            assert(ind < size);
            return data[ind];
        }

        char &operator[](size_t ind)
        {
            assert(ind < size);
            return data[ind];
        }

        const char &operator[](size_t ind) const
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
            if (size+1 >= max_capacity)
            {
                resize(max_capacity * 2 + 1);
            }

            data[size++] = c;
            data[size] = '\0';
        }

        void append(const string &str)
        {
            if (max_capacity <= size + str.size)
            {
                resize((str.size + size) * 2 + 1);
            }

            for (int i = size; i < str.size + size; i++)
            {
                data[i] = str[i - size];
            }

            size += str.size;

            data[size] = '\0';
        }

        list<string> split(char c) const
        {
            list<string> out;
            out.push_back(string());
            for (int i = 0; i < size; i++)
            {
                if (data[i] == c)
                {
                    out.push_back(string());
                    continue;
                }

                out[out.size() - 1].push_back(data[i]);
            }

            return out;
        }

        string substr(int start, int len) const
        {
            assert(start + len <= size && start >= 0);
            string ret(len);
            for(int i = 0; i < len; i++)
            {
                ret[i] = data[i+start];
            }
            return ret;
        }

        string substr(int start) const
        {
            assert(start >= 0);
            int len = size - start;
            string ret(len);
            for(int i = 0; i < len; i++)
            {
                ret[i] = data[i+start];
            }
            return ret;
        }

        int find(int start, char c) const 
        {
            for(int i = start; i < size; i++)
            {
                if(data[i] == c) return i;
            }

            return -1;
        }

        bool operator==(const string &other) const
        {
            if (size != other.size)
                return false;
            return strcmp(data, other.data);
        }
    };

}