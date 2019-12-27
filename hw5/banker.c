/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: banker.c
# Date: 10/30/2019
==================================================================================================== */
#include <stdlib.h>     //exit()
#include <stdio.h>      //printf()
#include <stdbool.h>    //bool variable
#include <stdint.h>     //for uint32_t
#include <string.h>     //str function
#include <unistd.h>     //standard symbolic constants and types
#include "banker.h"


/* ====================================================================================================
* Function    :  setMatrix()
* Definition  :  
* Parameter   :  
* Return      :  
==================================================================================================== */
void setMatrix(struct ProcessControlBlock *pcbt, struct Queue *queue, int maxm[][MAX_RESOURCE], int allot[][MAX_RESOURCE], int count)
{
	struct QNode next;
	next.next = queue->front;

	int i, j;
	int c_index = next.next->index;
	for(i = 0; i < count; i++)
	{
		for(j = 0; j < MAX_RESOURCE; j++)
		{
			maxm[i][j] = pcbt[c_index].maximum[j];
			allot[i][j] = pcbt[c_index].allocation[j];
		}

		//Point the pointer to the next queue element
		if(next.next->next != NULL)
		{
			next.next = next.next->next;
			c_index = next.next->index;
		}
		else
		{
			next.next = NULL;
		}
	}
}


/* ====================================================================================================
* Function    :  calculateNeed()
* Definition  :  A utility function to find the need of each process.
* Parameter   :  
* Return      :  None.
==================================================================================================== */
void calculateNeedMatrix(struct Data *data, int need[][MAX_RESOURCE], int maxm[][MAX_RESOURCE], int allot[][MAX_RESOURCE], int count)
{
	int i, j;
	for(i = 0; i < count; i++)
	{
		for(j = 0; j < MAX_RESOURCE; j++) 
		{
			//Need of instance = maxm instance - allocated instance 
			need[i][j] = maxm[i][j] - allot[i][j]; 
		}
	}
}


/* ====================================================================================================
* Function    :  displayVector()
* Definition  :  
* Parameter   :  
* Return      :  
==================================================================================================== */
void displayVector(FILE *fpw, int *line_count, char *v_name, char *l_name, int vector[MAX_RESOURCE])
{
	fprintf(stderr, "===%s Resource===\n%3s :  <", v_name, l_name);
	fprintf(fpw, "===%s Resource===\n%3s :  <", v_name, l_name);

	int i;
	for(i = 0; i < MAX_RESOURCE; i++)
	{
		fprintf(stderr, "%2d", vector[i]);
		fprintf(fpw, "%2d", vector[i]);

		if(i < MAX_RESOURCE - 1)
		{
			fprintf(stderr, " | ");
			fprintf(fpw, " | ");
		}
	}
	fprintf(stderr, ">\n");
	fprintf(fpw, ">\n");
	fflush(fpw);
}


/* ====================================================================================================
* Function    :  displayMatrix()
* Definition  :  
* Parameter   :  
* Return      :  
==================================================================================================== */
void displayMatrix(FILE *fpw, int *line_count, char *m_name, struct Queue *queue, int matrix[][MAX_RESOURCE], int count)
{
	struct QNode next;
	next.next = queue->front;

	int i, j;
	fprintf(stderr, "===%s Matrix===\n", m_name);
	fprintf(fpw, "===%s Matrix===\n", m_name);

	for(i = 0; i < count; i++)
	{
		fprintf(stderr, "P%2d :  <", next.next->index);
		fprintf(fpw, "P%2d :  <", next.next->index);
		for(j = 0; j < MAX_RESOURCE; j++)
		{
			fprintf(stderr, "%2d", matrix[i][j]);
			fprintf(fpw, "%2d", matrix[i][j]);

			if(j < MAX_RESOURCE - 1)
			{
				fprintf(stderr, " | ");
				fprintf(fpw, " | ");
			}
		}
		fprintf(stderr, ">\n");
		fprintf(fpw, ">\n");
		fflush(fpw);

		//Point the pointer to the next queue element
		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}
}


