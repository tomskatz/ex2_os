#include <iostream>
#include <stdlib.h>
#include "sync_handler.h"


/**
 * set the masking set and check system calls
 * @return
 */
void sync_handler::init_maskedSignals()
{
    if (sigemptyset(&_maskedSignals) < 0)
    {
        fprintf(stderr, "%s%s/n", SYSTEM_ERROR, "sigemptyset failed to clear the set");
        exit(1); //TODO : CHECK CLEARING THE RECOURSES
    }
    if (sigaddset(&_maskedSignals, SIGVTALRM) < 0)
    {
        fprintf(stderr, "%s%s/n", SYSTEM_ERROR, "sigaddset failed to add signal to the set");
        exit(1); //TODO : CHECK CLEARING THE RECOURSES
    }
}

void sync_handler::block_maskedSignals()
{
    if (sigprocmask(SIG_BLOCK, &_maskedSignals, NULL) < 0)
    {
        exit(-1);
        //TODO to fix + add error
    }
}

void sync_handler::unblock_maskedSignals()
{
    if (sigprocmask(SIG_UNBLOCK, &_maskedSignals, NULL) < 0)
    {
        exit(-1);
        //TODO to fix + add error
    }
}

Thread* sync_handler::create_main_thread()
{
    Thread* thread = new(std::nothrow) Thread(0, nullptr);
    if (thread == nullptr)
    {
        exit(-1);
        //TODO to fix + add error (alloc fail)
    }
    thread->setState(RUNNING);
    thread->increaseQuantumCount();
    _allThreads[0] = thread;
    return thread;
}

int sync_handler::create_new_thread(void (*f)(void))
{
    int id = _nextAvailableID.top();
    Thread* thread = new(std::nothrow) Thread(id, f);
    if (thread == nullptr)
    {
        return FAIL;
        //TODO to fix + add error (alloc fail)
    }
    _nextAvailableID.pop();
    thread->setState(READY);
    _readyThreads.push_back(id);
    _allThreads[id] = thread;
    return id;
}

/**
 * @brief
 */
void sync_handler::init_sync_handler(int quantum_usecs)
{
    init_maskedSignals();

    for (int i = 1; i < MAX_THREAD_NUM; ++i)
    {
        _nextAvailableID.push(i);
    }

    _quantumSecs = quantum_usecs;
    _totalQuantumCount = 1;
    _runningThread = create_main_thread();

    init_timer();
    set_timer();
}

void sync_handler::sigvtalrm_handler(int)
{
    block_maskedSignals();
    changeStateToReady();

    int ret_val = sigsetjmp(_runningThread->getEnv(), 1);
    if (ret_val == 0)
    {
        changeStateToRunning();
    }
    // TODO check if need to clear resources
    unblock_maskedSignals();
}

void sync_handler::changeStateToReady()
{
    _runningThread->setState(READY);
    _readyThreads.push_back(_runningThread->getId());
}

void sync_handler::changeStateToRunning()
{
    _totalQuantumCount++;
    _runningThread = _allThreads[_readyThreads.front()];
    _runningThread->setState(RUNNING);
    _runningThread->increaseQuantumCount();
    _readyThreads.pop_front();

    set_timer();
    siglongjmp(_runningThread->getEnv(), RETURN_VALUE_FROM_JMP);
}


void sync_handler::init_timer()
{
    _sa.sa_handler = &sigvtalrm_handler;

    if (sigaction(SIGVTALRM, &_sa, NULL) < 0)
    {
        exit(-1);
        //TODO write message. + maybe terminate
    }
}

void sync_handler::set_timer()
{
    _timer.it_value.tv_sec = _quantumSecs / MICRO_SECONDS;
    _timer.it_value.tv_usec = _quantumSecs % MICRO_SECONDS;

    _timer.it_interval.tv_sec = 0;
    _timer.it_interval.tv_usec = 0;

    if (setitimer (ITIMER_VIRTUAL, &_timer, NULL)) {
        printf("setitimer error."); // TODO change
    }
}

bool sync_handler::can_add_new_thread()
{
    return (_allThreads.size() < MAX_THREAD_NUM);
}

Thread* sync_handler::get_thread_by_id(int id)
{
    auto thread = _allThreads.find(id);
    if(thread == _allThreads.end())
    {
        return nullptr;
    }
    return thread->second;
}



void sync_handler::release_resources_by_thread(int id)
{
    block_maskedSignals();
    Thread* threadToTerminate = _allThreads[id];
    if (threadToTerminate->getState() == BLOCKED)
    {
        _blockedThreads.erase(id);
    }
    if (threadToTerminate->getState() == READY)
    {
        _readyThreads.erase(std::remove(_readyThreads.begin(), _readyThreads.end(), id),
                            _readyThreads.end());
    }
    delete(threadToTerminate);
    _allThreads.erase(id);
    _nextAvailableID.push(id);
    unblock_maskedSignals();
}

void sync_handler::release_all_resources()
{
    block_maskedSignals();
    for (auto th : _allThreads)
    {
        delete(th.second);
    }
    _readyThreads.clear();
    _blockedThreads.clear();
    // todo check if need to delete priority queue
    unblock_maskedSignals();
}

