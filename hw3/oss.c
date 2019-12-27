/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: oss.c
# Date: 9/26/19
# Last Update: 
# Purpose: 
	In this project you will be creating an empty shell of an os simulator and 
	doing some very basic tasks in preparation for a more comprehensive simulation later. 

	This will require the use of fork, exec, shared memory, and semaphores.
==================================================================================================== */
#include <errno.h>		//errno variable
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <stdbool.h>	//bool variable
#include <unistd.h>		//standard symbolic constants and types
#include <string.h>		//str function
#include <signal.h>		//signal handling
#include <sys/ipc.h>	//IPC flags
#include <sys/sem.h>	//semget
#include <sys/shm.h>	//shared memory stuff
#include <sys/time.h>	//setitimer()
#include <sys/types.h>	//contains a number of basic derived types
#include <sys/wait.h>	//waitpid()
#include <time.h>		//time()


//Constant
#define MAX_PROCESS 20
#define BUFFER_LENGTH 1024


//Struct
struct clockseg
{
	int second;
	int nanosecond;
};
struct shmmsg
{
	char message[BUFFER_LENGTH];
};


//Static global variable (for dealin with signal handling)
static char *g_executable_name;

static struct clockseg *g_clockseg_shmptr = NULL;
static int g_clockseg_shmid = 0;

static struct shmmsg *g_shmmsg_shmptr = NULL;
static int g_shmmsg_shmid = 0;

static int g_semid = 0;
static struct sembuf g_sema_operation;

static pid_t g_pid;
static int g_all_child_pid[MAX_PROCESS];
static bool g_is_infinite = true;


//Function prototype
void parentInterrupt(int seconds);
void exitHandler(int signum);
void terminateHandler(int signum);
void timer(int seconds);
void finalize();
void releaseClockseg(char *exe_name, char *process_type);
void deleteClockseg(char *exe_name, char *process_type);
void releaseShmmsg(char *exe_name, char *process_type);
void deleteShmmsg(char *exe_name, char *process_type);
void semaClocksegLock();
void semaClocksegRelease();
void semaShmmsgLock();
void semaShmmsgRelease();
void semaTermLock();
void semaTermRelease();


