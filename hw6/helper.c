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


/* ====================================================================================================
* Function    :  strduplicate()
* Definition  :  A utility function to duplicate a string.
* Parameter   :  Char pointer source.
* Return      :  Char pointer.
==================================================================================================== */
char *strduplicate(const char *src) 
{
	size_t len = strlen(src) + 1;       // String plus '\0'
	char *dst = malloc(len);            // Allocate space
	if (dst == NULL) return NULL;       // No memory
	memcpy (dst, src, len);             // Copy the block
	return dst;                         // Return the new string
}
