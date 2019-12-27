/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: shared.h
# Date: 10/30/2019
==================================================================================================== */
#ifndef MY_SHARED_H
#define MY_SHARED_H
#include <stdbool.h>

struct SharedClock 
{
	unsigned int second;
	unsigned int nanosecond;
};


struct Message
{
	long mtype;
	int index;
	pid_t childPid;
	int flag;	//0 : isDone | 1 : isQueue
	bool isRequest;
	bool isRelease;
	bool isSafe;
	char message[BUFFER_LENGTH];
};


struct Data
{
	int init_resource[MAX_RESOURCE];
	int resource[MAX_RESOURCE];
	int shared_number;
};


struct ProcessControlBlock
{
	int pidIndex;
	pid_t actualPid;
	int maximum[MAX_RESOURCE];
	int allocation[MAX_RESOURCE];
	int request[MAX_RESOURCE];
	int release[MAX_RESOURCE];
};


#endif

