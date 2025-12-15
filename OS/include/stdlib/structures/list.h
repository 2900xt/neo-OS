#ifndef _STDLIB_STRUCTURES_LIST_H
#define _STDLIB_STRUCTURES_LIST_H
#include <types.h>   
#include <kernel/mem/mem.h>
#include <stdlib/assert.h>

namespace stdlib
{
    template <typename T>
    class list
    {
        T *_data;
        size_t size;
        size_t max_capacity;

    public:
        list(size_t size)
        {
            this->size = 0;
            this->max_capacity = size;
            this->_data = new T[max_capacity];
        }

        list(list &other)
        {
            this->size = other.size;
            this->max_capacity = other.max_capacity;
            this->_data = new T[max_capacity];
            kernel::memcpy(_data, other.data(), this->size*sizeof(T));
        }

        list()
        {
            this->size = 0;
            this->max_capacity = 10;
            this->_data = new T[max_capacity];
        }

        ~list()
        {
            if (this->_data != NULL)
            {
                delete[] this->_data;
            }
        }

        size_t length()
        {
            return size;
        }

        size_t capacity()
        {
            return max_capacity;
        }

        T *data()
        {
            assert(_data != NULL);
            return _data;
        }

        void clear()
        {
            this->size = 0;
            kernel::memset(_data, size*sizeof(T), 0);
        }

        void resize(size_t requested)
        {
            this->max_capacity = requested;

            T *newData = new T[requested];
            kernel::memcpy(newData, _data, size*sizeof(T));

            delete[] _data;

            this->_data = newData;
        }

        T &at(size_t ind)
        {
            assert(ind < size && ind >= 0);
            return _data[ind];
        }

        T &operator[](size_t ind)
        {
            assert(ind < size && ind >= 0);
            return _data[ind];
        }

        void set(size_t ind, T c)
        {
            assert(ind < size && ind >= 0);
            _data[ind] = c;
        }

        void push_back(T c)
        {
            if (size >= max_capacity - 1)
            {
                resize(max_capacity * 2);
            }

            _data[size++] = c;
            _data[size] = '\0';
        }

        void pop_back()
        {
            assert(size > 0);
            size--;
            _data[size] = '\0';
        }

        void append(list<T> &other)
        {
            if (max_capacity - 1 <= size + other.size)
            {
                resize(other.size + size * 2);
            }

            for (int i = size; i < other.size + size; i++)
            {
                _data[i] = other.at(i - size);
            }

            size += other.size;

            _data[size] = '\0';
        }

        void erase(size_t ind)
        {
            assert(ind < size && ind >= 0);

            for (size_t i = ind; i < size - 1; i++)
            {
                _data[i] = _data[i + 1];
            }

            size--;
        }
    };
}
#endif