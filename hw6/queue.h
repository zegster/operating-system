/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: queue.h
# Date: 10/30/2019
==================================================================================================== */
#ifndef MY_QUEUE_H
#define MY_QUEUE_H


typedef struct NodeQ
{ 
	int index;
	struct NodeQ *next;
}QNode; 


typedef struct
{ 
	QNode *front;
	QNode *rear;
	int count;
}Queue; 


Queue *createQueue();
QNode *newQNode(int index);
void enQueue(Queue* q, int index);
QNode *deQueue(Queue *q);
bool isQueueEmpty(Queue *q);
int getQueueCount(Queue *q);
char *getQueue(const Queue *q);

#endif

