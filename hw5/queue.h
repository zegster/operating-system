/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: queue.h
# Date: 10/30/2019
==================================================================================================== */
#ifndef MY_QUEUE_H
#define MY_QUEUE_H


struct QNode 
{ 
	int index;
	struct QNode *next;
}; 


struct Queue 
{ 
	struct QNode *front;
	struct QNode *rear;
	int count;
}; 


struct Queue *createQueue();
struct QNode *newNode(int index);
void enQueue(struct Queue* q, int index);
struct QNode *deQueue(struct Queue *q);
bool isQueueEmpty(struct Queue *q);
int getQueueCount(struct Queue *q);


#endif

