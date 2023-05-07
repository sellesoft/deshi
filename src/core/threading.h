#pragma once
#ifndef DESHI_THREADING_H
#define DESHI_THREADING_H

#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/map.h"
#include "kigu/string.h"
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
    void* handle = 0;
    b32 is_locked = 0;
};

// initialize a mutex and returns it
mutex mutex_init();

// deinitializes a mutex
// this does NOT unlock the mutex!
void mutex_deinit(mutex* m);

// the calling thread locks the given mutex 
// if another thread has already locked it, the calling thread will wait until it is unlocked
void mutex_lock(mutex* m);

// attempts to lock the given mutex and returns true if it is sucessful
b32 mutex_try_lock(mutex* m);

// attempts to lock the given mutex for a given amount of milliseconds, returns true if it was sucessful
b32 mutex_try_lock_for(u64 millis);

// unlocks the given mutex
void mutex_unlock(mutex* m);

// sed for thread syncronization
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
semaphore semaphore_init(u64 initial_val, u64 max_val);

// deinitializes a semaphore
void semaphore_deinit(semaphore* se);

void semaphore_enter(semaphore* se);
void semaphore_leave(semaphore* se);

// a job that a thread attempts to take upon waking up.
//TODO job priorities
struct ThreadJob{
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
	mutex job_ring_lock;
    semaphore wake_up_barrier;
    condvar idle; //waited on by threads who could not find jobs to do. these threads are waken up by wake_threads
    u64 idle_count; //count of idling threads. NOTE(sushi) this is a count of threads waiting on the idle condition variable and does not represent how many threads are sleeping overall
    mutex idle_count_lock; 
    ring_array<ThreadJob> job_ring; 
 arrayT<Thread*> threads; //TODO arena threads instead of using memalloc

    //queue of threads that are waiting to be woken up 
    ring_array<condvar*> wake_up_queue;

    u32 max_threads = 0; // a value of 0 indiciates no limit to created threads
    u32 max_awake_threads = 8; // how many threads are allowed to be awake at one time

    u32 awake_threads = 1; //main thread counts towards awake threads 
};

//global ThreadManager
extern ThreadManager* g_tmanager;
#define DeshThreadManager g_tmanager

// initializes the thread manager
// this must be done after loading memory
void threader_init(u32 max_jobs = 255);

// spawns a new thread 
void threader_spawn_thread(u32 count = 1);  

// closes all threads
void threader_close_all_threads(); 

// adds a new ThreadJob to the job ring.
void threader_add_job(ThreadJob job); 

// adds a collection of jobs to the job ring.
void threader_add_jobs(carray<ThreadJob> jobs); 

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
    if(DeshThreadManager->wake_up_queue.count) return 0;
    return 1;
}

EndLinkageC();
#endif