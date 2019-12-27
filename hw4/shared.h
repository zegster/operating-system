/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: shared.h
# Date: 10/9/2019
==================================================================================================== */
#pragma once
#ifndef SHARED_H
#define SHARED_H
#include <stdlib.h>     //exit()
#include <stdio.h>      //printf()
#include <stdbool.h>    //bool variable
#include <stdint.h>     //for uint32_t
#include <string.h>     //str function
#include <unistd.h>     //standard symbolic constants and types

#include <stdarg.h>     //va macro
#include <errno.h>      //errno variable
#include <signal.h>     //signal handling
#include <sys/ipc.h>    //IPC flags
#include <sys/msg.h>    //message queue stuff
#include <sys/shm.h>    //shared memory stuff
#include <sys/sem.h>    //semaphore stuff, semget()
#include <sys/time.h>   //setitimer()
#include <sys/types.h>  //contains a number of basic derived types
#include <sys/wait.h>   //waitpid()
#include <time.h>       //time()


//Constant
#define BUFFER_LENGTH 1024
#define MAX_PROCESS 18
#define TOTAL_PROCESS 100
#define FULL_QUANTUM 5000
#define HALF_QUANTUM FULL_QUANTUM / 2
#define QUAR_QUANTUM HALF_QUANTUM / 2
#define ALPHA 1.2
#define BETA 1.5

struct SharedClock 
{
	unsigned int second;
	unsigned int nanosecond;
};


struct Message
{
	long mtype;
	int flag;	// 0 : isDone, 1 : isQueue
	int index;
	pid_t childPid;
	int priority;
	unsigned int burstTime;
	unsigned int spawnSecond;
	unsigned int spawnNanosecond;
	unsigned int waitSecond;
	unsigned int waitNanosecond;
	double waitTime;
	unsigned int second;
	unsigned int nanosecond;
	char message[BUFFER_LENGTH];
};


struct ProcessControlBlock
{
	int pidIndex;
	pid_t actualPid;
	int priority;
	unsigned int lastBurst;
	unsigned int totalBurst;
	unsigned int totalSystemSecond;
	unsigned int totalSystemNanosecond;
	unsigned int totalWaitSecond;
	unsigned int totalWaitNanosecond;
};


struct UserProcess
{
	int index;
	pid_t actualPid;
	int priority;
};


struct QNode 
{ 
	int index;
	struct QNode *next;
}; 


struct Queue 
{ 
	struct QNode *front;
	struct QNode *rear; 
}; 


#endif

