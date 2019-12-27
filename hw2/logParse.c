/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: dt.c
# Date: 9/10/19
# Last Update: 9/17/19
# Purpose: 
	The goal of this homework is to become familiar with using shared memory and creating multiple 
	processes. We will be using getopt and perror as well as fork().

	In this project, you will solve a number of subset sum problems, with each child solving one 
	instance of the problem. A simple definition of the subset sum problem is: Given a set of 
	integers and a value sum, determine if there is a subset of the given set with sum equal to 
	the given sum. In your case, you will determine if there is any subset that sums to 0.
	
	Your project should consist of one program, which will fork off versions of itself to do 
	some file processing. To do this, it will start by taking some command line arguments. 
	Your executable should be called logParse.
==================================================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

//Function prototype
void parentInterrupt(int seconds);
void exitHandler(int signum);
void childInterrupt(int seconds);
void terminateHandler(int signum);
void timer(int seconds);

void strfcat(char *src, char *fmt, ...);
void findSubsetSum(FILE *fpw, int set[], int element, int sum);
void subsetSum(FILE *fpw, bool *is_subset_sum, int set[], int solution[], int current_sum, int index, int element, int sum);

//Static global variable (for dealin with signal handling)
static int g_fork_num = 0;
static int *g_all_child_PID = NULL;

