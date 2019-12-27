/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: user.c
# Date: 10/30/2019
# Purpose:
	The user processes are not actually doing anything, they will ask for resources at random times.
==================================================================================================== */
#include "standardlib.h"
#include "constant.h"
#include "shared.h"


/* Static GLOBAL variable (misc) */
static char *exe_name;
static int exe_index;
static key_t key;


/* Static GLOBAL variable (shared memory) */
static int mqueueid = -1;
static struct Message user_message;
static int shmclock_shmid = -1;
static struct SharedClock *shmclock_shmptr = NULL;
static int semid = -1;
static struct sembuf sema_operation;
static int pcbt_shmid = -1;
static struct ProcessControlBlock *pcbt_shmptr = NULL;


/* Prototype Function */
void processInterrupt();
void processHandler(int signum);
void resumeHandler(int signum);
void discardShm(void *shmaddr, char *shm_name , char *exe_name, char *process_type);
void cleanUp();
void semaLock(int sem_index);
void semaRelease(int sem_index);
void getSharedMemory();


/* ====================================================================================================
MAIN
==================================================================================================== */
int main(int argc, char *argv[]) 
{
	/* =====Signal Handling====== */
	processInterrupt();


	//--------------------------------------------------
	/* =====Initialize resources===== */
	int i;
	exe_name = argv[0];
	exe_index = atoi(argv[1]);
	srand(getpid());


	//--------------------------------------------------
	/* =====Getting shared memory===== */
	getSharedMemory();
	
	//--------------------------------------------------
	bool is_resource_once = false;
	bool is_requesting = false;
	bool is_acquire = false;
	struct SharedClock userStartClock;
	struct SharedClock userEndClock;
	userStartClock.second = shmclock_shmptr->second;
	userStartClock.nanosecond = shmclock_shmptr->nanosecond;
	bool is_ran_duration = false;
	while(1)
	{
		//Waiting for master signal to get resources
		msgrcv(mqueueid, &user_message, (sizeof(struct Message) - sizeof(long)), getpid(), 0);
		//DEBUG fprintf(stderr, "%s (%d): my index [%d]\n", exe_name, getpid(), user_message.index);

		//Did this child process ran for at least one second?
		if(!is_ran_duration)
		{
			userEndClock.second = shmclock_shmptr->second;
			userEndClock.nanosecond = shmclock_shmptr->nanosecond;
			if(abs(userEndClock.nanosecond - userStartClock.nanosecond) >= 1000000000)
			{
				is_ran_duration = true;
			}
			else if(abs(userEndClock.second - userStartClock.second) >= 1)
			{
				is_ran_duration = true;
			}
		}

		//Determine what to do base on whether this process have gotten resource once or not
		/* CHOICE
		- You should have a parameter giving a bound B for when a process should request (or let go of) a resource.
		- Each process, when it starts, should generate a random number in the range [0 : B] and when it occurs, 
			it should try and either claim a new resource or release an already acquired resource. 
		[0] indicates that the process should request some resources (request should never exceed the maximum claims minus whatever the process already has).
		[1] indicates that the process should release already acquired resources.
		[2] indicates that the process terminates and release all resources. */
		bool is_terminate = false;
		bool is_releasing = false;
		int choice;
		if(!is_resource_once || !is_ran_duration)
		{
			choice = rand() % 2 + 0;
		}
		else
		{
			choice = rand() % 3 + 0;
		}

		if(choice == 0)
		{
			is_resource_once = true;

			if(!is_requesting)
			{
				for(i = 0; i < MAX_RESOURCE; i++)
				{
					pcbt_shmptr[exe_index].request[i] = rand() % (pcbt_shmptr[exe_index].maximum[i] - pcbt_shmptr[exe_index].allocation[i] + 1);
				}
				is_requesting = true;
			}
		}
		else if(choice == 1)
		{
			if(is_acquire)
			{
				for(i = 0; i < MAX_RESOURCE; i++)
				{
					pcbt_shmptr[exe_index].release[i] = pcbt_shmptr[exe_index].allocation[i];
				}
				is_releasing = true;
			}
		}
		else if(choice == 2)
		{
			is_terminate = true;
		}

		//Send a message to master that I got the signal and master should invoke an action base on my "choice"
		user_message.mtype = 1;
		user_message.flag = (is_terminate) ? 0 : 1;
		user_message.isRequest = (is_requesting) ? true : false;
		user_message.isRelease = (is_releasing) ? true : false;
		msgsnd(mqueueid, &user_message, (sizeof(struct Message) - sizeof(long)), 0);


		//--------------------------------------------------
		//Determine sleep again or end the current process
		//If sleep again, check if process can proceed the request OR release already allocated resources
		if(is_terminate)
		{
			break;
		}
		else
		{
			/* Update resources by either: 
			[1] If it requesting and it is SAFE, increment allocation vector. 
			[2] If it requesting and it is UNSAFE, do nothing.
			[3] If it releasing, decrement allocation vector base on current allocation vector */
			if(is_requesting)
			{
				//Waiting for master signal to determine if it safe to proceed the request
				msgrcv(mqueueid, &user_message, (sizeof(struct Message) - sizeof(long)), getpid(), 0);

				if(user_message.isSafe == true)
				{
					for(i = 0; i < MAX_RESOURCE; i++)
					{
						pcbt_shmptr[exe_index].allocation[i] += pcbt_shmptr[exe_index].request[i];
						pcbt_shmptr[exe_index].request[i] = 0;
					}
					is_requesting = false;
					is_acquire = true;
				}
			}

			if(is_releasing)
			{
				for(i = 0; i < MAX_RESOURCE; i++)
				{
					pcbt_shmptr[exe_index].allocation[i] -= pcbt_shmptr[exe_index].release[i];
					pcbt_shmptr[exe_index].release[i] = 0;
				}
				is_acquire = false;
			}
		}
	}//END OF: Infinite while loop #1

	cleanUp();
	exit(exe_index);
}


