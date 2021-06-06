// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define USER_PROGRAM
#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void run_thread(int arg){
    
    currentThread->space->InitRegisters();		// set the initial register values
    currentThread->space->RestoreState();
    machine->Run();
    ASSERT(FALSE); // machine->Run never returns;
}
void
StartProcess(char *filename)
{
    int index = 0, next_index, len = strlen(filename);
    while (index < len) {
        for (next_index = index; filename[next_index] != ':' && filename[next_index] != '\0'; next_index++);

        filename[next_index] = '\0';
        OpenFile *executable = fileSystem->Open(filename + index);
        AddrSpace *space;

        if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
        }
        space = new AddrSpace(executable); 

        Thread* p_thread = new Thread(filename + index); 
        p_thread->space = space;

        delete executable;			// close file

        p_thread->Fork(run_thread, 0);
		// load page table register

        //machine->Run();			// jump to the user progam
        index = next_index + 1;
    }
    currentThread->Yield();
    //ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}