/* ====================================================================================================
MAIN
==================================================================================================== */
int main(int argc, char *argv[])
{
	//Misc
	int iteration;

	//Optional Variables
	char input_file[256] = "input.dat";
	char output_file[256] = "output.dat";
	int time_seconds = 10;

	//Options
	int opt;
	while((opt = getopt(argc, argv, "hi:o:t:")) != -1)
	{
		switch(opt)
		{
			case 'h':
				printf("NAME:\n");
				printf("	%s - solve a number of subset sum problems, with each child solving one instance of the problem.\n", argv[0]);
				printf("\nUSAGE:\n");
				printf("	%s dt [-h] [-i inputfilename] [-o outputfilename] [-t time].\n", argv[0]);
				printf("\nDESCRIPTION:\n");
				printf("	-h                  : print the help page and exit.\n");
				printf("	-i inputfilename    : specifies the input file (default is input.dat).\n");
				printf("	-o outputfilename   : specifies the output file (default is output.dat).\n");
				printf("	-t time             : specifies the maximum duration the code should run (default 10 seconds).\n\n");
				exit(0);

			case 'i':
				strncpy(input_file, optarg, 255);
				printf("Your new input file is: %s\n", input_file);
				break;
				
			case 'o':
				strncpy(output_file, optarg, 255);
				printf("Your new output file is: %s\n", output_file);
				break;
				
			case 't':
				time_seconds = atoi(optarg);
				if(time_seconds <= 0)
				{
					fprintf(stderr, "%s: Given duration is not possible! Duration must be positive non-zero whole number.\n", argv[0]);
					exit(0);
				}
				break;

			default:
				fprintf(stderr, "%s: Please use \"-h\" option for more info.\n", argv[0]);
				exit(0);
		}
	}
	
	//Check for extra arguments
	if(optind < argc)
	{
		fprintf(stderr, "%s ERROR: extra arguments was given! Please use \"-h\" option for more info.\n", argv[0]);
		exit(0);
	}

	//Check if input file and output file have the same name. If so, terminate with error to avoid file corruption.
	if(strcmp(input_file, output_file) == 0)
	{
		fprintf(stderr, "%s ERROR: File corruption may occur when input file and output file have the same name.\n", argv[0]);
		exit(0);
	}

	//Invoke parent interrupt
	parentInterrupt(time_seconds);

	//Check if read/write a file is possible
	FILE *fpr = NULL;
	FILE *fpw = NULL;
	fpr = fopen(input_file, "r");
	fpw = fopen(output_file, "w");
	if(fpr == NULL)
	{
		fprintf(stderr, "%s ERROR: unable to read the input file.\n", argv[0]);
		exit(0);
	}
	if(fpw == NULL)
	{
		fprintf(stderr, "%s ERROR: unable to write the output file.\n", argv[0]);
		exit(0);
	}

	//Create buffer variable.
	char buf[4096];

	//Initialized the number of child process to fork.
	fgets(buf, (sizeof(buf) - 1), fpr);
	g_fork_num = atoi(buf);

	//Create an array to store all child process PID.
	g_all_child_PID = (int *)calloc(g_fork_num, sizeof(int));

	//Forking procedure...
	for(iteration = 0; iteration < g_fork_num; iteration++)
	{
		pid_t pid = fork();
		fgets(buf, (sizeof(buf) - 1), fpr);

		//Check if fork is unsuccessful.
		if(pid == -1)
		{
			fprintf(stderr, "%s (parent) ERROR: %s\n", argv[0], strerror(errno));
			exit(0);
		}
		
		//Child
		if(pid == 0)
		{
			char *token;			//Store the current token
			char token_buf[4096];	//Temporary variable to store buff stream since strtok destroy input variable

			//Get the length of the the token which is the number of element of the set
			//Note: first element of the set is not included
			int set_element = -1;
			strcpy(token_buf, buf);
			token = strtok(token_buf, " \t,.");
			while(token != NULL)
			{
				set_element++;
				token = strtok(NULL, " \t,.");
			}

			//Dynamic allocate size for the subset
			int *subset = NULL;
			subset = (int *)calloc(set_element, sizeof(int));
						
			//Get the given sum
			int given_sum = 0;
			strcpy(token_buf, buf);
			token = strtok(token_buf, " \t,.");
			given_sum = atoi(token);
			token = strtok(NULL, " \t,.");
			
			//Store the remaining token into the subset
			int token_index = 0;
			while(token != NULL)
			{	
				subset[token_index] = atoi(token);
				token = strtok(NULL, " \t,.");
				token_index++;
			}

			//Execute subset sum function
			childInterrupt(1);
			findSubsetSum(fpw, subset, set_element, given_sum);

			//Cleaning up...
			free(g_all_child_PID);
			free(subset);
			exit(0);
		}
		//Parent
		else
		{	
			//Save current child PID to an array
			g_all_child_PID[iteration] = pid;

			//Waiting for child to end.
			int child_status;
			pid_t child_PID = waitpid(pid, &child_status, 0);

			//Check if child process exited abnormally
			if(WIFEXITED(child_status))
			{
				if(WEXITSTATUS(child_status) == 2)
				{
					printf("%d: No valid subset found after 1 second.\n", getpid());
					fprintf(fpw, "%d: No valid subset found after 1 second.\n", getpid());
					fflush(fpw);
				}
			}
			
			//Check if child process was terminated by a signal
			if(WIFSIGNALED(child_status))
			{
				if(WTERMSIG(child_status) == SIGTERM)
				{
					printf("%d: Exceed the time limit! Terminating...\n", child_PID);
					fprintf(fpw, "%d: Exceed the time limit! Terminating...\n", child_PID);
					fflush(fpw);
					break;
				}
			}
		}
	}
	
	//Write all child PID into the outfile
	fprintf(fpw, "Child PID: ");
	printf("Child PID: ");
	fflush(fpw);
	for(iteration = 0; iteration < g_fork_num; iteration++)
	{
		if(g_all_child_PID[iteration] != 0)
		{
			printf("%d ", g_all_child_PID[iteration]);
			fprintf(fpw, "%d ", g_all_child_PID[iteration]);
			fflush(fpw);
		}
	}
	printf("\n");
	fprintf(fpw, "\n");
	fflush(fpw);

	//Write parent PID into the outfile
	printf("Parent PID: %d\n", getpid());
	fprintf(fpw, "Parent PID: %d\n", getpid());
	fflush(fpw);

	//Clean up...
	free(g_all_child_PID);
	fclose(fpr);		
	fclose(fpw);
	return EXIT_SUCCESS;
}

