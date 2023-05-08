#include <stdlib/stdlib.h>
#include <stdlib/structures/bitmap.h>
#include <kernel/mem/mem.h>

namespace std
{

void bitmap::init(size_t _sz_bits)
{
    data = (uint64_t*)kcalloc(1, (_sz_bits / 64) + 1);

    assert(data != NULL);

    this->sz_bits = _sz_bits;
}

void bitmap::free(void)
{
    kfree(data);
}

bool bitmap::get(uint64_t pos)
{
    assert(pos <= sz_bits);

    uint64_t offset = pos % 64;
    pos /= 64;

    return ((data[pos] >> offset) & 0b1);
}

void bitmap::set(uint64_t pos, bool value)
{
    assert(pos <= sz_bits);

    uint64_t offset = pos % 64;
    pos /= 64;

    if(value)
    {
        data[pos] = data[pos] | (1 << offset);
    } else {
        data[pos] = data[pos] & (0 << offset);
    }
}


void bitmap::set_chunk(uint64_t pos, uint64_t val)
{
    assert(pos <= (sz_bits / 64));
    data[pos] = val;
}

uint64_t bitmap::get_chunk(uint64_t pos)
{
    assert(pos <= (sz_bits / 64));
    return data[pos];
}

uint64_t* bitmap::get_data(void)
{
    return data;
}

size_t bitmap::get_size(void)
{
    return sz_bits;
}

}