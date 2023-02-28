#include "kernel/x64/paging.h"
#include "stdlib/assert.h"
#include "types.h"
#include <kernel/proc.h>
#include <kernel/mem.h>
#include <stdlib/stdlib.h>

Process** pid_list;
pid_t       next_pid;

pid_t pid_generator(Process* proc)
{
    if(pid_list)
    {
        pid_list[next_pid] = proc;
        return next_pid++;
    }
    pid_list = (Process**)kcalloc(2048, sizeof(uint64_t));
    return pid_generator(proc);
}

Process* pid_hash(pid_t pid)
{
    assert(pid < next_pid);
    return pid_list[pid];
}

Process* Process::create_child(const char *child_pname, void (*code)(pid_t, int, char**))
{
    Process* child = (Process*)kcalloc(1, sizeof(Process));
    child->pname = child_pname;
    child->youngest_child = NULL;
    child->state = EMBRYO;
    child->parent = this;
    child->older_sibling = NULL;

    if(this->youngest_child == NULL)
    {
        this->youngest_child = child;
        return child;
    }

    child->older_sibling = this->youngest_child;
    child->older_sibling->younger_sibling = child;
    this->youngest_child = child;

    child->pid = pid_generator(child);
    child->code_ptr = code;
    child->stack_ptr = 0;
    child->stack_base = (uint64_t*)AMD64::next_page();

    return child;
}


int process_run(Process* proc, int argc, char** argv)
{
    if(proc->state == EMBRYO)
    {
        proc->state = RUNNING;
        proc->code_ptr(proc->pid, argc, argv);
        while(proc->state != READY);
        return proc->exit_code;
    }

    else return 127;
}

void process_exit(pid_t pid, int code)
{
    Process* proc = pid_hash(pid);
    proc->exit_code = code;
    proc->state = READY;
}

inline uint64_t get_stack_ptr(void)
{
    uint64_t rsp;
    asm("mov %%rsp, %0" : "=r"(rsp));
    return rsp;
}

extern "C" void save_regs(void);

void process_suspend(Process* proc)
{
    if(proc->state != RUNNING)
    {
        return;
    }

    save_regs();
    proc->stack_ptr = get_stack_ptr();
    proc->state = WAITING;
}