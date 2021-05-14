#include <iostream>
#include <signal.h>
#include "Thread.h"

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}
#else
#endif

//TODO: CHRCK IF THERE IS A NEED TO MAKE A DIFF BETWEEN THREAD[0] TO THE REST
Thread::Thread(int id, void (*f)(void))
{
    _id = id;
    _state = READY;
    _quantumCount = 0;
    _stack = new char[STACK_SIZE];

    address_t sp, pc;
    sp = (address_t)_stack + STACK_SIZE - sizeof(address_t);
    pc = (address_t) f;
    sigsetjmp(_env, 1);
    (_env->__jmpbuf)[JB_SP] = translate_address(sp);
    (_env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&_env->__saved_mask);
}

Thread::~Thread()
{
    delete[] _stack;
}

int Thread::getId() const
{
    return _id;
}

void Thread::setState(int state)
{
    _state = state;
}

int Thread::getState() const
{
    return _state;
}

void Thread::increaseQuantumCount()
{
    _quantumCount++;
}

int Thread::getQuantumCount() const
{
    return _quantumCount;
}

__jmp_buf_tag* Thread::getEnv()
{
    return _env; // TODO check this works with int*
}