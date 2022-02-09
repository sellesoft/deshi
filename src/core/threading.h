#pragma once
#ifndef DESHI_THREADING_H
#define DESHI_THREADING_H

#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/unicode.h"

const u32 max_threads = 3;

struct mutex{
    void* handle = 0;
    b32 is_locked = 0;
    mutex();
    ~mutex();
    void lock();
    b32  try_lock();
    b32  try_lock_for(u64 milliseconds);
    void unlock();
};

struct condition_variable{
    void* cvhandle = 0; //win32: CONDITION_VARIABLE
    void* cshandle = 0; //win32: CRITICAL_SECTION
	
    condition_variable();
    ~condition_variable();
    void notify_one();
    void notify_all();
    void wait();
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

enum ThreadState{
    ThreadState_NotInitialized,
    ThreadState_Initializing,
	ThreadState_Close,
	ThreadState_Sleep,
	ThreadState_CallFunction,
};

//TODO rename this to persistent thread and make a Thread that starts running the function immediately and closes when finished
struct Thread{
    void* handle = 0;
    void* tuple_handle = 0; //when using win32, we must allocate a tuple to pass as arguments to the function
    ThreadState state;
    string comment = "";
	
    condvar calling_thread_cv;
    condvar thread_cv;
    mutex state_mutex;
	
    template<typename FuncToRun, typename... FuncArgs>
		void threadfunc(FuncToRun f, FuncArgs... args);
	
    //sets the function that the thread calls 
    //TODO maybe theres a way around closing and reopening the thread? probably not with templating
	template<typename FuncToRun, typename... FuncArgs>
		void SetFunction(FuncToRun f, FuncArgs...args);
	
    //sets the function that the thread calls and waits until its done initializing
	template<typename FuncToRun, typename... FuncArgs>
		void SetFunctionAndWait(FuncToRun f, FuncArgs...args);
	
	void WakeUp();
	
	void ChangeState(ThreadState ts);
	
	//this funciton locks the caller mutex and waits until CallingThreadCondition receieves
	//a signal to unblock
	b32 Wait(u64 timeout = 0);
	
    //attempts to run the threads function immediately, but returns early if it can't
	void Run(int count = 1);
	
    //waits until the the thread finishes executing before telling it to run again
    void WaitToRun(int count = 1);
	
	//pauses the calling thread until this one has finished executing
	void RunAndWait(int count = 1);
	
    void WaitToRunAndWait(int count = 1);
	
    //causes the thread to return
	void Close();
	
	//waits for the thread to return, this means you must know the thread is active to begin with!
	void CloseAndJoin();
	
    void SetName(str16 name);
	
	~Thread();
	
	
};

struct ThreadManager{
    set<Thread*> threads;
    Arena*       thread_arena;
	condvar      waitcv;
	mutex        waitm;
	mutex        return_callback_lock;
	Thread*      last_returned = 0;
	b32          waiting_for_thread = 0;
	
    Thread* MakeNewThread(const string& comment = "");
    void    StopAndDeleteThread(Thread* thread);
    void    StopAndDeleteThreadAndWait(Thread* thread);
    void    StopAndDeleteAllThreads();
    void    StopAndDeleteAllThreadsAndWait();
    void    DeleteThread(Thread* thread);
	void    CloseAllThreads();
    void    WaitForAllThreadsToFinish(u64 timeout = 0);
	Thread* GetNextAvaliableThread();
	Thread* WaitForFirstAvaliableThread();
    ~ThreadManager();
};

//global ThreadManager
extern ThreadManager* g_tmanager;
#define DeshThreadManager g_tmanager

#endif