/* ====================================================================================================
* Function    :  processInterrupt() and processHandler()
* Definition  :  Interrupt process when caught SIGTERM. Release all resources.
                  Once that done, simply exit with status 2 code.
* Parameter   :  Number of second.
* Return      :  None.
==================================================================================================== */
void processInterrupt()
{
	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_handler = &processHandler;
	sa1.sa_flags = SA_RESTART;
	if(sigaction(SIGUSR1, &sa1, NULL) == -1)
	{
		perror("ERROR");
	}

	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &processHandler;
	sa2.sa_flags = SA_RESTART;
	if(sigaction(SIGINT, &sa2, NULL) == -1)
	{
		perror("ERROR");
	}
}
void processHandler(int signum)
{
	printf("%d: Terminated!\n", getpid());
	cleanUp();
	exit(2);
}



/* ====================================================================================================
* Function    :  discardShm()
* Definition  :  Detach any shared memory.
* Parameter   :  Shared memory address, shared memory name, current executable name, and current 
                  process type.
* Return      :  None.
==================================================================================================== */
void discardShm(void *shmaddr, char *shm_name , char *exe_name, char *process_type)
{
	//Detaching...
	if(shmaddr != NULL)
	{
		if((shmdt(shmaddr)) << 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not detach [%s] shared memory!\n", exe_name, process_type, shm_name);
		}
	}
}


/* ====================================================================================================
* Function    :  cleanUp()
* Definition  :  Release all shared memory.
* Parameter   :  None.
* Return      :  None.
==================================================================================================== */
void cleanUp()
{
	//Release [shmclock] shared memory
	discardShm(shmclock_shmptr, "shmclock", exe_name, "Child");

	//Release [pcbt] shared memory
	discardShm(pcbt_shmptr, "pcbt", exe_name, "Child");
}


/* ====================================================================================================
* Function    :  semaLock()
* Definition  :  Invoke semaphore lock of the given semaphore and index.
* Parameter   :  Semaphore Index.
* Return      :  None.
==================================================================================================== */
void semaLock(int sem_index)
{
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = -1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}


/* ====================================================================================================
* Function    :  semaRelease()
* Definition  :  Release semaphore lock of the given semaphore and index.
* Parameter   :  Semaphore Index.
* Return      :  None.
==================================================================================================== */
void semaRelease(int sem_index)
{	
	sema_operation.sem_num = sem_index;
	sema_operation.sem_op = 1;
	sema_operation.sem_flg = 0;
	semop(semid, &sema_operation, 1);
}



/* ====================================================================================================
* Function    :  getSharedMemory()
* Definition  :  Get message queue, get shared clock, get semaphore, and get process control block
                  table.
* Parameter   :  None.
* Return      :  None.
==================================================================================================== */
void getSharedMemory()
{
	/* =====Getting [message queue] shared memory===== */
	key = ftok("./oss.c", 1);
	mqueueid = msgget(key, 0600);
	if(mqueueid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [message queue] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}


	//--------------------------------------------------
	/* =====Getting [shmclock] shared memory===== */
	key = ftok("./oss.c", 2);
	shmclock_shmid = shmget(key, sizeof(struct SharedClock), 0600);
	if(shmclock_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [shmclock] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. 
	shmclock_shmptr = shmat(shmclock_shmid, NULL, 0);
	if(shmclock_shmptr == (void *)( -1 ))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmclock] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);	
	}


	//--------------------------------------------------
	/* =====Getting semaphore===== */
	key = ftok("./oss.c", 3);
	semid = semget(key, 1, 0600);
	if(semid == -1)
	{
		fprintf(stderr, "%s ERROR: fail to attach a private semaphore! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//--------------------------------------------------
	/* =====Getting process control block table===== */
	key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(struct ProcessControlBlock) * MAX_PROCESS;
	pcbt_shmid = shmget(key, process_table_size, 0600);
	if(pcbt_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [pcbt] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it.
	pcbt_shmptr = shmat(pcbt_shmid, NULL, 0);
	if(pcbt_shmptr == (void *)( -1 ))
	{
		fprintf(stderr, "%s ERROR: fail to attach [pcbt] shared memory! Exiting...\n", exe_name);
		cleanUp();
		exit(EXIT_FAILURE);	
	}
}

