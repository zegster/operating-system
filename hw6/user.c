/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: user.c
# Date: 10/30/2019
# Purpose:
	Each user process generates memory references to one of its locations.
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
static Message user_message;
static int shmclock_shmid = -1;
static SharedClock *shmclock_shmptr = NULL;
static int semid = -1;
static struct sembuf sema_operation;
static int pcbt_shmid = -1;
static ProcessControlBlock *pcbt_shmptr = NULL;


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
	exe_name = argv[0];
	exe_index = atoi(argv[1]);
	srand(getpid());


	//--------------------------------------------------
	/* =====Getting shared memory===== */
	getSharedMemory();
	
	//--------------------------------------------------
	bool is_terminate = false;
	int memory_reference = 0;
	unsigned int address = 0;
	unsigned int request_page = 0;
	while(1)
	{
		//Waiting for master signal to get resources
		msgrcv(mqueueid, &user_message, (sizeof(Message) - sizeof(long)), getpid(), 0);
		//DEBUG fprintf(stderr, "%s (%d): my index [%d]\n", exe_name, getpid(), user_message.index);

		if(memory_reference <= 1000)
		{
			//- Requesting Memory -//
			address = rand() % 32768 + 0;
			request_page = address >> 10;
			memory_reference++;
		}
		else
		{
			is_terminate = true;
		}
		
			
		//Send a message to master that I got the signal and master should invoke an action base on my data
		user_message.mtype = 1;
		user_message.flag = (is_terminate) ? 0 : 1;
		user_message.address = address;
		user_message.requestPage = request_page;
		msgsnd(mqueueid, &user_message, (sizeof(Message) - sizeof(long)), 0);


		//--------------------------------------------------
		//Determine sleep again or end the current process
		if(is_terminate)
		{
			break;
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
	shmclock_shmid = shmget(key, sizeof(SharedClock), 0600);
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
	size_t process_table_size = sizeof(ProcessControlBlock) * MAX_PROCESS;
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

