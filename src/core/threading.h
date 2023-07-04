#pragma once
#ifndef DESHI_THREADING_H
#define DESHI_THREADING_H

#ifndef DESHI_THREAD_PRIORITY_LAYERS
#define DESHI_THREAD_PRIORITY_LAYERS 4 
#endif

#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/ring_array.h"
StartLinkageC();

// used for thread syncronization. 
// when a thread wants to use a resource that is not guaranteed not to be changed by another thread          
// it locks it using a mutex 
// when another thread attempts to lock the same mutex it must wait until the locking thread releases 
// the lock before waking back up
// Windows: functional
// Linux: TODO
// Mac: TODO
struct mutex{
    void* handle;
    b32 is_locked;
};

// initialize a mutex and returns it
mutex mutex_init();

// deinitializes a mutex
// if the mutex is still locked, it's possible this function will fail
// and will need to be called again
void mutex_deinit(mutex* m);

// the calling thread locks the given mutex 
// if another thread has already locked it, the calling thread will wait until it is unlocked
void mutex_lock(mutex* m);

// attempts to lock the given mutex and returns true if it is sucessful
b32 mutex_try_lock(mutex* m);

// attempts to lock the given mutex for a given amount of milliseconds, returns true if it was sucessful
b32 mutex_try_lock_for(mutex* m, u64 millis);

// unlocks the given mutex
void mutex_unlock(mutex* m);

// similar to a normal mutex, but allows you to do a 'shared' lock
// along with the normal locking. When a mutex is free and is shared_lock'd
// any other thread may also call shared_lock on the same mutex and not block.
// however, if a mutex has been shared_lock'd, a thread that tries to lock it normally
// will block until all threads who have shared_lock'd it release their locks
// the primary application of this is read-write mutexes, where any number
// of threads can read some data at one time, but only one thread may access
// the data when writing. 
// !!!! A shared_mutex CANNOT be used recursively like a normal mutex can
//      eg. you cannot lock normally and then, with the same thread, try to 
//          to lock the same mutex again. There are no checks for this!
struct shared_mutex {
    void* handle;
};
typedef shared_mutex shmutex;

// initializes a shared mutex and returns it 
shared_mutex shared_mutex_init();

// deinitializes a shared mutex
// if the mutex is still locked, it's possible this function will fail
// and will need to be called again
void shared_mutex_deinit(shared_mutex* m);

// the calling thread locks the given mutex 
// if another thread has already locked the given mutex in any way, it will block until it is unlocked
void shared_mutex_lock(shared_mutex* m);

// attempts to lock the given mutex and returns true if it is sucessful
b32 shared_mutex_try_lock(shared_mutex* m);

// attempts to lock the given mutex for a given amount of milliseconds, returns true if it was sucessful
b32 shared_mutex_try_lock_for(shared_mutex* m, u64 millis);

// the calling thread locks the given mutex 
// if another thread has already locked the given mutex in any way, it will block until it is unlocked
void shared_mutex_lock_shared(shared_mutex* m);

// attempts to lock the given mutex and returns true if it is sucessful
b32 shared_mutex_try_lock_shared(shared_mutex* m);

// attempts to lock the given mutex for a given amount of milliseconds, returns true if it was sucessful
b32 shared_mutex_try_lock_for_shared(shared_mutex* m, u64 millis);

// unlocks the given mutex
void shared_mutex_unlock(shared_mutex* m);

// used for thread syncronization
// this struct allows for making threads go to sleep until notified.
// a thread calls wait() on a condition variable and doesn't wake up until 
// another thread calls notify on the same condition variable
struct condition_variable{
    void* cvhandle = 0; //win32: CONDITION_VARIABLE
    void* cshandle = 0; //win32: CRITICAL_SECTION
};
typedef condition_variable condvar;

// initializes a condition_variable
condition_variable condition_variable_init();

// deinitializes a condition_variable
void condition_variable_deinit(condition_variable* cv);

//notifies one of the threads waiting on this condition_variable
void condition_variable_notify_one(condition_variable* cv);

//notifies all threads waiting on this condition variable
void condition_variable_notify_all(condition_variable* cv);

//causes a thread to sleep on this condition_variable until some other thread calls
//one of the notify functions
void condition_variable_wait(condition_variable* cv);

//causes a thread to sleep on this condition_variable for a specified amount of time
//or until another thread calls one of the notify functions.
void condition_variable_wait_for(condition_variable* cv, u64 milliseconds);

//TODO(sushi) explain semaphores
struct semaphore{
    void* handle = 0;
    //count that should match the semaphores count but im not sure yet if it accurately does!
    u64 count;
};

// initializes a semaphore and returns it 
semaphore semaphore_init(u64 max_val);

// deinitializes a semaphore
void semaphore_deinit(semaphore* se);

void semaphore_enter(semaphore* se);
void semaphore_leave(semaphore* se);

// a job that a thread attempts to take upon waking up.
//TODO job priorities
struct ThreadJob{
    Node node;
    void (*ThreadFunction)(void*); //job function pointer
    void* data;
};

struct Thread{
    void* handle;
    b32 running = 0; //this is only set by the worker
    b32 close = 0; //this is only set by the thread manager
    u32 idx;
    str8 name; //for debugging. 
};

struct ThreadManager{
    // locked by a thread who wants to take a new job
	mutex find_job_lock;
    // prevents > max_awake_threads from running at the same time
    semaphore wake_up_barrier;
    //waited on by threads who could not find jobs to do. these threads are waken up by wake_threads
    condvar idle; 
    //count of idling threads. NOTE(sushi) this is a count of threads waiting on the idle condition variable and does not represent how many threads are sleeping overall
    u64 idle_count;
    mutex idle_count_lock; 
    ThreadJob* jobs;
    Node free_jobs;
    Thread* threads;

    //queue of threads that are waiting to be woken up 
    ring_array<condvar*> wake_up_queue;

    u32 max_threads;
    u32 max_awake_threads;
    u32 max_jobs;

    u32 awake_threads = 1; //main thread counts towards awake threads 

    ThreadJob* priorities[DESHI_THREAD_PRIORITY_LAYERS];
};

//global ThreadManager
extern ThreadManager* g_threader;
#define DeshThreader g_threader

// initializes the thread manager
// this must be done after loading memory
// the number of total threads you want available at all times
// must be specified here and cannot be changed 
// max_awake_threads is the amount of threads that the manager
// will allow to run at the same time 
void threader_init(u32 max_threads, u32 max_awake_threads, u32 max_jobs = 255);

// spawns a new thread 
void threader_spawn_thread(u32 count = 1);  

// closes all threads
void threader_close_all_threads(); 

// adds a new ThreadJob to the job ring.
void threader_add_job(ThreadJob job, u8 priority = 0);

// removes all jobs from the job ring
void threader_cancel_all_jobs(); 

// wakes up a specified amount of threads
// if count == 0 then we wake all idle threads
// NOTE(sushi) this will only wake up to (max_awake_threads - awake_threads) threads
void threader_wake_threads(u32 count = 0);

// sets a name for the calling thread
void threader_set_thread_name(str8 name);

// called by all worker threads when they finish a job
// the manager prioritizes workers that want to wake up but cant because 
// of how many threads are running
// NOTE(sushi) waking up waiting threads is handled when this thread goes to sleep when it calls going_to_sleep
static b32 threader_worker_should_continue() {
    if(DeshThreader->wake_up_queue.count) return 0;
    return 1;
}

EndLinkageC();
#endif