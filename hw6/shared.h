/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: shared.h
# Date: 10/30/2019
==================================================================================================== */
#ifndef MY_SHARED_H
#define MY_SHARED_H
#include <stdbool.h>
#include "constant.h"


/* New type variable */
typedef unsigned int uint;


/* SharedClock */
typedef struct 
{
	unsigned int second;
	unsigned int nanosecond;
}SharedClock;


/* Message */
typedef struct
{
	long mtype;
	int index;
	pid_t childPid;
	int flag;	//0 : isDone | 1 : isQueue
	unsigned int address;
	unsigned int requestPage;
	char message[BUFFER_LENGTH];
}Message;


/* PageTableEntry */
typedef struct
{
	uint frameNo;
	uint address: 8;
	uint protection: 1; //What kind of protection you want on that page, such as read only or read and write. (0 : read only | 1 : read and write) 
	uint dirty: 1;      //Is set if the page has been modified or written into. (0 : nay | 1 : aye)
	uint valid: 1;      //Indicate wheter the page is in memory or not. (0: nay | 1 : aye)
}PageTableEntry; 

/* ProcessControlBlock */
typedef struct
{
	int pidIndex;
	pid_t actualPid;
	PageTableEntry page_table[MAX_PAGE];
}ProcessControlBlock;


#endif

