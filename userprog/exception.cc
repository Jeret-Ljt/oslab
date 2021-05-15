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

#define USER_PROGRAM
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

    switch (which){
        case SyscallException:
            if (type == SC_Halt){
	            DEBUG('a', "Shutdown, initiated by user program.\n");
   	            interrupt->Halt();
            }   else
            if (type == SC_Exit){

                printf("%d\n", machine->ReadRegister(4));
                ASSERT(machine->ReadRegister(4) == 0);
                currentThread->Finish();
            }
        break;
        case TLBMissException:

            printf("TLBMISS! vpage = %d\n", machine->registers[BadVAddrReg] / PageSize);
            stats->numTLBMiss++;
            for (int i = 0; i < machine->InvertTableSize; i++){
                if (machine->registers[BadVAddrReg] / PageSize == machine->InvertTable[i].virtualPage){
                    if (machine->InvertTable[i].valid == false){
                        machine->RaiseException(PageFaultException, machine->registers[BadVAddrReg]);
                        return;
                    }

                    bool full = true;
                    for (int j = 0; j < TLBSize; j++)
                        if (machine->tlb[j].valid == false){
                            machine->tlb[j] = machine->InvertTable[i];
                            full = false;
                            break;
                        }
                    if (full){
                        int min_time = 0x7fffffff, index = 0;
                        for (int j = 0; j < TLBSize; j++)
                            if (machine->tlb[j].last_use_time < min_time){
                                min_time = machine->tlb[j].last_use_time;
                                index = j;
                            }

                         printf("TLB is full and replace the vpage %d\n", machine->tlb[index].virtualPage);
                         printf("before: \n");
                         machine->PrintTlb();
                        machine->tlb[index] = machine->InvertTable[i];
                        machine->tlb[index].last_use_time = stats->totalTicks;
                        printf("after\n");
                        machine->PrintTlb();
                    }
                    
                    return;
                } 
            }
            machine->RaiseException(PageFaultException, machine->registers[BadVAddrReg]);
            break;

        default:
            printf("Unexpected user mode exception %d %d\n", which, type);
            ASSERT(FALSE);
    }
}
