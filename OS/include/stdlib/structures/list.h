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
        size_t _size;
        size_t max_capacity;
    public:
        list(size_t size)
        {
            this->_size = 0;
            this->max_capacity = size;
            this->_data = new T[max_capacity];
        }

        list(size_t size, T default_value)
        {
            this->_size = size;
            this->max_capacity = size;
            this->_data = new T[max_capacity];
            for (size_t i = 0; i < size; i++)
            {
                _data[i] = default_value;
            }
        }

        list(const list &other)
        {
            this->_size = other._size;
            this->max_capacity = other.max_capacity;
            this->_data = new T[max_capacity];
            for(int i = 0; i < this->_size; i++)
            {
                this->_data[i] = other._data[i];
            }
        }

        list(list &&other) noexcept
        {
            this->_size = other._size;
            this->max_capacity = other.max_capacity;
            this->_data = other._data;
            other._data = NULL;
            other._size = 0;
            other.max_capacity = 0;
        }

        list& operator=(const list &other)
        {
            if(&other != this)
            {
                delete [] this->_data;
                this->_size = other._size;
                this->max_capacity = other.max_capacity;
                this->_data = new T[max_capacity];
                for(int i = 0; i < this->_size; i++)
                {
                    this->_data[i] = other._data[i];
                }
            }

            return *this;
        }

        list& operator=(list &&other) noexcept
        {
            if(&other != this)
            {
                delete [] this->_data;
                this->_size = other._size;
                this->max_capacity = other.max_capacity;
                this->_data = other._data;
                other._data = NULL;
                other._size = 0;
                other.max_capacity = 0;
            }

            return *this;
        }

        list()
        {
            this->_size = 0;
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

        size_t size() const
        {
            return _size;
        }

        T *data()
        {
            assert(_data != NULL);
            return _data;
        }

        const T *data() const
        {
            assert(_data != NULL);
            return _data;
        }

        void clear()
        {
            this->_size = 0;
        }

        void resize(size_t requested)
        {
            this->max_capacity = requested;

            T *newData = new T[requested];
            for(int i = 0; i < _size;i++)
            {
                newData[i] = _data[i];
            }

            delete[] _data;

            this->_data = newData;
        }

        T &at(size_t ind)
        {
            assert(ind < _size);
            return _data[ind];
        }

        const T &at(size_t ind) const
        {
            assert(ind < _size);
            return _data[ind];
        }

        T &operator[](size_t ind)
        {
            assert(ind < _size);
            return _data[ind];
        }

        const T &operator[](size_t ind) const
        {
            assert(ind < _size);
            return _data[ind];
        }

        void set(size_t ind, const T& c)
        {
            assert(ind < _size);
            _data[ind] = c;
        }

        void push_back(const T& c)
        {
            if (_size >= max_capacity)
            {
                resize(max_capacity * 2+1);
            }

            _data[_size++] = c;
        }

        void pop_back()
        {
            assert(_size > 0);
            _size--;
        }

        void append(const list<T> &other)
        {
            if (max_capacity <= _size + other._size)
            {
                resize((other._size + _size) * 2+1);
            }

            for (int i = _size; i < other._size + _size; i++)
            {
                _data[i] = other.at(i - _size);
            }

            _size += other._size;
        }

        void erase(size_t ind)
        {
            assert(ind < _size);

            for (size_t i = ind; i < _size - 1; i++)
            {
                _data[i] = _data[i + 1];
            }

            _size--;
        }
    };
}
#endif