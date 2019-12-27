/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: queue.c
# Date: 10/30/2019
==================================================================================================== */
#include <stdlib.h>     //exit()
#include <stdio.h>      //printf()
#include <stdbool.h>    //bool variable
#include <stdint.h>     //for uint32_t
#include <string.h>     //str function
#include <unistd.h>     //standard symbolic constants and types
#include "helper.h"
#include "queue.h"


/* ====================================================================================================
* Function    :  createQueue()
* Definition  :  A utility function to create an empty queue.
* Parameter   :  None.
* Return      :  Struct Queue.
==================================================================================================== */
Queue *createQueue()
{
	Queue *q = (Queue *)malloc(sizeof(Queue));
	q->front = NULL;
	q->rear = NULL;
	q->count = 0;
	return q;
}


/* ====================================================================================================
* Function    :  newQNode()
* Definition  :  A utility function to create a new linked list node. 
* Parameter   :  Integer index.
* Return      :  Struct QNode.
==================================================================================================== */
QNode *newQNode(int index)
{ 
    QNode *temp = (QNode *)malloc(sizeof(QNode));
    temp->index = index;
    temp->next = NULL;
    return temp;
} 


/* ====================================================================================================
* Function    :  enQueue()
* Definition  :  The function to add an index to given queue.
* Parameter   :  Struct Queue, and index.
* Return      :  None.
==================================================================================================== */
void enQueue(Queue *q, int index) 
{ 
	//Create a new LL node
	QNode *temp = newQNode(index);

	//Increase queue count
	q->count = q->count + 1;

	//If queue is empty, then new node is front and rear both
	if(q->rear == NULL)
	{
		q->front = q->rear = temp;
		return;
	}

	//Add the new node at the end of queue and change rear 
	q->rear->next = temp;
	q->rear = temp;
}


/* ====================================================================================================
* Function    :  deQueue()
* Definition  :  Function to remove an index from given queue.
* Parameter   :  Struct Queue.
* Return      :  Struct QNode.
==================================================================================================== */
QNode *deQueue(Queue *q) 
{
	//If queue is empty, return NULL
	if(q->front == NULL) 
	{
		return NULL;
	}

	//Store previous front and move front one node ahead
	QNode *temp = q->front;
	free(temp);
	q->front = q->front->next;

	//If front becomes NULL, then change rear also as NULL
	if(q->front == NULL)
	{
		q->rear = NULL;
	}

	//Decrease queue count
	q->count = q->count - 1;
	return temp;
} 


/* ====================================================================================================
* Function    :  isQueueEmpty()
* Definition  :  Check if given queue is empty or not.
* Parameter   :  Struct Queue.
* Return      :  True or false.
==================================================================================================== */
bool isQueueEmpty(Queue *q)
{
	if(q->rear == NULL)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/* ====================================================================================================
* Function    :  getQueueCount()
* Definition  :  A utility function to return queue count. 
* Parameter   :  Struct Queue.
* Return      :  Queue count.
==================================================================================================== */
int getQueueCount(Queue *q)
{
	return (q->count);	
}


/* ====================================================================================================
* Function    :  getQueue()
* Definition  :  Returns a string representation of the linked list.
* Parameter   :  Struct Queue.
* Return      :  Char pointer.
==================================================================================================== */
char *getQueue(const Queue *q)
{
	char buf[4096];
	QNode next;
	next.next = q->front;

	sprintf(buf, "Queue: ");
	while(next.next != NULL)
	{
		sprintf(buf, "%s%d", buf, next.next->index);
		
		next.next = (next.next->next != NULL) ? next.next->next : NULL;
		if(next.next != NULL)
		{
			sprintf(buf, "%s, ", buf);
		}
	}
	sprintf(buf, "%s\n", buf);

	return strduplicate(buf);
}

