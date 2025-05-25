#pragma once

#include "stdlib/lock.h"
#include "stdlib/string.h"
namespace kernel
{
    struct stream
    {
        stdlib::string *data;
        stdlib::spinlock_t rw_lock;
        bool ack_update;
    };

    void stream_flush(stream *stream);
    void stream_write(stream *ostream, char data);
    stdlib::string *stream_read(stream *istream);
    void stream_write(stream *ostream, stdlib::string *data);
}