#include <kernel/kernel.h>
#include <stdlib/stdlib.h>

namespace kernel
{
    process_t *task_list[100];
    stream *kernel_stdout, *kernel_stdin;

    process_t *create_task(proc_main main)
    {
        process_t *proc = new process_t;

        uint64_t pid = 0;
        while (task_list[pid++] != NULL)
            ;
        proc->pid = pid - 1;
        task_list[proc->pid] = proc;

        proc->exited = false;
        proc->started = false;

        return proc;
    }

    void yeild(process_t *proc, int exit_code)
    {
        uint64_t pid = 99;
        for (; pid >= 0; pid++)
        {
            if (task_list[pid] != NULL && task_list[pid] != proc && !task_list[pid]->started)
            {
                break;
            }
        }

        proc->exit_code = exit_code;
        proc->exited = true;
        task_list[pid]->started = true;
        ((proc_main)task_list[pid]->main)(task_list[pid]);
    }

    void stream_write(stream *ostream, stdlib::string *data)
    {
        stdlib::acquire_spinlock(&ostream->rw_lock);

        if (ostream->data == NULL)
        {
            ostream->data = new stdlib::string();
        }

        ostream->data->append(*data);

        ostream->ack_update = true;

        stdlib::release_spinlock(&ostream->rw_lock);
    }

    void stream_write(stream *ostream, char data)
    {
        stdlib::acquire_spinlock(&ostream->rw_lock);

        if (ostream->data == NULL)
        {
            ostream->data = new stdlib::string();
        }

        ostream->data->append(data);

        ostream->ack_update = true;

        stdlib::release_spinlock(&ostream->rw_lock);
    }

    void stream_flush(stream *stream)
    {
        stdlib::acquire_spinlock(&stream->rw_lock);

        kfree(stream->data);
        stream->data = NULL;
        stream->ack_update = false;

        stdlib::release_spinlock(&stream->rw_lock);
    }

    stdlib::string *stream_read(stream *istream)
    {
        stdlib::string *data;

        stdlib::acquire_spinlock(&istream->rw_lock);

        if (istream->data == NULL)
        {
            data = new stdlib::string();
        }
        else
        {
            data = new stdlib::string(*istream->data);
        }

        istream->ack_update = false;

        stdlib::release_spinlock(&istream->rw_lock);

        return data;
    }
}