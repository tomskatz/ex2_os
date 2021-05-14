#include <signal.h>
#include <queue>
#include <unordered_map>
#include "uthreads.h"
#include "Thread.h"
#include <sys/time.h>
#include <setjmp.h>

#ifndef EX2_OS_SYNC_HANDLER_H
#define EX2_OS_SYNC_HANDLER_H
#define SUCCESS 0
#define FAIL -1
#define UNLOCKED -1
#define THREAD_LIBRARY_ERROR "thread library error: "
#define SYSTEM_ERROR "system error: "
#define MICRO_SECONDS 1000000
#define RESET_TIMER 0
#define RETURN_VALUE_FROM_JMP 1
#define UNLOCK_FAIL_MSG "Unlocking the mutex failed."
#define LOCK_FAIL_MSG "Locking the mutex failed."
#define SETITIMER_ERR_MSG "setitimer error."
#define SIGACTION_ERR_MSG "sigaction error."
#define SIGADDSET_FAIL_MSG "sigaddset failed to add signal to the set."
#define SIGEMPTYSET_FAIL_MSG "sigemptyset failed to clear the set."
#define SIGPROCMASK_BLOCK_FAIL_MSG "sigprocmask failed to block the set."
#define SIGPROCMASK_UNBLOCK_FAIL_MSG "sigprocmask failed to unblock the set."

#define CREATE_THREAD_FAIL_MSG "Allocating a new thread failed."

#define INIT_MUTEX_ERR "Initializing the mutex failed."




class sync_handler
{
private:
    /**
     * counter for all the quantoms in the process
     */
    static int _totalQuantumCount;

    /**
     *A pointer to the current running thread.
     */
    static Thread* _runningThread;

    /**
     * the id of the thread that is currently locking the mutex. Will be -1 when unlocked
     */
    static int _mutexThreadId;

    /**
     * A queue of threads in 'READY' status
     */
    static std::deque<int> _readyThreads;

    /**
     * A mapping between threadID and the thread pointer - for all threads.
     */
    static std::unordered_map<int, Thread*> _allThreads;

    /**
    * A mapping between threadID and the thread pointer - for the blocked threads.
    */
    static std::unordered_map<int, Thread*> _blockedThreads;

    /**
     * A queue of threads in 'MUTEX_BLOCKED' status
     */
    static std::deque<int> _mutexBlockedThreads;

    /**
    * A set containing the signals to be blocked
    */
    static sigset_t _maskedSignals;

    /**
     * A priority queue (min heap) that keeps the next smallest available Id.
     */
    static std::priority_queue<u_int, std::vector<u_int>, std::greater<u_int>> _nextAvailableID;

    /**
     * Sigaction struct - to define handlers.
     * */
    static struct sigaction _sa;

    /**
     * The timer used.
     * */
    static struct itimerval _timer;

    /**
     * The size of a quantum in ms (as received in the init method).
     */
    static int _quantumSecs;

    static pthread_mutex_t _mutex;

    /**
     * set the masking set and check system calls
     * @return
     */
    static void init_maskedSignals();

    static void init_timer();

    static void set_interval_timer();

    static void set_timer();

    static void reset_timer();

    static void init_mutex();

    static void sigvtalrm_handler(int);

    static void changeStateToReady(int id);

    static void changeStateToRunning();

    static void block_maskedSignals();

    static void unblock_maskedSignals();

    static Thread* create_main_thread();

    static void remove_from_readyThreads(Thread* threadToRemove);

public:

    static int create_new_thread(void (*f)(void));

    static void init_sync_handler(int quantum_usecs);

    static bool can_add_new_thread();

    static Thread* get_thread_by_id(int id);

    static void release_resources_by_thread(int id);

    static void release_all_resources();

    static void changeStateToBlocked(int id);

    static void resumeThread(int id);

    static int get_running_thread_id();

    static int get_mutex_thread_id();

    static int get_total_quantums();

    static int get_quantums_by_id(int id);

    static int lock_mutex();

    static int unlock_mutex();

    static void exit_and_print_error(std::string msg);

    static int return_and_print_error(std::string msg);

};


#endif //EX2_OS_SYNC_HANDLER_H
