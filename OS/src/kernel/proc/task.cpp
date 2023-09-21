#include <kernel/proc/proc.h>
#include <kernel/x64/io.h>
#include <kernel/mem/paging.h>

process_t* task_list[100];
stream *kernel_stdout, *kernel_stdin;

process_t* create_task(proc_main main)
{
    process_t* proc = new process_t;
    
    uint64_t pid = 0;
    while(task_list[pid++] != NULL);
    proc->pid = pid - 1;
    task_list[proc->pid] = proc;

    proc->exited = false;
    proc->started = false;

    return proc;
}

void yeild(process_t* proc, int exit_code)
{
    uint64_t pid = 99;
    for(;pid >= 0; pid++)
    {
        if(task_list[pid] != NULL && task_list[pid] != proc && !task_list[pid]->started)
        {
            break;
        }
    }

    proc->exit_code = exit_code;
    proc->exited = true;
    task_list[pid]->started = true;
    ((proc_main)task_list[pid]->main)(task_list[pid]);
}

void stream_write(stream* ostream, std::string* data)
{
    acquire_spinlock(&ostream->rw_lock);

    ostream->data = new std::string(*data);
    ostream->ack_update = true;

    release_spinlock(&ostream->rw_lock);
}

std::string* stream_read(stream* istream)
{
    std::string* data;

    acquire_spinlock(&istream->rw_lock);

    data = new std::string(*istream->data);
    istream->ack_update = false;

    release_spinlock(&istream->rw_lock);

    return data;
}