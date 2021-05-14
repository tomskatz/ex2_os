#include <iostream>
#include <stdlib.h>
#include <queue>
#include "uthreads.h"
#include "signal.h"
#include "Thread.h"
#include "sync_handler.h"



#define SUCCESS 0;
#define FAIL -1;
#define NON_NEGATIVE_INT 0
#define THREAD_LIBRARY_ERROR "thread library error: "
#define SYSTEM_ERROR "system error: "


 /**
  * @brief
  */
  static sync_handler _syncHandler;


/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
    if (quantum_usecs <= NON_NEGATIVE_INT)
    {
        fprintf(stderr, "%s%s\n", THREAD_LIBRARY_ERROR,
                " invalid quantum usecs, non-positive integer");
        return FAIL;
    }
    _syncHandler.init_sync_handler(quantum_usecs);
    return SUCCESS;
}

/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void)){
    if(!_syncHandler.can_add_new_thread())
    {
        return FAIL;
        //TODO: EXIT MASSAGE
    }
    return _syncHandler.create_new_thread(f);

}

/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
    Thread* currThread = _syncHandler.get_thread_by_id(tid);
    if (currThread == nullptr)
    {
        return -1; //todo add msg
    }

    if (currThread->getId() == 0 || currThread->getState() == RUNNING)
    {
        _syncHandler.release_all_resources();
        exit(0); // todo msg
        //
    }

    // TODO : check if running state need a spaical tretment
    _syncHandler.release_resources_by_thread(tid);
    // todo think about thread that locked the mutex termination.
    return SUCCESS;
}

/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
    // check if thread with this ID exists or if we are blocking the main thread
    Thread* currThread = _syncHandler.get_thread_by_id(tid);
    if (currThread == nullptr || currThread->getId() == 0)
    {
        return FAIL; //todo add msg
    }

    // if thread is in BLOCK status do nothing
    if(currThread->getState() == BLOCKED || currThread->getState() == BLOCKED_AND_BLOCKED_MUTEX)
    {
        return SUCCESS;
    }

    //change the thread to BLOCK status
    _syncHandler.changeStateToBlocked(tid);

    return SUCCESS;
}

/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
    //there is no thread with this id : error
    Thread* currThread = _syncHandler.get_thread_by_id(tid);
    if (currThread == nullptr || currThread->getId() == 0)
    {
        return FAIL; //todo add msg
        // TODO NETTA ASK TOM - should this be exit(-1)?
    }

    //resume the blocked thread if it is not running or ready status
    if(currThread->getState() != RUNNING || currThread->getState() != READY)
    {
        _syncHandler.resumeThread(tid);
    }

    return SUCCESS;
}

/*
 * Description: This function tries to acquire a mutex.
 * If the mutex is unlocked, it locks it and returns.
 * If the mutex is already locked by different thread, the thread moves to BLOCK state.
 * In the future when this thread will be back to RUNNING state,
 * it will try again to acquire the mutex.
 * If the mutex is already locked by this thread, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_lock()
{
    if (_syncHandler.get_mutex_thread_id() == _syncHandler.get_running_thread_id())
    {
        // TODO print error + realse resources?
        exit(-1);
    }
    return _syncHandler.lock_mutex();
}


/*
 * Description: This function releases a mutex.
 * If there are blocked threads waiting for this mutex,
 * one of them (no matter which one) moves to READY state.
 * If the mutex is already unlocked, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_unlock()
{
    if (_syncHandler.get_mutex_thread_id() == -1) // tODO is this enough?
    {
        // TODO print error + realse resources?
        exit(-1);
    }
    return _syncHandler.unlock_mutex();

}

/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
    //todo : calling thread can be only the running one ?
    return _syncHandler.get_running_thread_id();
}

/*
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
    return _syncHandler.get_total_quantums();
}

/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
    //if no thread with ID tid it's an error
    Thread* currThread = _syncHandler.get_thread_by_id(tid);
    if (currThread == nullptr || currThread->getId() == 0)
    {
        return FAIL; //todo add msg
    }

    return _syncHandler.get_quantums_by_id(tid);
}

