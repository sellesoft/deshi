#pragma once
#ifndef DESHI_THREADING_H
#define DESHI_THREADING_H

#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/unicode.h"
#include "kigu/ring_array.h"



//used for thread syncronization.
// when a thread is to use a resource that is not atmoic it locks it using a mutex
// when another thread attempts to lock the same mutex it must wait until the locking thread
// releases the lock before waking back up
struct mutex{
    void* handle = 0;
    b32 is_locked = 0;
    mutex();
    ~mutex();
    // locks this mutex from the calling thread 
    void lock(); 
    // attempts to lock this mutex from the calling thread, if it fails the function returns false
    b32  try_lock();
    // attempts to lock this mutex from the calling thread for a specified amount of time (milliseconds)
    // if it fails the function returns false
    b32  try_lock_for(u64 milliseconds);
    // unlocks this mutex
    void unlock();
};

//used for thread syncronization
// this struct allows for making threads go to sleep until notified.
// a thread calls wait() on a condition variable and doesn't wake up until 
// another thread calls notify on the same condition variable
struct condition_variable{
    void* cvhandle = 0; //win32: CONDITION_VARIABLE
    void* cshandle = 0; //win32: CRITICAL_SECTION
	
    void init();
    void deinit();
    //notifies one of the threads waiting on this condition_variable
    void notify_one();
    //notifies all threads waiting on this condition variable
    void notify_all();
    //causes a thread to sleep on this condition_variable until some other thread calls
    //one of the notify functions
    void wait();
    //causes a thread to sleep on this condition_variable for a specified amount of time
    //or until another thread calls one of the notify functions.
    void wait_for(u64 milliseconds);
};
typedef condition_variable condvar;

struct scopedlock{//locks a mutex and unlocks it when it goes out of scope
    mutex* m = 0;
    scopedlock(mutex& _m){DPZoneScoped;
        m=&_m;
        m->lock();
    }
    ~scopedlock(){DPZoneScoped;
        m->unlock();
    }
};

struct semaphore{
    void* handle = 0;

    //count that should match the semaphores count but im not sure yet if it accurately does!
    u64 count;

    void init(u64 initial_val, u64 max_val);
    void deinit();

    void enter();
    void leave();
};

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
    string name = ""; //for debugging. 
    u32 idx;
};

struct ThreadManager{
	DPTracyLockable(mutex, job_ring_lock);
    DPTracyLockable(mutex, hypnagogia_lock); //locked when a thread is going to sleep or waking up

    semaphore wake_up_barrier;
    condvar idle; //waited on by threads who could not find jobs to do. these threads are waken up by wake_threads
    u64 idle_count; //count of idling threads. NOTE(sushi) this is a count of threads waiting on the idle condition variable and does not represent how many threads are sleeping overall
    mutex idle_count_lock; 
    ring_array<ThreadJob> job_ring; 
    array<Thread*> threads; //TODO arena threads instead of using memalloc

    //queue of threads that are waiting to be woken up 
    ring_array<condvar*> wake_up_queue;

    u32 max_threads = 0; // a value of 0 indiciates no limit to created threads
    u32 max_awake_threads = 8; // how many threads are allowed to be awake at one time

    u32 awake_threads = 1; //main thread counts towards awake threads 

    //initializes the thread manager
    //this must be done after loading memory
    void init(u32 max_jobs = 255);

    //spawns a new thread 
    void spawn_thread(u32 count = 1); 
    //closes all threads
    void close_all_threads(); 

    //adds a new ThreadJob to the job ring.
    void add_job(ThreadJob job); 
    //adds a collection of jobs to the job ring.
    void add_jobs(carray<ThreadJob> jobs); 
    //removes all jobs from the job ring
    void cancel_all_jobs(); 

    //wakes up a specified amount of threads
    //if count == 0 then we wake all idle threads
    //NOTE(sushi) this will only wake up to (max_awake_threads - awake_threads) threads
    void wake_threads(u32 count = 0);

    //called when a thread attempts to wake up from a condvar
    //if there is no room for the thread to start running immediatly,
    //it must go back to sleep and wait for the manager to wake it up later
    void waking_up(){
        hypnagogia_lock.lock();
        if(awake_threads == max_awake_threads){
            condvar cv;
            cv.init();
            wake_up_queue.add(&cv);
            hypnagogia_lock.unlock();
            cv.wait();
        }else{
            awake_threads++;
        }
        hypnagogia_lock.unlock();
    }

    //called when a thread is going to sleep
    //this dispatches threads that wanted to wake up but couldnt due to how many threads were running
    //if there are no threads waiting in queue, but there are still jobs we dispatch another thread to 
    //take its place
    //TODO(sushi) this is deep default/implicit behavoir that should not be implemented when i rewrite this
    //            instead this should probably run based on a var for telling manager exactly how many threads you
    //            want to keep automatically running at any time
    void going_to_sleep(){
        hypnagogia_lock.lock();
        if(wake_up_queue.count){
            wake_up_queue[0]->notify_all();
            wake_up_queue.remove(1);
        }else if(job_ring.count){
            idle.notify_one();
        }else{
            awake_threads--;
        }
        if(!awake_threads){
            LogE("threader", "the last awake thread is going to sleep.");
        }
        hypnagogia_lock.unlock();
    }

    //called by all worker threads when they finish a job
    //the manager prioritizes workers that want to wake up but cant because 
    //of how many threads are running
    //NOTE(sushi) waking up waiting threads is handled when this thread goes to sleep when it calls going_to_sleep
    b32 worker_should_continue() {
        if(wake_up_queue.count) return 0;
        return 1;
    }

    //sets a name for the thread
    //this is useful to give a thread a name that indicates what its doing, so in debugging
    //you dont have to examine the call stack to figure it out
    //this sets the name of the thread that calls it
    void set_thread_name(str8 name);
};


//global ThreadManager
extern ThreadManager* g_tmanager;
#define DeshThreadManager g_tmanager

#endif