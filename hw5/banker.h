/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: banker.h
# Date: 10/30/2019
==================================================================================================== */
#ifndef MY_BANKER_H
#define MY_BANKER_H
#include "constant.h"
#include "shared.h"
#include "queue.h"

void setMatrix(struct ProcessControlBlock *pcbt, struct Queue *queue, int maxm[][MAX_RESOURCE], int allot[][MAX_RESOURCE], int count);
void calculateNeedMatrix(struct Data *data, int need[][MAX_RESOURCE], int maxm[][MAX_RESOURCE], int allot[][MAX_RESOURCE], int count);
void displayVector(FILE *fpw, int *line_count, char *v_name, char *l_name, int vector[MAX_RESOURCE]);
void displayMatrix(FILE *fpw, int *line_count, char *m_name, struct Queue *queue, int matrix[][MAX_RESOURCE], int count);
bool bankerAlgorithm(FILE *fpw, int *line_count, bool verbose, struct Data *data, struct ProcessControlBlock *pcbt, struct Queue *queue, int c_index);

#endif

