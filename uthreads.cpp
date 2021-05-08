//
// Created by Tom Katz on 07/05/2021.
//
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