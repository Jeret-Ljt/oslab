// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    for (num = 0; num < 16; num++) {
	    printf("*** thread %d looped %d times\n", currentThread->get_thread_id(), num);
        interrupt->OneTick();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2
//  Set up serveral threads, by forking 
//  to call SimpleThread, and then calling SimpleThread ourselves.
//  to test the threads number limitation.
//----------------------------------------------------------------------

void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");
    for (int i = 0; i < 130; i++){
        Thread *t = new Thread("forked thread");
        int ret = t->Fork(SimpleThread, 1);
        if (ret == -1){
            printf("thread id %d fork fail：threads number is up to limitation (128)\n", t->get_thread_id());
        }
    }
    scheduler->Print();
    SimpleThread(0);
} 


void Simple(int arg){
    printf("tid: %d, prioiry: %d is running\n", currentThread->get_thread_id(), currentThread->get_priority());
    //interrupt->OneTick();
    printf("tid: %d, prioiry: %d finished\n", currentThread->get_thread_id(), currentThread->get_priority());
}


void Preemptive_test(){
    DEBUG('t', "Entering Preemptive_test");
    for (int i = 0; i < 3; i++){
        
        int p = Random() % 3;
        printf("tid: %d, prioiry: %d is creating a thread with priority %d \n", currentThread->get_thread_id(), currentThread->get_priority(), p);
        
        Thread *t = new Thread("forked thread", p);
        t->Fork(Simple, i);
    }
    printf("tid: %d, prioiry: %d finished\n", currentThread->get_thread_id(), currentThread->get_priority());
        
}

void time_slice_test(){
    DEBUG('t', "Entering time_slice_test");
    for (int i = 0; i < 2; i++){
        Thread *t = new Thread("forked thread", 1);
        t->Fork(SimpleThread, i);
    }
    SimpleThread(0);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    Preemptive_test();
    break;
    case 4:
    time_slice_test();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

