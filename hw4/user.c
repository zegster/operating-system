/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: user.c
# Date: 10/9/2019
# Purpose:
	All user processes are alike but simulate the system by performing some tasks at random times. 
==================================================================================================== */
#include "shared.h"


/* Static GLOBAL variable (misc) */
static char *g_exe_name;
static key_t g_key;


/* Static GLOBAL variable (shared memory) */
static int g_mqueueid = -1;
static struct Message g_user_message;
static int g_shmclock_shmid = -1;
static struct SharedClock *g_shmclock_shmptr = NULL;
static int g_semid = -1;
static struct sembuf g_sema_operation;
static int g_pcbt_shmid = -1;
static struct ProcessControlBlock *g_pcbt_shmptr = NULL;


/* Prototype Function */
void processInterrupt();
void processHandler(int signum);
void resumeHandler(int signum);
void discardShm(void *shmaddr, char *shm_name , char *exe_name, char *process_type);
void cleanUp();
void semaLock(int sem_index);
void semaRelease(int sem_index);


/* ====================================================================================================
MAIN
==================================================================================================== */
int main(int argc, char *argv[]) 
{
	//=====Signal Handling======
	processInterrupt();


	//--------------------------------------------------
	//=====Initialize resources=====
	g_exe_name = argv[0];
	srand(getpid());


	//--------------------------------------------------
	/* =====Getting message queue===== */
	g_key = ftok("./oss.c", 1);
	g_mqueueid = msgget(g_key, 0600);
	if(g_mqueueid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [queue] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}


	//--------------------------------------------------
	/* =====Getting [shmclock] shared memory===== */
	g_key = ftok("./oss.c", 2);
	g_shmclock_shmid = shmget(g_key, sizeof(struct SharedClock), 0600);
	if(g_shmclock_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not get [shmclock] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. 
	g_shmclock_shmptr = shmat(g_shmclock_shmid, NULL, 0);
	if(g_shmclock_shmptr == (void *)( -1 ))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmclock] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);	
	}


	//--------------------------------------------------
	/* =====Getting semaphore===== */
	g_key = ftok("./oss.c", 3);
	g_semid = semget(g_key, 3, 0600);
	if(g_semid == -1)
	{
		fprintf(stderr, "%s ERROR: failed to create a new private semaphore! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}


	//--------------------------------------------------
	/* =====Getting process control block table===== */
	g_key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(struct ProcessControlBlock) * MAX_PROCESS;
	g_pcbt_shmid = shmget(g_key, process_table_size, 0600);
	if(g_pcbt_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [pcbt] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it.
	g_pcbt_shmptr = shmat(g_pcbt_shmid, NULL, 0);
	if(g_pcbt_shmptr == (void *)( -1 ))
	{
		fprintf(stderr, "%s ERROR: fail to attach [pcbt] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);	
	}



	//--------------------------------------------------
	int index = -1;
	int priority = -1;
	unsigned int spawn_second = g_shmclock_shmptr->second;
	unsigned int spawn_nanosecond = g_shmclock_shmptr->nanosecond;
	unsigned int wait_second = g_shmclock_shmptr->second;
	unsigned int wait_nanosecond = g_shmclock_shmptr->nanosecond;
	int quantum = 0;
	unsigned int burst_time = 0;
	long duration = 0;

	while(1) 
	{
		bool is_terminate = false;
		unsigned long event_time = 0;

		//--------------------------------------------------
		//Checking to see if it is my turn base on the message receive from master (OSS)
		while(1)
		{
			int result = msgrcv(g_mqueueid, &g_user_message, (sizeof(struct Message) - sizeof(long)), getpid(), IPC_NOWAIT);
			if(result != -1)
			{
				if(g_user_message.childPid == getpid())
				{
					//Getting the information from the master message
					index = g_user_message.index;
					priority = g_user_message.priority;

					//Decide what quantum to use base on given priority
					if(priority == 0)
					{
						quantum = FULL_QUANTUM;
					}
					else if(priority == 1)
					{
						quantum = HALF_QUANTUM;
					}
					else if(priority == 2)
					{
						quantum = QUAR_QUANTUM;
					}

					//Determine whether it will use the entire quantum, or only a part of it
					//If it has to use only a part of the quantum, it will generate a random number in the range [0, quantum] to see how long it runs.
					int r = rand() % 2 + 0;
					if(r == 1)
					{
						burst_time = quantum;
					}
					else
					{
						burst_time = rand() % quantum + 0;
					}

					/* Every time a process is scheduled, you will generate a random number in the range [0, 3].
					[0] indicates that the process terminates.
					[1] indicates that the process terminates at its time quantum
					[2] indicates that process starts to wait for an event that will last for r.s seconds, 
						where r and s are random numbers with range [0, 5) and [0, 1000) respectively.
					[3] indicates that the process gets preempted after using p % of its assigned quantum, where p is a random number in the range [1, 99]. 
					- Duration Time is the time that the process can run (base on burst time, which is base on the selective quantum time) */
					int scheduling_choice = rand() % 4 + 0;
					if(scheduling_choice == 0)
					{
						event_time = 0;
						duration = 0;
						is_terminate = true;
					}
					else if(scheduling_choice == 1)
					{
						event_time = 0;
						duration = burst_time;
					}
					else if(scheduling_choice == 2)
					{
						//float r_second = rand() % 6 + 0;
						//float r_millisecond = (rand() % 1001 + 0) / 1000.0;
						unsigned int r_second = 0 * 1000000000;
						unsigned int r_nanosecond = rand() % 1001 + 0;

						event_time = r_second + r_nanosecond;
						duration = burst_time + event_time;
					}
					else if(scheduling_choice == 3)
					{
						event_time = 0;
						double p = (rand() % 99 + 1) / 100;
						duration = burst_time - (p * burst_time);
					}	

					//Send a message to master about my dispatch time
					g_user_message.mtype = 1;
					g_user_message.index = index;
					g_user_message.childPid = getpid();
					g_user_message.second = g_shmclock_shmptr->second;
					g_user_message.nanosecond = g_shmclock_shmptr->nanosecond;
					msgsnd(g_mqueueid, &g_user_message, (sizeof(struct Message) - sizeof(long)), 0);
					break;
				}
			}
		}


		//--------------------------------------------------
		//Run the process up to the given duration
		unsigned int c_nanosecond = g_shmclock_shmptr->nanosecond;
		while(1)
		{
			duration -= g_shmclock_shmptr->nanosecond - c_nanosecond;

			if(duration <= 0)
			{
				//Send a message to master how long I ran for
				g_user_message.mtype = 1;
				g_user_message.index = index;
				g_user_message.childPid = getpid();
				g_user_message.burstTime = burst_time;
				msgsnd(g_mqueueid, &g_user_message, (sizeof(struct Message) - sizeof(long)), 0);
				break;
			}
		}


		//--------------------------------------------------
		//Send a message to master letting him know that I'm either keep running or terminate
		//Also including the index, actual pid, burst_time, cpu_time, and wait_time
		g_user_message.mtype = 1;
		if(is_terminate)
		{
			g_user_message.flag = 0;
		}
		else
		{
			g_user_message.flag = 1;

		}
		g_user_message.index = index;
		g_user_message.childPid = getpid();
		g_user_message.burstTime = burst_time;

		unsigned int elapse_second = g_shmclock_shmptr->second - spawn_second;
		unsigned int elapse_nanosecond = g_shmclock_shmptr->nanosecond + spawn_nanosecond;
		while(elapse_nanosecond >= 1000000000)
		{
			elapse_second++;
			elapse_nanosecond = 1000000000 - elapse_nanosecond;
		}
		g_user_message.spawnSecond = elapse_second;
		g_user_message.spawnNanosecond = elapse_nanosecond;

		unsigned int elapse_wait_second = g_shmclock_shmptr->second - wait_second;
		unsigned int elapse_wait_nanosecond = g_shmclock_shmptr->nanosecond + wait_nanosecond + event_time;
		while(elapse_wait_nanosecond >= 1000000000)
		{
			elapse_wait_second++;
			elapse_wait_nanosecond = 1000000000 - elapse_wait_nanosecond;
		}
		g_user_message.waitSecond = elapse_wait_second;
		g_user_message.waitNanosecond = elapse_wait_nanosecond;
		g_user_message.waitTime = elapse_wait_second + (elapse_wait_nanosecond / 1000000000.0);
		wait_second = elapse_wait_second;
		wait_nanosecond = elapse_wait_nanosecond;

		msgsnd(g_mqueueid, &g_user_message, (sizeof(struct Message) - sizeof(long)), 0);


		//--------------------------------------------------
		//Determine to busy wait again or end the current process
		g_user_message.childPid = -1;
		if(is_terminate)
		{
			break;
		}
	}

	exit(index);
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
	discardShm(g_shmclock_shmptr, "shmclock", g_exe_name, "Child");

	//Release [pcbt] shared memory
	discardShm(g_pcbt_shmptr, "pcbt", g_exe_name, "Child");
}


/* ====================================================================================================
* Function    :  semaLock()
* Definition  :  Invoke semaphore lock of the given semaphore and index.
* Parameter   :  Semaphore Index.
* Return      :  None.
==================================================================================================== */
void semaLock(int sem_index)
{
	g_sema_operation.sem_num = sem_index;
	g_sema_operation.sem_op = -1;
	g_sema_operation.sem_flg = 0;
	semop(g_semid, &g_sema_operation, 1);
}


/* ====================================================================================================
* Function    :  semaRelease()
* Definition  :  Release semaphore lock of the given semaphore and index.
* Parameter   :  Semaphore Index.
* Return      :  None.
==================================================================================================== */
void semaRelease(int sem_index)
{	
	g_sema_operation.sem_num = sem_index;
	g_sema_operation.sem_op = 1;
	g_sema_operation.sem_flg = 0;
	semop(g_semid, &g_sema_operation, 1);
}



