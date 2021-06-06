// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

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

static SyncConsole *sync_console;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------


Semaphore* empty = new Semaphore("CP", 0);
Semaphore* full = new Semaphore("CP2", 1);


void inputC(int arg){
    FileDescribeTable* pipe = fileSystem->fdOpen("pipe");
    for (;;) {
	    char ch = sync_console->GetChar();
        full->P();
        pipe->Write(&ch, 1);
        empty->V();
        if (ch == 'q') 
            interrupt->Halt();    
    }
}

void outputC(int arg){
    FileDescribeTable* pipe = fileSystem->fdOpen("pipe");
    for (;;) {
        char ch;
        empty->P();
        pipe->Read(&ch, 1);
        full->V();
        
        sync_console->PutChar(ch);

        if (ch == 'q') interrupt->Halt();
    }
}

void 
ConsoleTest (char *in, char *out)
{
    char ch;
    sync_console = new SyncConsole(in, out);
    
    fileSystem->Create("pipe", 0, 0);
    Thread* inputT = new Thread("intput", 0);
    inputT->Fork(inputC, 1);
    Thread* outputT = new Thread("output", 0);
    outputT->Fork(outputC, 1);
    currentThread->Sleep();
    return;
}

