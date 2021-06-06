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
extern void run_thread(int arg);

void RunFork(int ptr){
    //printf("ok!");
    currentThread->space->ForkState(ptr);
    machine->Run();
    ASSERT(0);
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int p_page, v_page, d_page;
    switch (which){
        case PageFaultException:
            p_page = machine->MBitMap->Find();
            v_page = machine->registers[BadVAddrReg] / PageSize;
            d_page = machine->pageTable[v_page].tmp_disk_page;
            if (p_page != -1 && currentThread->PhyPageNum < PagePerThread){
                
                machine->pageTable[v_page].physicalPage = p_page;
                machine->pageTable[v_page].pid = currentThread->get_thread_id();
                machine->pageTable[v_page].valid = true;
                machine->pageTable[v_page].dirty = false;
                machine->pageTable[v_page].last_use_time = stats->totalTicks;
                currentThread->PhyPageNum++;
                for (int i = 0; i < PageSize; i++)
                    machine->mainMemory[p_page * PageSize + i] = machine->tmp_disk[d_page * PageSize + i];
            }   else{
                int min_use_time = 0x7fffffff, index, dirty_d_page;
                for (int i = 0; i < machine->pageTableSize; i++){
                    if (machine->pageTable[i].last_use_time < min_use_time && machine->pageTable[i].valid == true){
                        min_use_time = machine->pageTable[i].last_use_time;
                        index = i;
                        p_page = machine->pageTable[i].physicalPage;
                    }
                }
                if (machine->pageTable[index].dirty == true){
                    dirty_d_page = machine->pageTable[index].tmp_disk_page;
                    for (int i = 0; i < PageSize; i++)
                        machine->tmp_disk[dirty_d_page * PageSize + i] = machine->mainMemory[p_page * PageSize + i]; //write back;
                    machine->pageTable[index].dirty = false;
                }
                machine->pageTable[index].valid = false;
                for (int i = 0; i < PageSize; i++)
                    machine->mainMemory[p_page * PageSize + i] = machine->tmp_disk[d_page * PageSize + i];
                
                machine->pageTable[v_page].physicalPage = p_page;
                machine->pageTable[v_page].pid = currentThread->get_thread_id();
                machine->pageTable[v_page].valid = true;
                machine->pageTable[v_page].dirty = false;
                machine->pageTable[v_page].last_use_time = stats->totalTicks;
            }
        break;
        case SyscallException:
            if (type == SC_Halt){
	            DEBUG('a', "Shutdown, initiated by user program.\n");
   	            interrupt->Halt();
            }   else
            if (type == SC_Exit){
                printf("thread %s: exit code: %d\n", currentThread->getName(), machine->ReadRegister(4));
                exitCode[currentThread->get_thread_id()] = machine->ReadRegister(4);
                currentThread->Finish();
            }   else
            if (type == SC_Fork){
                int funcPtr = machine->ReadRegister(4);
                Thread* p_thread = new Thread("fork thread", 0, 1); 
                p_thread->space = currentThread->space;
                p_thread->Fork(RunFork, funcPtr);
                int next_pc = machine->ReadRegister(NextPCReg);
                machine->WriteRegister(PCReg, next_pc);
                machine->WriteRegister(NextPCReg, next_pc + 4);
            }   else
            if (type == SC_Yield){
                currentThread->Yield();

                int next_pc = machine->ReadRegister(NextPCReg);
                machine->WriteRegister(PCReg, next_pc);
                machine->WriteRegister(NextPCReg, next_pc + 4);
            }   else
            if (type == SC_Exec){
                int ptr = machine->ReadRegister(4);
                char *name = new char[100];
                for (int i = 0; i < 100; i++) name[i] = 0;
                int len = 0;
                do{
                    int value;
                    while (!machine->ReadMem(ptr, 1, &value));
                    ptr++;
                    name[len++] = (char)value;
                }   while (name[len-1] != 0);

                OpenFile *executable = fileSystem->Open(name);
                AddrSpace *space;

                if (executable == NULL) {
                    printf("Unable to open file %s\n", name);
                    ASSERT(0);
                }
                space = new AddrSpace(executable); 

                Thread* p_thread = new Thread(name); 
                p_thread->space = space;

                delete executable;		// close file
               // delete[] name;
                p_thread->Fork(run_thread, 0);
                machine->WriteRegister(2, p_thread->get_thread_id());

                int next_pc = machine->ReadRegister(NextPCReg);
                machine->WriteRegister(PCReg, next_pc);
                machine->WriteRegister(NextPCReg, next_pc + 4);
            }   else
            if (type == SC_Join)
            {
                int ptr = machine->ReadRegister(4);
                JoinLock[ptr]->Acquire();
                JoinLock[ptr]->Release();

                machine->WriteRegister(2, exitCode[ptr]);
                int next_pc = machine->ReadRegister(NextPCReg);
                machine->WriteRegister(PCReg, next_pc);
                machine->WriteRegister(NextPCReg, next_pc + 4);
            }   else{
                printf("Unexpected syscall type %d\n", type);
                ASSERT(FALSE);
            }
        break;
        case TLBMissException:
            printf("TLBMISS! vpage = %d\n", machine->registers[BadVAddrReg] / PageSize);
            stats->numTLBMiss++;
            for (int i = 0; i < machine->pageTableSize; i++){
                if (machine->registers[BadVAddrReg] / PageSize == machine->pageTable[i].virtualPage){
                    if (machine->pageTable[i].valid == false){
                        machine->RaiseException(PageFaultException, machine->registers[BadVAddrReg]);
                        return;
                    }

                    bool full = true;
                    for (int j = 0; j < TLBSize; j++)
                        if (machine->tlb[j].valid == false){
                            machine->tlb[j] = machine->pageTable[i];
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
                        machine->tlb[index] = machine->pageTable[i];
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
