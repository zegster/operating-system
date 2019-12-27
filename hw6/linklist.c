/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: linklist.c
# Date: 11/21/2019
==================================================================================================== */
#include <stdlib.h>     //exit()
#include <stdio.h>      //printf()
#include <stdbool.h>    //bool variable
#include <stdint.h>     //for uint32_t
#include <string.h>     //str function
#include <unistd.h>     //standard symbolic constants and types
#include "helper.h"
#include "linklist.h"


/* ====================================================================================================
* Function    :  createList()
* Definition  :  A utility function to create an empty linked list.
* Parameter   :  None.
* Return      :  Struct List.
==================================================================================================== */
List *createList()
{
	List *l = (List *)malloc(sizeof(List));
	l->front = NULL;
	return l;
}


/* ====================================================================================================
* Function    :  newLNode()
* Definition  :  A utility function to create a new linked list node.
* Parameter   :  Integer index, page, and frame.
* Return      :  Struct LNode.
==================================================================================================== */
LNode *newLNode(int index, int page, int frame)
{ 
    LNode *temp = (LNode *)malloc(sizeof(LNode));
    temp->index = index;
	temp->page = page;
	temp->frame = frame;
    temp->next = NULL;
    return temp;
}


/* ====================================================================================================
* Function    :  addListElement()
* Definition  :  The function to add an index, page, and frame to given linked list.
* Parameter   :  Struct List, integer index, page, and frame.
* Return      :  None.
==================================================================================================== */
void addListElement(List *l, int index, int page, int frame)
{
	LNode *temp = newLNode(index, page, frame);

	if(l->front == NULL)
	{
		l->front = temp;
		return;
	}
	
	LNode *next = l->front;
	while(next->next != NULL)
	{
		next = next->next;
	}
	next->next = temp;
}



/* ====================================================================================================
* Function    :  deleteListFirst()
* Definition  :  Function to remove the first element from given linked list.
* Parameter   :  Struct List.
* Return      :  None.
==================================================================================================== */
void deleteListFirst(List *l) 
{
    if(l->front == NULL)
    {
        return;
    }
    
    LNode *temp = l->front;
    l->front = l->front->next;
    free(temp);
}


/* ====================================================================================================
* Function    :  deleteListElement()
* Definition  :  Function to remove a specific index, page, and frame from given linked list.
* Parameter   :  Struct List, integer index, page, and frame.
* Return      :  None.
==================================================================================================== */
int deleteListElement(List *l, int index, int page, int frame)
{
	LNode *current = l->front;
    LNode *previous = NULL;
    
    if(current == NULL)
    {
        return -1;
    }
    
    while(current->index != index || current->page != page || current->frame != frame)
    {
        if(current->next == NULL)
        {
            return -1;
        }
        else
        {
            previous = current;
            current = current->next;
        }
    }
    
    if(current == l->front)
    {
		int x = current->frame;
		free(current);
        l->front = l->front->next;
		return x;
    }
    else
    {
		int x = previous->next->frame;
		free(previous->next);
        previous->next = current->next;
		return x;
    }
}


/* ====================================================================================================
* Function    :  isInList()
* Definition  :  Find the value base on given search key in a given list.
* Parameter   :  Struct List, and a search key (frame).
* Return      :  True (if found) or false (if not found).
==================================================================================================== */
bool isInList(List *l, int key) 
{
    LNode next;
    next.next = l->front;

    if(next.next == NULL) 
    {
        return false;
    }

    while(next.next->frame != key) 
    {
        if(next.next->next == NULL) 
        {
            return false;
        }
        else 
        {
            next.next = next.next->next;
        }
    }      
	
    return true;
}


/* ====================================================================================================
* Function    :  getList()
* Definition  :  Returns a string representation of the linked list.
* Parameter   :  Struct List.
* Return      :  Char pointer.
==================================================================================================== */
char *getList(const List *l) 
{
	char buf[4096];
    LNode next;
    next.next = l->front;
    
    if(next.next == NULL) 
    {
        return strduplicate(buf);
    }
    
	sprintf(buf, "Linked List: ");
    while(next.next != NULL) 
    {
        sprintf(buf, "%s(%d | %d| %d)", buf, next.next->index, next.next->page, next.next->frame);
        
        next.next = (next.next->next != NULL) ? next.next->next : NULL;
		if(next.next != NULL)
		{
			sprintf(buf, "%s, ", buf);
		}
    }
	sprintf(buf, "%s\n", buf);

	return strduplicate(buf);
}

