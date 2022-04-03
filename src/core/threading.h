#pragma once
#ifndef DESHI_THREADING_H
#define DESHI_THREADING_H

#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/unicode.h"
#include "kigu/ring_array.h"

const u32 max_threads = 3;

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
	
    condition_variable();
    ~condition_variable();
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
    scopedlock(mutex& _m){
        m=&_m;
        m->lock();
    }
    ~scopedlock(){
        m->unlock();
    }
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
};

struct ThreadManager{
	mutex job_ring_lock;
    condvar idle; //waited on by threads who could not find jobs to do. these threads are waken up by wake_threads
    ring_array<ThreadJob> job_ring; 
    array<Thread*> threads; //TODO arena threads instead of using memalloc

    //initializes the thread manager
    //this must be done after loading memory
    void init(u32 max_jobs = 255);

    //spawns a new thread 
    void spawn_thread(); 
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
    void wake_threads(u32 count = 0);
};

//global ThreadManager
extern ThreadManager* g_tmanager;
#define DeshThreadManager g_tmanager

#endif