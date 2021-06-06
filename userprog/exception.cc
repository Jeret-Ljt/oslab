// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == SyscallException){
        if (type == SC_Halt) {
	        DEBUG('a', "Shutdown, initiated by user program.\n");
   	        interrupt->Halt();
        } else
        if (type == SC_Exit){
            int arg = machine->ReadRegister(4);
            if (arg == 0)
                currentThread->Finish();
            else{
                printf("exit value not = 0\n");
                ASSERT(0);

            }
        }   else
        if (type == SC_Create){
            int ptr = machine->ReadRegister(4);
            char *name = new char[100];
            for (int i = 0; i < 100; i++) name[i] = 0;
            int len = 0;

            do{
                int value;
                machine->ReadMem(ptr, 1, &value);
                ptr++;
                name[len++] = (char)value;
            }   while (name[len-1] != 0);

            fileSystem->Create(name, 0);

            delete[] name;
            int pc = machine->ReadRegister(PCReg);
            machine->WriteRegister(PCReg, pc + 4);
            machine->WriteRegister(NextPCReg, pc + 8);
        }else 
        if (type == SC_Open){
            int ptr = machine->ReadRegister(4);
            char *name = new char[100];
            for (int i = 0; i < 100; i++) name[i] = 0;
            int len = 0;
            do{
                int value;
                machine->ReadMem(ptr, 1, &value);
                ptr++;
                name[len++] = (char)value;
            }   while (name[len-1] != 0);
            OpenFile* opFile = fileSystem->Open(name);
            delete[] name;

            machine->WriteRegister(2, opFile->GetFD());
            int pc = machine->ReadRegister(PCReg);
            machine->WriteRegister(PCReg, pc + 4);
            machine->WriteRegister(NextPCReg, pc + 8);
        }   else
        if (type == SC_Close){
            int fd = machine->ReadRegister(4);
            Close(fd);
            int pc = machine->ReadRegister(PCReg);
            machine->WriteRegister(PCReg, pc + 4);
            machine->WriteRegister(NextPCReg, pc + 8);
        }   else
        if (type == SC_Write){
            int ptr = machine->ReadRegister(4);
            char *buf = new char[100];
            for (int i = 0; i < 100; i++) buf[i] = 0;
            int len = 0;
            do{
                int value;
                machine->ReadMem(ptr, 1, &value);
                ptr++;
                buf[len++] = (char)value;
            }   while (buf[len-1] != 0);


            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);
            WriteFile(fd, buf, size);

            int pc = machine->ReadRegister(PCReg);
            machine->WriteRegister(PCReg, pc + 4);
            machine->WriteRegister(NextPCReg, pc + 8);
        } else
        if (type == SC_Read){
            int ptr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);
            char *buf = new char[size];
            
            Read(fd, buf, size);

            for (int i = 0; i < size; i++)
                machine->WriteMem(ptr + i, 1, int (buf[i]));
            
            int pc = machine->ReadRegister(PCReg);
            machine->WriteRegister(PCReg, pc + 4);
            machine->WriteRegister(NextPCReg, pc + 8);
        } 
    }   else
    {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
