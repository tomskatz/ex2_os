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

void sync_handler::exit_and_print_error(std::string prefix, std::string msg)
{
    fprintf(stderr, "%s%s/n", prefix.c_str(), msg.c_str());
    release_resources_by_thread(_runningThread->getId());
    release_all_resources();
    exit(FAIL);
    // TODO finish NETTA
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

    init_mutex();
    init_timer();
    set_timer();
}

void sync_handler::sigvtalrm_handler(int)
{
    block_maskedSignals();
    changeStateToReady(_runningThread->getId());

    int ret_val = sigsetjmp(_runningThread->getEnv(), 1);
    if (ret_val == 0)
    {
        changeStateToRunning();
    }
    // TODO check if need to clear resources
    unblock_maskedSignals();
}

void sync_handler::changeStateToReady(int id)
{
    Thread* threadToReady = _allThreads[id];
    threadToReady->setState(READY);
    _readyThreads.push_back(threadToReady->getId());
    //TODO: WE NEED TO CALL THE NEXT THREAD IN THE Q TO RUN ?
}

void sync_handler::changeStateToRunning() // TODO CHANGE THIS METHOD NAME
{
    _totalQuantumCount++;
    //TODO: THINK IF WE NEED TO CHECK FIRST THAT THE _readyThreads is not empty
    _runningThread = _allThreads[_readyThreads.front()];
    _runningThread->setState(RUNNING);
    _runningThread->increaseQuantumCount();
    _readyThreads.pop_front();

    set_timer();
    siglongjmp(_runningThread->getEnv(), RETURN_VALUE_FROM_JMP);
}

void sync_handler::changeStateToBlocked(int id)
{
    block_maskedSignals();
    Thread* threadToBlock = _allThreads[id];

    if (threadToBlock->getState() == RUNNING)
    {
        reset_timer();
        //If a thread blocks itself, a scheduling decision should be made ?
        int ret_val = sigsetjmp(_runningThread->getEnv(), 1);
        if (ret_val == 0)
        {
            //preempt the next ready thread to RUNNING
            changeStateToRunning();
        }
    }
    //TODO: DO WE NEED TO CHECK IF THERE ARE RESOURCES TO DELETE?

    if (threadToBlock->getState() == READY)
    {
        //remove thread from the ready queue
        _readyThreads.erase(std::remove(_readyThreads.begin(), _readyThreads.end(), id),
                            _readyThreads.end());
    }

    int newState = (threadToBlock->getState() == BLOCKED_MUTEX) ? BLOCKED_AND_BLOCKED_MUTEX :
            BLOCKED;

    //change the state and add to the blocked thread map
    threadToBlock->setState(newState);
    _blockedThreads[id] = threadToBlock;

    unblock_maskedSignals();
}

void sync_handler::resumeThread(int id)
{
    block_maskedSignals();
    //remove from the blocked list
    _blockedThreads.erase(id);
    if (_allThreads[id]->getState() == BLOCKED_AND_BLOCKED_MUTEX)
    {
        _allThreads[id]->setState(BLOCKED_MUTEX);
    }
    else
    {
        changeStateToReady(id);
    }
    unblock_maskedSignals();
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

void sync_handler::init_mutex()
{
    _mutex = PTHREAD_MUTEX_INITIALIZER;
    _mutexThreadId = -1;
    if (pthread_mutex_init(&_mutex, nullptr) != 0)
    {
        _mutex = nullptr;
        // TODO check if we want to terminate.
        //TODO write message
        exit(-1);
    }
}

void sync_handler::set_interval_timer()
{
    if (setitimer (ITIMER_VIRTUAL, &_timer, NULL)) {
        printf("setitimer error."); // TODO change
        exit(-1);
    }
}

void sync_handler::set_timer()
{
    _timer.it_value.tv_sec = _quantumSecs / MICRO_SECONDS;
    _timer.it_value.tv_usec = _quantumSecs % MICRO_SECONDS;

    _timer.it_interval.tv_sec = RESET_TIMER;
    _timer.it_interval.tv_usec = RESET_TIMER;

    set_interval_timer();
}

void sync_handler::reset_timer()
{
    _timer.it_value.tv_sec = RESET_TIMER;
    _timer.it_value.tv_usec = RESET_TIMER;

    _timer.it_interval.tv_sec = RESET_TIMER;
    _timer.it_interval.tv_usec = RESET_TIMER;

    set_interval_timer();
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
    if (threadToTerminate->getState() == BLOCKED ||
    threadToTerminate->getState() == BLOCKED_AND_BLOCKED_MUTEX)
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

int sync_handler::get_running_thread_id()
{
    return _runningThread->getId();
}

int sync_handler::get_mutex_thread_id()
{
    return _mutexThreadId;
}

int sync_handler::get_total_quantums()
{
    return _totalQuantumCount;
}

int sync_handler::get_quantums_by_id(int id)
{
    return _allThreads[id]->getQuantumCount();
}

int sync_handler::lock_mutex()
{
    // sigset will save the current location to return to in case the lock fails.
    sigsetjmp(_runningThread->getEnv(), 1);
    block_maskedSignals();
    if (_mutexThreadId != -1)
    {
        _runningThread->setState(BLOCKED_MUTEX);
        _mutexBlockedThreads.push_back(_runningThread->getId());
        changeStateToRunning(); // puts a new thread in running
        unblock_maskedSignals();
        return -1; // TODO CEHCK THIS - what should we return?
    }

    if (pthread_mutex_lock(&_mutex) != SUCCESS)
    {
        // TODO print error + realse resources?
        exit(-1);
    }
    _mutexThreadId = get_running_thread_id();
    unblock_maskedSignals();
    return SUCCESS;
}

int sync_handler::unlock_mutex()
{
    block_maskedSignals();
    if (pthread_mutex_unlock(&_mutex) != SUCCESS){
        // TODO print error + realse resources?
        exit(-1);
    }
    _mutexThreadId = -1;

    // Searching for the first thread that isn't BLOCKED, and changing its state to READY.
    int i = 0;
    for(auto threadId : _mutexBlockedThreads)
    {
        Thread* nextThread = _allThreads[threadId];
        if (nextThread->getState() == BLOCKED_MUTEX)
        {
            changeStateToReady(nextThread->getId());
            unblock_maskedSignals();
            return SUCCESS;
        }
        i++;
    }
    // When reaching this part, all threads are BLOCKED_AND_BLOCKED_MUTEX
    // We will take the first thread and change it's state to blocked (removing the mutex block).
    Thread* nextThread = _allThreads[_mutexBlockedThreads.front()];
    _mutexBlockedThreads.pop_front();
    nextThread->setState(BLOCKED);
    return SUCCESS;
}