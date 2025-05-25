#pragma once

#include <types.h>
#include <stdlib/stdlib.h>

namespace stdlib
{

class bitmap
{

protected:

    uint64_t *data;
    size_t sz_bits;

public:
    void init(size_t _sz_bits);
	void free(void);

    uint64_t get_chunk(uint64_t pos);
    void set_chunk(uint64_t pos, uint64_t val);
    bool get(uint64_t pos);
    void set(uint64_t pos, bool val);

    size_t get_size(void);
    uint64_t* get_data(void);
};

};