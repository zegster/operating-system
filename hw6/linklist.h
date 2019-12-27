/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: linklist.h
# Date: 11/21/2019
==================================================================================================== */
#ifndef MY_LINKLIST_H
#define MY_LINKLIST_H


typedef struct NodeL
{ 
	int index;
	int page;
	int frame;
	struct NodeL *next;
}LNode; 


typedef struct
{ 
	LNode *front;
}List;


List *createList();
LNode *newLNode(int index, int page, int frame);

void addListElement(List *l, int index, int page, int frame);
void deleteListFirst(List *l);
int deleteListElement(List *l, int index, int page, int frame);
bool isInList(List *l, int key);
char *getList(const List *l);

#endif

