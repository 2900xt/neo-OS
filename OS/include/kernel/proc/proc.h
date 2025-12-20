#pragma once

#include <types.h>
#include <kernel/proc/stream.h>
#include <kernel/vfs/file.h>

namespace kernel
{
    struct process_t
    {
        uint64_t pid;

        uint64_t stack_frame;
        void *main;

        stream *stdin, *stdout;

        kernel::file_handle **files;

        bool started;
        bool exited;
        int exit_code;
    };

    typedef void (*proc_main)(process_t *);

    extern process_t *task_list[100];

    extern void task_init(proc_main kernel_main);
    extern process_t *create_task(proc_main main);
    extern void yeild(process_t *proc);
}