/* ====================================================================================================
MAIN
==================================================================================================== */
int main(int argc, char *argv[]) 
{
	//=====Initialize resources=====
	g_executable_name = argv[0];
	srand(getpid());

	
	//--------------------------------------------------
	//=====Options=====
	//Optional Variables
	int max_child = 5;
	char log_file[256] = "log.dat";
	int time_seconds = 5;

	int opt;
	while((opt = getopt(argc, argv, "hs:l:t:")) != -1)
	{
		switch(opt)
		{
			case 'h':
				printf("NAME:\n");
				printf("	%s - creating an empty shell of an os simulator and doing some very basic tasks\n", g_executable_name);
				printf("\nUSAGE:\n");
				printf("	%s [-h] [-s number] [-l logfile] [-t time].\n", g_executable_name);
				printf("\nDESCRIPTION:\n");
				printf("	-h          : print the help page and exit.\n");
				printf("	-s number   : the maximum number of child processes spawned (default 5).\n");
				printf("	-l filename : the log file used (default log.dat).\n");
				printf("	-t number   : the time in real (not simulated) seconds when the master will terminate itself and all children (default 5).\n\n");
				exit(0);

			case 's':
				max_child = atoi(optarg);
				if(max_child > MAX_PROCESS || max_child <= 0)
				{
					fprintf(stderr, "%s: Given number of processes is invalid! Number must be between 1 to 20 (inclusive).\n", g_executable_name);
					exit(EXIT_FAILURE);
				}
				break;
				
			case 'l':
				strncpy(log_file, optarg, 255);
				printf("Your new log file is: %s\n", log_file);
				break;
				
			case 't':
				time_seconds = atoi(optarg);
				if(time_seconds <= 0)
				{
					fprintf(stderr, "%s: Given duration is not possible! Duration must be positive non-zero whole number.\n", g_executable_name);
					exit(EXIT_FAILURE);
				}
				break;

			default:
				fprintf(stderr, "%s: Please use \"-h\" option for more info.\n", g_executable_name);
				exit(EXIT_FAILURE);
		}
	}
	
	//Check for extra arguments
	if(optind < argc)
	{
		fprintf(stderr, "%s ERROR: extra arguments was given! Please use \"-h\" option for more info.\n", g_executable_name);
		exit(EXIT_FAILURE);
	}


	//--------------------------------------------------
	//=====Initialize LOG File=====
	FILE *fpw = NULL;
	fpw = fopen(log_file, "w");
	if(fpw == NULL)
	{
		fprintf(stderr, "%s ERROR: unable to write the output file.\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	fclose(fpw);
	fpw = NULL;


	//--------------------------------------------------
	//=====Initialize an array=====
	//Zero out all elements of the all child PID array
	int i_zero;
	for(i_zero = 0; i_zero < MAX_PROCESS; i_zero++)
	{
		g_all_child_pid[i_zero] = 0;
	}


	//--------------------------------------------------
	//=====Initialize [clockseg] shared memory=====
	key_t key1 = ftok("./oss.c", 1);
	if(key1 == (key_t)-1)
	{
		fprintf(stderr, "%s ERROR: Failed to derive key from ./oss.c for [clockseg]! Exiting...\n", g_executable_name);
		exit(EXIT_FAILURE);
	}

	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [clockseg] shared memory
	g_clockseg_shmid = shmget(key1, sizeof(struct clockseg), IPC_CREAT | 0666);
	if(g_clockseg_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [clockseg] shared memory! Exiting...\n", g_executable_name);
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. If not, delete the [clockseg] shared memory
	g_clockseg_shmptr = shmat(g_clockseg_shmid, NULL, 0);
	if(g_clockseg_shmptr == (void *)( -1 ))
	{
		fprintf(stderr, "%s ERROR: fail to attach [clockseg] shared memory! Exiting...\n", g_executable_name);

		//Delete [clockseg] shared memory
		deleteClockseg(g_executable_name, "Parent");
		exit(EXIT_FAILURE);	
	}

	//Initialize shared memory attribute of [clockseg]
	g_clockseg_shmptr->second = 0;
	g_clockseg_shmptr->nanosecond = 0;

	
	//--------------------------------------------------
	//=====Initialize [shmmsg] shared memory=====
	key_t key2 = ftok("./oss.c", 2);
	if(key2 == (key_t)-1)
	{
		fprintf(stderr, "%s ERROR: Failed to derive key from ./oss.c for [shmmsg]! Exiting...\n", g_executable_name);
		exit(EXIT_FAILURE);
	}

	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [shmmsg] shared memory
	g_shmmsg_shmid = shmget(key2, sizeof(struct shmmsg), IPC_CREAT | 0666);
	if(g_shmmsg_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [shmmsg] shared memory! Exiting...\n", g_executable_name);

		//Release and delete [clockseg] shared memory
		releaseClockseg(g_executable_name, "Parent");
		deleteClockseg(g_executable_name, "Parent");
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. If not, delete the [shmmsg] shared memory
	g_shmmsg_shmptr = shmat(g_shmmsg_shmid, NULL, 0);
	if(g_shmmsg_shmptr == (void *)( -1 ))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmmsg] shared memory! Exiting...\n", g_executable_name);

		//Delete [shmmsg] shared memory
		deleteShmmsg(g_executable_name, "Parent");

		//Release and delete [clockseg] shared memory
		releaseClockseg(g_executable_name, "Parent");
		deleteClockseg(g_executable_name, "Parent");
        exit(EXIT_FAILURE);
	}


	//--------------------------------------------------
	//=====Initialize semaphore====
	//Can take in private key with IPC_PRIVATE, but in our case we want to share this to user process
	key_t key3 = ftok("./oss.c", 3);
	if(key3 == (key_t)-1)
	{
		fprintf(stderr, "%s ERROR: Failed to derive key from ./oss.c for semaphore! Exiting...\n", g_executable_name);
		exit(EXIT_FAILURE);
	}

	//Creating 3 semaphores elements
	//Create semaphore if doesn't exist with 666 bits permission. Return error if semaphore already exists
	g_semid = semget(key3, 3, IPC_CREAT | IPC_EXCL | 0666);
	if(g_semid == -1)
	{
		fprintf(stderr, "%s ERROR: Failed to create new private semaphore! Exiting...\n", g_executable_name);

		//Release and delete [clockseg] shared memory
		releaseClockseg(g_executable_name, "Parent");
		deleteClockseg(g_executable_name, "Parent");

		//Release and delete [shmmsg] shared memory
		releaseShmmsg(g_executable_name, "Parent");
		deleteShmmsg(g_executable_name, "Parent");
		exit(EXIT_FAILURE);
	}
	
	//Initialize the first semaphore in our set to 1
	//Initialize the second semaphore in our set to 1
	semctl(g_semid, 0, SETVAL, 1);	//Semaphore #1: for user process reading [clockseg] shared memory
	semctl(g_semid, 1, SETVAL, 1);	//Semahpore #2: for user process reading [shmmsg] shared memory
	semctl(g_semid, 2, SETVAL, 1);	//Semahpore #3: for terminating user process


	//--------------------------------------------------
	//=====Signal Handling=====
	parentInterrupt(time_seconds);
	

	//--------------------------------------------------
	//=====Multi Processes=====
	//Child process attributes
	int active_child = 0;
	int number_of_fork = 0;
	int exit_type = 0;
	while(g_is_infinite)
	{
		//Continue to fork if there are still space
		if(active_child < max_child)
		{
			number_of_fork++;
			g_pid = fork();

			//Check if fork success
			if(g_pid == -1)
			{
				fprintf(stderr, "%s (parent) ERROR: %s\n", g_executable_name, strerror(errno));
				exit(0);
			}
		
			//Child
			if(g_pid == 0)
			{
				//Release all shared memory of this child
				releaseClockseg(g_executable_name, "Child");
				releaseShmmsg(g_executable_name, "Child");

				//Terminate uncaught child process
				//This issue can occuer when g_pid is not store correctly or time delay
				if(!g_is_infinite)
				{
					exit(EXIT_SUCCESS);
				}

				//Replaces the current running process with a new process (user)
				int exect_status = execl("./user", "./user", log_file, NULL);
				if(exect_status == -1)
        		{	
					fprintf(stderr, "%s (child) ERROR: execl fail to execute! Exiting...\n", g_executable_name);
				}
				exit(EXIT_FAILURE);
			}
			//Parent
			else
			{	
				//Save current child PID to an array and increment active child
				int i_append;
				for(i_append = 0; i_append < MAX_PROCESS; i_append++)
				{
					if(g_all_child_pid[i_append] == 0)
					{
						g_all_child_pid[i_append] = g_pid;
						break;
					}
				}
				active_child++;
			}
		}


		//--------------------------------------------------
		/*End the infinite when reached 100 forking times.
		Or when reached 2 second simulation time.
		Or in both scenario.
		Turn off timer to avoid collision.*/
		if(number_of_fork >= 100 && g_clockseg_shmptr->second >= 2 && g_is_infinite == true)
		{
			g_is_infinite = false;
			exit_type = -1;
			timer(0);
		}
		else if(number_of_fork >= 100 && g_is_infinite == true)
		{
			g_is_infinite = false;
			exit_type = -2;
			timer(0);
		}
		else if(g_clockseg_shmptr->second >= 2 && g_is_infinite == true)
		{
			g_is_infinite = false;
			exit_type = -3;
			timer(0);
		}


		//--------------------------------------------------
		/*CRITICAL SECTION: Accessing and writing to clockseg shared memory
		Simulating time passing in our system. From 1 to 100 nanoseconds*/
		semaClocksegLock();
		fpw = fopen(log_file, "a");
		fprintf(fpw, "%s: Incrementing time in clockseg\n", g_executable_name);
		fflush(fpw);

		g_clockseg_shmptr->nanosecond += rand() % 100 + 1;
		if(g_clockseg_shmptr->nanosecond >= 1000000000)
		{
			g_clockseg_shmptr->second++;
			g_clockseg_shmptr->nanosecond = 1000000000 - g_clockseg_shmptr->nanosecond;
		}

		fprintf(fpw, "%s: Exiting clockseg\n", g_executable_name);
		fflush(fpw);
		fclose(fpw);
		fpw = NULL;
		semaClocksegRelease();


		//--------------------------------------------------
		/*CRITICAL SECTION: Check if there is a message in shmmsg
		If there is one, that the child process is terminating so OSS should then
		do a wait until that process has left the system and then fork off another
		child and clear shmMsg*/
		if((strcmp(g_shmmsg_shmptr->message, "") != 0))
		{
			semaShmmsgLock();
			fpw = fopen(log_file, "a");
			fprintf(fpw, "%s: Is in shmmsg\n", g_executable_name);
			fflush(fpw);

			printf("%s: Child pid is terminating at my time %d.%d because it reached %s in child process.\n",
					g_executable_name, g_clockseg_shmptr->second, g_clockseg_shmptr->nanosecond, g_shmmsg_shmptr->message);
			
			fprintf(fpw, "%s: Child pid is terminating at my time %d.%d because it reached %s in child process.\n",
					g_executable_name, g_clockseg_shmptr->second, g_clockseg_shmptr->nanosecond, g_shmmsg_shmptr->message);
			fflush(fpw);
			
			//Erase shmmsg message so other user process can write into it again
			//User process can only write to shmmsg when message is empty
			strncpy(g_shmmsg_shmptr->message, "", BUFFER_LENGTH - 1);

			//Note: there will be a blank line between the message below and next user process procedure
			//This is for readability. Feel free to remove the extra end of line if it distracting
			fprintf(fpw, "%s: Exiting shmmsg\n\n", g_executable_name);
			fflush(fpw);	
			fclose(fpw);
			fpw = NULL;
			semaShmmsgRelease();
		}


		//--------------------------------------------------
		//Check to see if a child exit, wait no bound (return immediately if no child has exited)
		//Remove the return child PID out of the array and decrement active child
		int child_status = 0;
		pid_t child_PID = waitpid(-1, &child_status, WNOHANG);

		if(child_PID > 0)
		{
			int i_pop;
			for(i_pop = 0; i_pop < MAX_PROCESS; i_pop++)
			{
				if(g_all_child_pid[i_pop] == child_PID)
				{
					g_all_child_pid[i_pop] = 0;
					break;
				}
			}
			active_child--;
		}
	}

	
	//--------------------------------------------------
	//Display exit type message
	if(exit_type == -1)
	{
		fpw = fopen(log_file, "a");
		fprintf(stderr, "\nMaximum number of processes and maximum simulation seconds has reached!\n");
		fprintf(fpw, "\nMaximum number of processes and maximum simulation seconds has reached!\n");
		fflush(fpw);
		fclose(fpw);
		fpw = NULL;
	}
	else if(exit_type == -2)
	{
		fpw = fopen(log_file, "a");
		fprintf(stderr, "\nMaximum number of processes has reached!\n");
		fprintf(fpw, "\nMaximum number of processes has reached!\n");
		fflush(fpw);
		fclose(fpw);
		fpw = NULL;
	}
	else if(exit_type == -3)
	{
		fpw = fopen(log_file, "a");
		fprintf(stderr, "\nMaximum simulation seconds has reached!\n");
		fprintf(fpw, "\nMaximum simulation seconds has reached!\n");
		fflush(fpw);
		fclose(fpw);
		fpw = NULL;
	}
	else
	{
		fpw = fopen(log_file, "a");
		fprintf(stderr, "\nReal time limit has reached! Waiting for all process to finish.\n");
		fprintf(fpw, "\nReal time limit has reached! Waiting for all process to finish.\n");
		fflush(fpw);
		fclose(fpw);
		fpw = NULL;
	}
	finalize();	

	
	//--------------------------------------------------
	//Print out basic statistic
	printf("- Number of forking during this execution: %d\n", number_of_fork);
	printf("- Final simulation time of this execution: %d.%d\n", g_clockseg_shmptr->second, g_clockseg_shmptr->nanosecond);


	//--------------------------------------------------
	//=====Clean Up=====
	//Release and delete [clockseg] shared memory
	releaseClockseg(g_executable_name, "Parent");
	deleteClockseg(g_executable_name, "Parent");

	//Release and delete [shmmsg] shared memory
	releaseShmmsg(g_executable_name, "Parent");
	deleteShmmsg(g_executable_name, "Parent");

	//Delete semaphore
	semctl(g_semid, 0, IPC_RMID);

	//Final check for closing log file
	if(fpw != NULL)
	{
		fclose(fpw);
		fpw = NULL;
	}
    return EXIT_SUCCESS; 
} 


/* ====================================================================================================
parentInterrupt() and exitHandler()
	Interrupt parent process base on given time and send a terminate to all the child process and
	"user" process. Set the infinite loop flag to false.
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
	finalize();
	g_is_infinite = false;
}


/* ====================================================================================================
timer()
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
finalize()
	Send a terminate to all the child process and "user" process.
==================================================================================================== */
void finalize()
{
	//In case of current child PID is not store in the array, kill it here first
	if(kill(g_pid, 0) == 0)
	{
		semaTermLock();
		if(kill(g_pid, SIGTERM) != 0)
		{
			perror("ERROR");
		}
		else
		{
			waitpid(g_pid, NULL, 0);
		}
		semaTermRelease();
	}

	//Loop through the all child PID array and kill the active process
	int iteration;
	for(iteration = 0; iteration < MAX_PROCESS; iteration++)
	{
		if(g_all_child_pid[iteration] != 0)
		{
			if(kill(g_all_child_pid[iteration], 0) == 0)
			{
				semaTermLock();
				if(kill(g_all_child_pid[iteration], SIGTERM) != 0)
				{
					perror("ERROR");
				}
				else
				{
					waitpid(g_all_child_pid[iteration], NULL, 0);
				}
				semaTermRelease();
			}
		}
	}
}


/* ====================================================================================================
releaseClockseg() and deleteClockseg()
	Release or delete [clockseg] shared memory.
==================================================================================================== */
void releaseClockseg(char *exe_name, char *process_type)
{
	if(g_clockseg_shmptr != NULL)
	{
		if((shmdt(g_clockseg_shmptr)) < 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not detach [clockseg] shared memory!\n", exe_name, process_type);
		}
	}
}
void deleteClockseg(char *exe_name, char *process_type)
{
	if(g_clockseg_shmid > 0)
	{
		if((shmctl(g_clockseg_shmid, IPC_RMID, NULL)) < 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not delete [clockseg] shared memory! Exiting...\n", exe_name, process_type);
		}
	}
}


/* ====================================================================================================
releaseShmmsg() and deleteShmmsg()
	Release or delete [shmmsg] shared memory.
==================================================================================================== */
void releaseShmmsg(char *exe_name, char *process_type)
{
	if(g_shmmsg_shmptr != NULL)
	{
		if((shmdt(g_shmmsg_shmptr)) < 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not detach [clockseg] shared memory!\n", exe_name, process_type);
		}
	}
}
void deleteShmmsg(char *exe_name, char *process_type)
{
	if(g_shmmsg_shmid > 0)
	{
		if((shmctl(g_shmmsg_shmid, IPC_RMID, NULL)) < 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not delete [clockseg] shared memory! Exiting...\n", exe_name, process_type);
		}
	}
}


/* ====================================================================================================
semaClocksegLock()
	Invoke semaphore lock of clockseg
==================================================================================================== */
void semaClocksegLock()
{
	//Semaphore #1: clockseg (lock)
	g_sema_operation.sem_num = 0;
	g_sema_operation.sem_op = -1;
	g_sema_operation.sem_flg = 0;
	semop(g_semid, &g_sema_operation, 1);
}	
	

/* ====================================================================================================
semaClocksegRelease()
	Release semaphore lock of clockseg
==================================================================================================== */
void semaClocksegRelease()
{
	//Semaphore #1: clockseg (unlock)
	g_sema_operation.sem_num = 0;
	g_sema_operation.sem_op = 1;
	g_sema_operation.sem_flg = 0;
	semop(g_semid, &g_sema_operation, 1);
}


/* ====================================================================================================
semaShmmsgLock()
	Invoke semaphore lock of shmmsg
==================================================================================================== */
void semaShmmsgLock()
{
	//Semaphore #2: shmmsg (lock)
	g_sema_operation.sem_num = 1;
	g_sema_operation.sem_op = -1;
	g_sema_operation.sem_flg = 0;
	semop(g_semid, &g_sema_operation, 1);
}


/* ====================================================================================================
semaShmmsgRelease()
	Release semaphore lock of shmmsg
==================================================================================================== */
void semaShmmsgRelease()
{
	//Semaphore #2: shmmsg (unlock)
	g_sema_operation.sem_num = 1;
	g_sema_operation.sem_op = 1;
	g_sema_operation.sem_flg = 0;
	semop(g_semid, &g_sema_operation, 1);
}


/* ====================================================================================================
semaTermLock()
	Invoke semaphore lock of terminating prcoess
==================================================================================================== */
void semaTermLock()
{
	//Semaphore #3: Terminate USER (lock)
	g_sema_operation.sem_num = 2;
	g_sema_operation.sem_op = -1;
	g_sema_operation.sem_flg = 0;
	semop(g_semid, &g_sema_operation, 1);
}


/* ====================================================================================================
SEMATERMUNLOCK
	Release semaphore lock of terminating prcoess
==================================================================================================== */
void semaTermRelease()
{
	//Semaphore #3: Terminate USER (unlock)
	g_sema_operation.sem_num = 2;
	g_sema_operation.sem_op = 1;
	g_sema_operation.sem_flg = 0;
	semop(g_semid, &g_sema_operation, 1);
}

