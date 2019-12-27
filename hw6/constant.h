/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: constant.h
# Date: 10/30/2019
==================================================================================================== */
#ifndef MY_CONSTANT_H
#define MY_CONSTANT_H


/* File related constant */
#define BUFFER_LENGTH 4096
#define MAX_FILE_LINE 100000


/* Process related constant */
//Note (Warning): MAX_PROCESS should not be greater than 18 or 28 (depend on your setting)
#define TERMINATION_TIME 20
#define MAX_PROCESS 18
#define TOTAL_PROCESS 100


/* Paging related constant */
//Note: page is your program (processes) divide into smaller block
//Note: each process will have a requirement of less than 32K memory, with each page being 1K
//Note: each process has 32 pages (each processes can have up to 32 frames at given time)
#define PROCESS_SIZE 32000
#define PAGE_SIZE 1000
#define MAX_PAGE (PROCESS_SIZE / PAGE_SIZE)

//Note: frame is your memory didvide into smaller block
//Note: your system has a total memory of 256K
//Note: you have 256 frames available
//Note: a problem occur when total memory exceed 100k
#define MEMORY_SIZE 256000
#define FRAME_SIZE PAGE_SIZE
#define MAX_FRAME (MEMORY_SIZE / FRAME_SIZE)


#endif

