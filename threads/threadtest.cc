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



struct SYNC_CONSUMER_PRODUCER{
    Lock* lock;
    Condition* cond;
    List* queue;
    int queue_max_size;
};


void print_it(int thing){
    printf(" %d", thing);
}

void consumer(int arg){
    SYNC_CONSUMER_PRODUCER* ptr = (SYNC_CONSUMER_PRODUCER *)arg;
    ptr->lock->Acquire();
    while (ptr->queue->size() == 0){
        printf("thread \"%s\" found queue is empty\n", currentThread->getName());
        ptr->cond->Wait(ptr->lock);
    }
    int thing =(int) ptr->queue->Remove();
    printf("thread \"%s\" is consuming product %d\n", currentThread->getName(), thing);
    printf("now we have %d product(s): ", ptr->queue->size());
    ptr->queue->Mapcar(print_it);
    printf("\n");
    if (ptr->queue->size() == ptr->queue_max_size - 1)
        ptr->cond->Broadcast(ptr->lock);
    ptr->lock->Release();
}

void producer(int arg){
    SYNC_CONSUMER_PRODUCER* ptr = (SYNC_CONSUMER_PRODUCER *)arg;

    ptr->lock->Acquire();
    while (ptr->queue->size() == ptr->queue_max_size){
        printf("thread \"%s\" found queue is full(max_size = %d)\n", currentThread->getName(), ptr->queue_max_size);
        ptr->cond->Wait(ptr->lock);
    }

    int thing = currentThread->get_thread_id() - 1;
    ptr->queue->Append((void*)thing);
    printf("thread \"%s\" is proucing product %d\n", currentThread->getName(), thing);
    printf("now we have %d product(s): ", ptr->queue->size());
    ptr->queue->Mapcar(print_it);
    printf("\n");


    if (ptr->queue->size() == 1)
        ptr->cond->Broadcast(ptr->lock);
    ptr->lock->Release();
}

void test_consumer_producer(){
    DEBUG('t', "Entering test_comsumer_producer");
    SYNC_CONSUMER_PRODUCER global_var;
    global_var.lock = new Lock("a lock");
    global_var.cond = new Condition("a cond");
    global_var.queue = new List();
    global_var.queue_max_size = 3;

    for (int i = 0; i < 5; i++){
        char* buf = new char[20];
        sprintf(buf, "producer %d", i);
        Thread *t = new Thread(buf, 0);
        t->Fork(producer,(int) &global_var);
    }

    for (int i = 0; i < 5; i++){
        char* buf = new char[20];
        sprintf(buf, "consumer %d", i);
        Thread *t = new Thread(buf, 0);
        t->Fork(consumer,(int) &global_var);
    }

    currentThread->Yield();
}
void print_s(int arg){
    printf("%s\n", (char *)arg);
}

void go_barrier(int arg){
    Barrier* ptr = (Barrier*) arg;

    printf("thread %d now is going to the barrier\n", currentThread->get_thread_id());
    ptr->SignalAndWait();
    printf("thraed %d doing other thing after barrier\n", currentThread->get_thread_id());
}
void test_barrier(){
    Barrier* bar = new Barrier("new barrier", print_s, 3);
    for (int i = 0; i < 3; i++){
        Thread *t = new Thread("fork thread", 0);
        t->Fork(go_barrier,(int) bar);
    }
    currentThread->Yield();
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
    case 5:
    test_consumer_producer();
    break;
    case 6:
    test_barrier();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