/* ====================================================================================================
PARENTINTERRUPT and EXITHANDLER
	Interrupt parent process base on given time and send a terminate to all the child process.
==================================================================================================== */
void parentInterrupt(int seconds)
{
	timer(seconds);

	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &exitHandler;
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGALRM, &sa, NULL) == -1)
	{
		perror("ERROR");
	}
}
void exitHandler(int signum)
{
	int iteration;
	for(iteration = 0; iteration < g_fork_num; iteration++)
	{
		if(g_all_child_PID[iteration] != 0)
		{
			//Check if the child exists.
			if(kill(g_all_child_PID[iteration], 0) == 0)
			{	
				//If it is, check if the child process can terminate. Display error if not.
				if(kill(g_all_child_PID[iteration], SIGTERM) != 0)
				{
					perror("ERROR");
				}
			}
		}
	}
}

/* ====================================================================================================
CHILDINTERRUPT and TERMINATEHANDLER
	Interrupt child process base on given time and send an user defined terminate signal to itself
==================================================================================================== */
void childInterrupt(int seconds)
{
	timer(seconds);

	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &terminateHandler;
	if(sigaction(SIGALRM, &sa, NULL) == -1)
	{
		perror("ERROR");
	}
}
void terminateHandler(int signum)
{
	exit(2);
}

/* ====================================================================================================
TIMER
	Create a timer that decrement in real time. Once the timer end, send out SIGALRM.
==================================================================================================== */
void timer(int seconds)
{
	//Timers decrement from it_value to zero, generate a signal, and reset to it_interval.
	//A timer which is set to zero (it_value is zero or the timer expires and it_interval is zero) stops.
	struct itimerval value;
	value.it_value.tv_sec = seconds;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	
	if(setitimer(ITIMER_REAL, &value, NULL) == -1)
	{
		perror("ERROR");
	}
}

/* ====================================================================================================
STRFCAT
	Similar to strcat, but allow formatting.
==================================================================================================== */
void strfcat(char *src, char *fmt, ...)
{
	char buf[4096];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	strcat(src, buf);
}

/* ====================================================================================================
FINDSUBSETSUM
	Find the subset sum base on the given set. Output the result on screen and file (if any).
==================================================================================================== */
void findSubsetSum(FILE *fpw, int set[], int element, int sum)
{
	//Create an array that store the solution of the subset sum
	int *solution = NULL;
	solution = (int *)calloc(element, sizeof(int));
	
	//Invoke the driver of the process
	bool is_subset_sum = false;
	subsetSum(fpw, &is_subset_sum, set, solution, 0, 0, element, sum);

	//Check if there a subset sum for the given set
	if(!is_subset_sum)
	{
		printf("%d: No subset of numbers summed to %d.\n", getpid(), sum);
		fprintf(fpw, "%d: No subset of numbers summed to %d.\n", getpid(), sum);
		fflush(fpw);
	}

	free(solution);
}

/* ====================================================================================================
SUBSETSUM (modify version of: https://www.geeksforgeeks.org/subset-sum-problem-dp-25/)
	The main driver of FINESUBSETSUM function. Recursively called until result is found.
	Base on the idea of Generate All Strings of n bits.
==================================================================================================== */
void subsetSum(FILE *fpw, bool *is_subset_sum, int set[], int solution[], int current_sum, int index, int element, int sum)
{
	if(current_sum == sum)
	{	
		*is_subset_sum = true;

		int i;
		bool is_first = true;
		char result[256];

		strfcat(result, "%d: ", getpid());
		for(i = 0; i < element; i++)
		{
			if(solution[i] == 1)
			{
				if(is_first)
				{
					strfcat(result, "%d", set[i]);
					is_first = false;
				}
				else
				{	
					strfcat(result, " + %d", set[i]);
				}
			}
		}
		strfcat(result, " = %d\n", sum);

		printf("%s", result);
		fprintf(fpw, "%s", result);
		fflush(fpw);
	}
	else if(index == element)
	{
		return;
	}
	else
	{
		solution[index] = 1;
		current_sum += set[index];
		subsetSum(fpw, is_subset_sum, set, solution, current_sum, index + 1, element, sum);

		current_sum -= set[index];
		solution[index] = 0;
		subsetSum(fpw, is_subset_sum, set, solution, current_sum, index + 1, element, sum);
	}
}

