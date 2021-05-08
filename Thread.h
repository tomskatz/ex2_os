
#include <setjmp.h>
#include "uthreads.h"

#ifndef EX2_OS_THREAD_H
#define EX2_OS_THREAD_H

#define RUNNING 0
#define READY 1
#define BLOCKED 2

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

    int getState() const;

    void increaseQuantumCount();

    int getQuantumCount() const;

    int* getEnv();
};


#endif //EX2_OS_THREAD_H