/* ====================================================================================================
* Function    :  bankerAlgorithm()
* Definition  :  A utility function to find the system is in safe state (true) or not (false). 
* Parameter   :  
* Return      :  True or false.
==================================================================================================== */
bool bankerAlgorithm(FILE *fpw, int *line_count, bool verbose, struct Data *data, struct ProcessControlBlock *pcbt, struct Queue *queue, int c_index) 
{
	//=====Main Variable=====
	//i = normal for loop
	//p = find a process which is not finish and whose needs can be satisfied with current work[] resources. 
	//j = normal for loop | check if for all resources of current process need is less than work
	//k = add the allocated resources of current process to the available/work resources i.e. free the resources
	int i, p, j, k;	

	//=====Check for null queue=====
	//Return true (safe) when working queue is null
	struct QNode next;
	next.next = queue->front;
	if(next.next == NULL)
	{
		return true;
	}


	//=====Initialization/Get information=====
	int count = getQueueCount(queue);
	int maxm[count][MAX_RESOURCE];
	int allot[count][MAX_RESOURCE];
	int req[MAX_RESOURCE];
	int need[count][MAX_RESOURCE]; 
	int avail[MAX_RESOURCE];
	
	//Setting up matrix base on given queue and process control block table
	setMatrix(pcbt, queue, maxm, allot, count);

	//Calculate need matrix 
	calculateNeedMatrix(data, need, maxm, allot, count); 

	//Setting up available vector and request vector
	for(i = 0; i < MAX_RESOURCE; i++)
	{
		avail[i] = data->resource[i];
		req[i] = pcbt[c_index].request[i];
	}

	//Update available vector
	for(i = 0; i < count; i++)
	{
		for(j = 0; j < MAX_RESOURCE; j++)
		{
			avail[j] = avail[j] - allot[i][j];
		}
	}

	//Map the PID Index to NEED vector index
	int idx = 0;
	next.next = queue->front;
	while(next.next != NULL)
	{
		if(next.next->index == c_index)
		{
			break;
		}
		idx++;

		//Point the pointer to the next queue element
		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}
	//DEBUG fprintf(fpw, "NEED index: %d\n", idx);

	//Display information
	if(verbose)
	{
		displayMatrix(fpw, line_count, "Maximum", queue, maxm, count);
		displayMatrix(fpw, line_count, "Allocation", queue, allot, count);
		char str[BUFFER_LENGTH];
		sprintf(str, "P%2d", c_index);
		displayVector(fpw, line_count, "Request", str, req);
	}
 

	//=====Finding SAFE Sequence=====
	bool finish[count];        //To store finish 
	int safeSeq[count];        //To store safe sequence 
	memset(finish, 0, count * sizeof(finish[0]));  //Mark all processes as not finish

	//Make a copy of available resources (working vector)
	int work[MAX_RESOURCE];
	for(i = 0; i < MAX_RESOURCE; i++)
	{
		work[i] = avail[i]; 
	}

	/* =====Resource Request Algorithm===== */
	for(j = 0; j < MAX_RESOURCE; j++)
	{
		//Check to see if the process is not asking for more than it will ever need
		if(need[idx][j] < req[j] && j < data->shared_number)
		{
			fprintf(stderr, "NOTICE: Asked for more than initial max request.\n");
			fprintf(fpw, "NOTICE: Asked for more than initial max request.\n");
			fflush(fpw);

			//Display information
			if(verbose)
			{
				displayVector(fpw, line_count, "Available", "A  ", avail);
				displayMatrix(fpw, line_count, "Need", queue, need, count);
			}
			return false;
		}

		if(req[j] <= avail[j] && j < data->shared_number)
		{
			avail[j] -= req[j];
			allot[idx][j] += req[j];
			need[idx][j] -= req[j];
		}
		else
		{
			fprintf(stderr, "NOTICE: Not enough available resources!\n");
			fprintf(fpw, "NOTICE: Not enough available resources!\n");
			fflush(fpw);

			//Display information
			if(verbose)
			{
				displayVector(fpw, line_count, "Available", "A  ", avail);
				displayMatrix(fpw, line_count, "Need", queue, need, count);
			}
			return false;
		}
	}

	/* =====Safety Algorithm===== */
	//While all processes are not finished or system is not in safe state. 
	int index = 0; 
	while(index < count) 
	{
		//Find a process which is not finish and whose needs can be satisfied with current work[] resources. 
		bool found = false; 
		for(p = 0; p < count; p++) 
		{ 
			//First check if a process is finished, if no, go for next condition 
			if(finish[p] == 0) 
			{ 
				//Check if for all resources of current process need is less than work 
				for (j = 0; j < MAX_RESOURCE; j++)
				{
					if(need[p][j] > work[j] && data->shared_number)
					{ 
						break;
					}
				}

				//If all needs of p were satisfied. 
				if(j == MAX_RESOURCE) 
				{
					//Add the allocated resources of current process to the available/work resources i.e. free the resources
					for (k = 0 ; k < MAX_RESOURCE ; k++)
					{
						work[k] += allot[p][k]; 
					}

					//Add this process to safe sequence. 
					safeSeq[index++] = p;

					//Mark this p as finished and found is true
					finish[p] = 1;
					found = true;
				}
			}
		}

		//If we could not find a next process in safe sequence. 
		if(found == false) 
		{
			fprintf(stderr, "System is in UNSAFE (not safe) state\n"); 
			fprintf(fpw, "System is in UNSAFE (not safe) state\n");
			fflush(fpw);
			return false; 
		}
	}//END OF: index < count

	//Display information
	if(verbose)
	{
		displayVector(fpw, line_count, "Available", "A  ", avail);
		displayMatrix(fpw, line_count, "Need", queue, need, count);
	}

	//Map the safe sequence with the queue sequence
	int sequence[count];
	int seq_index = 0;
	next.next = queue->front;
	while(next.next != NULL)
	{
		sequence[seq_index++] = next.next->index; 

		//Point the pointer to the next queue element
		next.next = (next.next->next != NULL) ? next.next->next : NULL;
	}
	
	//If system is in safe state then safe sequence will be as below 
	fprintf(stderr, "System is in SAFE state. Safe sequence is: "); 
	fprintf(fpw, "System is in SAFE state. Safe sequence is: "); 
	for (i = 0; i < count ; i++)
	{
		fprintf(stderr, "%2d ", sequence[safeSeq[i]]);
		fprintf(fpw, "%2d ", sequence[safeSeq[i]]);
	}
	fprintf(stderr, "\n\n");
	fprintf(fpw, "\n");
	fflush(fpw);

	return true; 
}

