#include <signal.h>
#include <queue>
#include <unordered_map>
#include "uthreads.h"
#include "Thread.h"
#include <sys/time.h>
#include <setjmp.h>



#ifndef EX2_OS_SYNC_HANDLER_H
#define EX2_OS_SYNC_HANDLER_H
#define SUCCESS 0;
#define FAIL -1;
#define THREAD_LIBRARY_ERROR "thread library error: "
#define SYSTEM_ERROR "system error: "
#define MICRO_SECONDS 1000000
#define RESET_TIMER 0
#define RETURN_VALUE_FROM_JMP 1

class sync_handler
{
private:
    /**
     * counter for all the quantoms in the proess
     */
    static int _totalQuantumCount;

    /**
     *A pointer to the current running thread.
     */
    static Thread* _runningThread;

    /**
     * A list of threads in 'READY' status
     */
    static std::deque<int> _readyThreads;

    /**
     * A mapping between threadID and the thread pointer
     */
    static std::unordered_map<int, Thread*> _allThreads;

    /**
    * A mapping between threadID and the thread pointer
    */
    static std::unordered_map<int, Thread*> _blockedThreads;

    /**
    * A set containing the signals to be blocked
    */
    static sigset_t _maskedSignals;

    /**
     * A priority queue that keeps the next smallest available Id.
     */
    static std::priority_queue<u_int, std::vector<u_int>, std::greater<u_int>> _nextAvailableID;

    /**
     * Sigaction struct - to define handlers.
     * */
    static struct sigaction _sa;

    static struct itimerval _timer;

    /**
     * The size of a quantum in ms (as recieved in the init)
     */
    static int _quantumSecs;

    /**
     * set the masking set and check system calls
     * @return
     */
    static void init_maskedSignals();

    static void init_timer();

    static void set_interval_timer();

    static void set_timer();

    static void reset_timer();

    static void sigvtalrm_handler(int);

    static void changeStateToReady(int id);

    static void changeStateToRunning();

    static void block_maskedSignals();

    static void unblock_maskedSignals();

    static Thread* create_main_thread();

public:

    static int create_new_thread(void (*f)(void));

    static void init_sync_handler(int quantum_usecs);

    static bool can_add_new_thread();

    static Thread* get_thread_by_id(int id);

    void release_resources_by_thread(int id);

    void release_all_resources();

    static void changeStateToBlocked(int id);

    static void resumeThread(int id);

    static int get_running_thread_id();

    static int get_total_quantums();

    static int get_quantums_by_id(int id);

};


#endif //EX2_OS_SYNC_HANDLER_H
