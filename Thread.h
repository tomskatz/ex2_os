
#include <setjmp.h>
#include "uthreads.h"

#ifndef EX2_OS_THREAD_H
#define EX2_OS_THREAD_H

#define RUNNING 0
#define READY 1
#define BLOCKED 2
#define BLOCKED_MUTEX 3
#define BLOCKED_AND_BLOCKED_MUTEX 4

class Thread

{
private:
    int _id;
    int _state;
    char* _stack;
    sigjmp_buf _env;
    int _quantumCount;

public:
    Thread(int id, void (*f)(void)); // TODO check if need distractor

    ~Thread();

    int getId() const;

    void setState(int new_state);

    int getState() const; // TODO go over all places where threads differ by state change
    // according to BLOCKED_MUTEX & BLOCKED_AND_BLOCKED_MUTEX

    void increaseQuantumCount();

    int getQuantumCount() const;

    __jmp_buf_tag* getEnv();
};


#endif //EX2_OS_THREAD_H
