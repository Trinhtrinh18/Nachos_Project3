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

#define MAX_LENGTH 255

void IncreasePC()
{
    int pc;
    pc = machine->ReadRegister(PCReg);
    machine->WriteRegister(PrevPCReg, pc);
    pc = machine->ReadRegister(NextPCReg);
    machine->WriteRegister(PCReg, pc);
    pc += 4;
    machine->WriteRegister(NextPCReg, pc);
}

char* User2System(int virtAddr,int limit)
{
    int i;// index
    int oneChar;
    char* kernelBuf = NULL;
    kernelBuf = new char[limit +1];//need for terminal string
    if (kernelBuf == NULL)
        return kernelBuf;
    memset(kernelBuf,0,limit+1);
    //printf("\n Filename u2s:");
    for (i = 0 ; i < limit ;i++)
    {
        machine->ReadMem(virtAddr+i,1,&oneChar);
        kernelBuf[i] = (char)oneChar;
        //printf("%c",kernelBuf[i]);
        if (oneChar == 0)
            break;
    }
    return kernelBuf;
}

int System2User(int virtAddr,int len,char* buffer)
{
    if (len < 0) return -1;
    if (len == 0)return len;
    int i = 0;
    int oneChar = 0 ;
    do{
        oneChar= (int) buffer[i];
        machine->WriteMem(virtAddr+i,1,oneChar);
        i ++;
    }while(i < len && oneChar != 0);
    return i;
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    switch (which) {
        case NoException:
            return;
        case PageFaultException:
            DEBUG('a', "Page Fault Error.\n");
            printf("No valid translation found.\n");
            interrupt->Halt();
            break;
        case SyscallException:
            switch (type) {
                case SC_Halt:
                    DEBUG('a', "Shutdown, initiated by user program.\n");
                    interrupt->Halt();
                    break;
                case SC_ReadChar:
                    SysReadChar();
                    IncreasePC();
                    break;
                case SC_PrintChar:
                    SysPrintChar();
                    IncreasePC();
                    break;
                case SC_ReadString:
                    int virtAddr, length;
                    char* buffer;
                    virtAddr = machine->ReadRegister(4);
                    length = machine->ReadRegister(5);
                    buffer = new char[length + 1];
                    if (length > MAX_LENGTH){
                        DEBUG('a', "\nLength too long.\n");
                        interrupt->Halt();
                    }
                    buffer = User2System(virtAddr, length);
                    synchConsole->Read(buffer, length);
                    System2User(virtAddr, length, buffer);
                    delete[] buffer;
                    IncreasePC();
                    break;

                case SC_PrintString:
                    int vAddr;
                    int len = 0;
                    char* buff;
                    vAddr = machine->ReadRegister(4);
                    buff = User2System(vAddr, MAX_LENGTH);
                    while (buff[len] != 0 && buff[len] != '\n')
                        len++;
                    synchConsole->Write(buff, len + 1);
                    delete[] buff;
                    IncreasePC();
                    break;    

                //case...
            }
        IncreasePC();
    }
}
