/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: oss.c
# Date: 10/9/19
# Purpose:
	The operating system simulator, or OSS, will be your main program and serve as the master process. 
	You will start the simulator (call the executable oss) as one main process who will fork multiple 
	children at random times. The randomness will be simulated by a logical clock that will also be 
	updated by oss.
==================================================================================================== */
#include "shared.h"


/* Static GLOBAL variable (misc) */
static FILE *fpw = NULL;
static char *g_exe_name;
static key_t g_key;


/* Static GLOBAL variable (shared memory) */
static int g_mqueueid = -1;
struct Message g_master_message;
static struct Queue *highQueue;
static struct Queue *midQueue;
static struct Queue *lowQueue;
static int g_shmclock_shmid = -1;
static struct SharedClock *g_shmclock_shmptr = NULL;
static int g_semid = -1;
static struct sembuf g_sema_operation;
static int g_pcbt_shmid = -1;
static struct ProcessControlBlock *g_pcbt_shmptr = NULL;


/* Static GLOBAL variable (fork) */
static int g_fork_number = 0;
static pid_t g_pid = -1;
static unsigned char g_bitmap[MAX_PROCESS];


/* Prototype Function */
void masterInterrupt(int seconds);
void masterHandler(int signum);
void exitHandler(int signum);
void timer(int seconds);
void finalize();
void discardShm(int shmid, void *shmaddr, char *shm_name , char *exe_name, char *process_type);
void cleanUp();
void semaLock(int sem_index);
void semaRelease(int sem_index);
void incShmclock();
struct Queue *createQueue();
struct QNode *newNode(int index);
void enQueue(struct Queue* q, int index);
struct QNode *deQueue(struct Queue *q);
bool isQueueEmpty(struct Queue *q);
struct UserProcess *initUserProcess(int index, pid_t pid);
void userToPCB(struct ProcessControlBlock *pcb, struct UserProcess *user);


/* ====================================================================================================
MAIN
==================================================================================================== */
int main(int argc, char *argv[]) 
{
	/* =====Initialize resources===== */
	g_exe_name = argv[0];
	srand(getpid());

	
	//--------------------------------------------------
	/* =====Options===== */
	//Optional Variables
	char log_file[256] = "log.dat";

	int opt;
	while((opt = getopt(argc, argv, "hl:")) != -1)
	{
		switch(opt)
		{
			case 'h':
				printf("NAME:\n");
				printf("	%s - simulate the process scheduling part of an operating system with the use of message queues for synchronization.\n", g_exe_name);
				printf("\nUSAGE:\n");
				printf("	%s [-h] [-l logfile] [-t time].\n", g_exe_name);
				printf("\nDESCRIPTION:\n");
				printf("	-h          : print the help page and exit.\n");
				printf("	-l filename : the log file used (default log.dat).\n");
				exit(0);

			case 'l':
				strncpy(log_file, optarg, 255);
				printf("Your new log file is: %s\n", log_file);
				break;
				
			default:
				fprintf(stderr, "%s: please use \"-h\" option for more info.\n", g_exe_name);
				exit(EXIT_FAILURE);
		}
	}
	
	//Check for extra arguments
	if(optind < argc)
	{
		fprintf(stderr, "%s ERROR: extra arguments was given! Please use \"-h\" option for more info.\n", g_exe_name);
		exit(EXIT_FAILURE);
	}


	//--------------------------------------------------
	/* =====Initialize LOG File===== */
	fpw = fopen(log_file, "w");
	if(fpw == NULL)
	{
		fprintf(stderr, "%s ERROR: unable to write the output file.\n", argv[0]);
		exit(EXIT_FAILURE);
	}


	//--------------------------------------------------
	/* =====Initialize BITMAP===== */
	//Zero out all elements of bit map
	memset(g_bitmap, '\0', sizeof(g_bitmap));


	//--------------------------------------------------
	/* =====Initialize message queue===== */
	//Allocate shared memory if doesn't exist, and check if it can create one. Return ID for [queue] shared memory
	g_key = ftok("./oss.c", 1);
	g_mqueueid = msgget(g_key, IPC_CREAT | 0600);
	if(g_mqueueid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [queue] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}


	//--------------------------------------------------
	/* =====Initialize [shmclock] shared memory===== */
	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [shmclock] shared memory
	g_key = ftok("./oss.c", 2);
	g_shmclock_shmid = shmget(g_key, sizeof(struct SharedClock), IPC_CREAT | 0600);
	if(g_shmclock_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [shmclock] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. If not, delete the [shmclock] shared memory
	g_shmclock_shmptr = shmat(g_shmclock_shmid, NULL, 0);
	if(g_shmclock_shmptr == (void *)( -1 ))
	{
		fprintf(stderr, "%s ERROR: fail to attach [shmclock] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);	
	}

	//Initialize shared memory attribute of [shmclock]
	g_shmclock_shmptr->second = 0;
	g_shmclock_shmptr->nanosecond = 0;


	//--------------------------------------------------
	/* =====Initialize semaphore===== */
	//Creating 3 semaphores elements
	//Create semaphore if doesn't exist with 666 bits permission. Return error if semaphore already exists
	g_key = ftok("./oss.c", 3);
	g_semid = semget(g_key, 3, IPC_CREAT | IPC_EXCL | 0600);
	if(g_semid == -1)
	{
		fprintf(stderr, "%s ERROR: failed to create a new private semaphore! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}
	
	//Initialize the semaphore(s) in our set to 1
	semctl(g_semid, 0, SETVAL, 1);	//Semaphore #0: for [shmclock] shared memory
	semctl(g_semid, 1, SETVAL, 1);	//Semaphore #1: for terminating user process
	semctl(g_semid, 2, SETVAL, 1);	//Semaphore #2: for [pcbt] shared memory
	


	//--------------------------------------------------
	/* =====Initialize process control block table===== */
	//Allocate shared memory if doesn't exist, and check if can create one. Return ID for [pcbt] shared memory
	g_key = ftok("./oss.c", 4);
	size_t process_table_size = sizeof(struct ProcessControlBlock) * MAX_PROCESS;
	g_pcbt_shmid = shmget(g_key, process_table_size, IPC_CREAT | 0600);
	if(g_pcbt_shmid < 0)
	{
		fprintf(stderr, "%s ERROR: could not allocate [pcbt] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);
	}

	//Attaching shared memory and check if can attach it. If not, delete the [pcbt] shared memory
	g_pcbt_shmptr = shmat(g_pcbt_shmid, NULL, 0);
	if(g_pcbt_shmptr == (void *)( -1 ))
	{
		fprintf(stderr, "%s ERROR: fail to attach [pcbt] shared memory! Exiting...\n", g_exe_name);
		cleanUp();
		exit(EXIT_FAILURE);	
	}


	//--------------------------------------------------
	/* ===== Queue ===== */
	//Set up the high, mid, and low priority queues
	highQueue = createQueue();
	midQueue = createQueue();
	lowQueue = createQueue();


	//--------------------------------------------------
	/* =====Signal Handling===== */
	masterInterrupt(3);


	//--------------------------------------------------
	/* =====Multi Processes===== */
	int last_index = -1;
	bool is_open = false;
	int current_queue = 0;
	float mid_average_wait_time = 0.0;
	float low_average_wait_time = 0.0;
	while(1)
	{
		//Is this spot in the bit map open?
		is_open = false;
		int count_process = 0;
		while(1)
		{
			last_index = (last_index + 1) % MAX_PROCESS;
			uint32_t bit = g_bitmap[last_index / 8] & (1 << (last_index % 8));
			if(bit == 0)
			{
				is_open = true;
				break;
			}
			else
			{
				is_open = false;
			}

			if(count_process >= MAX_PROCESS - 1)
			{
				fprintf(stderr, "%s: bitmap is full (size: %d)\n", g_exe_name, MAX_PROCESS);
				fprintf(fpw, "%s: bitmap is full (size: %d)\n", g_exe_name, MAX_PROCESS);
				fflush(fpw);
				break;
			}
			count_process++;
		}


		//Continue to fork if there are still space in the bitmap
		if(is_open == true)
		{
			g_pid = fork();

			if(g_pid == -1)
			{
				fprintf(stderr, "%s (Master) ERROR: %s\n", g_exe_name, strerror(errno));
				finalize();
				cleanUp();
				exit(0);
			}
		
			if(g_pid == 0) //Child
			{
				//Simple signal handler: this is for mis-synchronization when timer fire off
				signal(SIGUSR1, exitHandler);

				//Replaces the current running process with a new process (user)
				int exect_status = execl("./user", "./user", NULL);
				if(exect_status == -1)
        		{	
					fprintf(stderr, "%s (Child) ERROR: execl fail to execute! Exiting...\n", g_exe_name);
				}
				
				finalize();
				cleanUp();
				exit(EXIT_FAILURE);
			}
			else //Parent
			{	
				//Increment the total number of fork in execution
				g_fork_number++;

				//Set the current index to one bit (meaning it is taken)
				g_bitmap[last_index / 8] |= (1 << (last_index % 8));

				//Initialize the user process and transfer the user process information to the process control block table
				struct UserProcess *user = initUserProcess(last_index, g_pid);
				userToPCB(&g_pcbt_shmptr[last_index], user);

				//Add the process to highest queue
				enQueue(highQueue, last_index);

				//Display creation time
				fprintf(stderr, "%s: generating process with PID (%d) [%d] and putting it in queue [%d] at time %d.%d\n", 
					g_exe_name, g_pcbt_shmptr[last_index].pidIndex, g_pcbt_shmptr[last_index].actualPid, 
					g_pcbt_shmptr[last_index].priority, g_shmclock_shmptr->second, g_shmclock_shmptr->nanosecond);
						
				fprintf(fpw, "%s: generating process with PID (%d) [%d] and putting it in queue [%d] at time %d.%d\n", 
					g_exe_name, g_pcbt_shmptr[last_index].pidIndex, g_pcbt_shmptr[last_index].actualPid, 
					g_pcbt_shmptr[last_index].priority, g_shmclock_shmptr->second, g_shmclock_shmptr->nanosecond);
				fflush(fpw);
			}
		}


		//--------------------------------------------------
		//Scheduling Queue (Multilevel Queue)
		fprintf(stderr, "\n%s: working with queue [%d]\n", g_exe_name, current_queue);
		fprintf(fpw, "\n%s: working with queue [%d]\n", g_exe_name, current_queue);
		fflush(fpw);
		struct QNode next;
		if(current_queue == 0)
		{
			next.next = highQueue->front;
		}
		else if(current_queue == 1)
		{
			next.next = midQueue->front;
		}
		else if(current_queue == 2)
		{
			next.next = lowQueue->front;
		}

		int total_process = 0;
		float total_wait_time = 0.0;
		struct Queue *tempQueue = createQueue();
		while(next.next != NULL)
		{
			total_process++;

			//- CRITICAL SECTION -//
			incShmclock();

			//Sending a message to a specific child to tell him it is his turn
			int c_index = next.next->index;
			g_master_message.mtype = g_pcbt_shmptr[c_index].actualPid;
			g_master_message.index = c_index;
			g_master_message.childPid = g_pcbt_shmptr[c_index].actualPid;
			g_master_message.priority = g_pcbt_shmptr[c_index].priority = current_queue;
			msgsnd(g_mqueueid, &g_master_message, (sizeof(struct Message) - sizeof(long)), 0);
			fprintf(stderr, "%s: signaling process with PID (%d) [%d] from queue [%d] to dispatch\n",
				g_exe_name, g_master_message.index, g_master_message.childPid, current_queue);

			fprintf(fpw, "%s: signaling process with PID (%d) [%d] from queue [%d] to dispatch\n",
				g_exe_name, g_master_message.index, g_master_message.childPid, current_queue);
			fflush(fpw);

			//- CRITICAL SECTION -//
			incShmclock();

			//Getting dispatching time from the child
			msgrcv(g_mqueueid, &g_master_message, (sizeof(struct Message) - sizeof(long)), 1, 0);
			
			fprintf(stderr, "%s: dispatching process with PID (%d) [%d] from queue [%d] at time %d.%d\n",
				g_exe_name, g_master_message.index, g_master_message.childPid, current_queue, 
				g_master_message.second, g_master_message.nanosecond);

			fprintf(fpw, "%s: dispatching process with PID (%d) [%d] from queue [%d] at time %d.%d\n",
				g_exe_name, g_master_message.index, g_master_message.childPid, current_queue, 
				g_master_message.second, g_master_message.nanosecond);
			fflush(fpw);

			//- CRITICAL SECTION -//
			incShmclock();

			//Calculate the total time of the dispatch
			int difference_nanosecond = g_shmclock_shmptr->nanosecond - g_master_message.nanosecond;
			fprintf(stderr, "%s: total time of this dispatch was %d nanoseconds\n", g_exe_name, difference_nanosecond);

			fprintf(fpw, "%s: total time of this dispatch was %d nanoseconds\n", g_exe_name, difference_nanosecond);
			fflush(fpw);

			//--------------------------------------------------
			//Getting how long the current process is running (in nanosecond)
			while(1)
			{
				//- CRITICAL SECTION -//
				incShmclock();
				
				int result = msgrcv(g_mqueueid, &g_master_message, (sizeof(struct Message) - sizeof(long)), 1, IPC_NOWAIT);
				if(result != -1)
				{
					fprintf(stderr, "%s: receiving that process with PID (%d) [%d] ran for %d nanoseconds\n", 
						g_exe_name, g_master_message.index, g_master_message.childPid, g_master_message.burstTime);

					fprintf(fpw, "%s: receiving that process with PID (%d) [%d] ran for %d nanoseconds\n", 
						g_exe_name, g_master_message.index, g_master_message.childPid, g_master_message.burstTime);
					fflush(fpw);
					break;
				}
			}

			//- CRITICAL SECTION -//
			incShmclock();
			
			//--------------------------------------------------
			//Getting if the child finish running or not. Update process control block base on the receive message
			msgrcv(g_mqueueid, &g_master_message, (sizeof(struct Message) - sizeof(long)), 1, 0);
			g_pcbt_shmptr[c_index].lastBurst = g_master_message.burstTime;
			g_pcbt_shmptr[c_index].totalBurst += g_master_message.burstTime;
			g_pcbt_shmptr[c_index].totalSystemSecond = g_master_message.spawnSecond;
			g_pcbt_shmptr[c_index].totalSystemNanosecond = g_master_message.spawnNanosecond;
			g_pcbt_shmptr[c_index].totalWaitSecond += g_master_message.waitSecond;
			g_pcbt_shmptr[c_index].totalWaitNanosecond += g_master_message.waitNanosecond;
			

			if(g_master_message.flag == 0)
			{
				fprintf(stderr, "%s: process with PID (%d) [%d] has finish running at my time %d.%d\n\tLast Burst (nano): %d\n\tTotal Burst (nano): %d\n\tTotal CPU Time (second.nano): %d.%d\n\tTotal Wait Time (second.nano): %d.%d\n",
					g_exe_name, g_master_message.index, g_master_message.childPid, g_shmclock_shmptr->second, g_shmclock_shmptr->nanosecond,
					g_pcbt_shmptr[c_index].lastBurst, g_pcbt_shmptr[c_index].totalBurst, g_pcbt_shmptr[c_index].totalSystemSecond, 
					g_pcbt_shmptr[c_index].totalSystemNanosecond, g_pcbt_shmptr[c_index].totalWaitSecond, g_pcbt_shmptr[c_index].totalWaitNanosecond);

				fprintf(fpw, "%s: process with PID (%d) [%d] has finish running at my time %d.%d\n\tLast Burst (nano): %d\n\tTotal Burst (nano): %d\n\tTotal CPU Time (second.nano): %d.%d\n\tTotal Wait Time (second.nano): %d.%d\n",
					g_exe_name, g_master_message.index, g_master_message.childPid, g_shmclock_shmptr->second, g_shmclock_shmptr->nanosecond,
					g_pcbt_shmptr[c_index].lastBurst, g_pcbt_shmptr[c_index].totalBurst, g_pcbt_shmptr[c_index].totalSystemSecond, 
					g_pcbt_shmptr[c_index].totalSystemNanosecond, g_pcbt_shmptr[c_index].totalWaitSecond, g_pcbt_shmptr[c_index].totalWaitNanosecond);
				fflush(fpw);

				total_wait_time += g_master_message.waitTime;
			}
			else
			{
				if(current_queue == 0)
				{
					if(g_master_message.waitTime > (ALPHA * mid_average_wait_time))
					{
						fprintf(stderr, "%s: putting process with PID (%d) [%d] to queue [1]\n",
							g_exe_name, g_master_message.index, g_master_message.childPid);

						fprintf(fpw, "%s: putting process with PID (%d) [%d] to queue [1]\n",
							g_exe_name, g_master_message.index, g_master_message.childPid);
						fflush(fpw);

						enQueue(midQueue, c_index);
						g_pcbt_shmptr[c_index].priority = 1;
					}
					else
					{
						fprintf(stderr, "%s: not using its entire time quantum. Putting process with PID (%d) [%d] to queue [0]\n",
							g_exe_name, g_master_message.index, g_master_message.childPid);

						fprintf(fpw, "%s: not using its entire time quantum. Putting process with PID (%d) [%d] to queue [0]\n",
							g_exe_name, g_master_message.index, g_master_message.childPid);
						fflush(fpw);

						enQueue(tempQueue, c_index);
						g_pcbt_shmptr[c_index].priority = 0;
					}
				}
				else if(current_queue == 1)
				{
					if(g_master_message.waitTime > (BETA * low_average_wait_time))
					{
						fprintf(stderr, "%s: putting process with PID (%d) [%d] to queue [2]\n",
							g_exe_name, g_master_message.index, g_master_message.childPid);

						fprintf(fpw, "%s: putting process with PID (%d) [%d] to queue [2]\n",
							g_exe_name, g_master_message.index, g_master_message.childPid);
						fflush(fpw);

						enQueue(lowQueue, c_index);
						g_pcbt_shmptr[c_index].priority = 2;
					}
					else
					{
						fprintf(stderr, "%s: not using its entire time quantum. Putting process with PID (%d) [%d] to queue [1]\n",
							g_exe_name, g_master_message.index, g_master_message.childPid);

						fprintf(fpw, "%s: not using its entire time quantum. Putting process with PID (%d) [%d] to queue [1]\n",
							g_exe_name, g_master_message.index, g_master_message.childPid);
						fflush(fpw);

						enQueue(tempQueue, c_index);
						g_pcbt_shmptr[c_index].priority = 1;
					}
				}
				else if(current_queue == 2)
				{
					fprintf(stderr, "%s: putting process with PID (%d) [%d] to queue [2]\n",
						g_exe_name, g_master_message.index, g_master_message.childPid);

					fprintf(fpw, "%s: putting process with PID (%d) [%d] to queue [2]\n",
						g_exe_name, g_master_message.index, g_master_message.childPid);
					fflush(fpw);

					enQueue(tempQueue, c_index);
					g_pcbt_shmptr[c_index].priority = 2;
				}

				total_wait_time += g_master_message.waitTime;
			}

			
			//--------------------------------------------------
			//Point the pointer to the next queue element
			if(next.next->next != NULL)
			{
				next.next = next.next->next;
			}
			else
			{
				next.next = NULL;
			}
		}


		//--------------------------------------------------
		//Calculate the average time for the next queue
		if(total_process == 0)
		{
			total_process = 1;
		}

		if(current_queue == 1)
		{
			mid_average_wait_time = (total_wait_time / total_process);
		}
		else if(current_queue == 2)
		{
			low_average_wait_time = (total_wait_time / total_process);
		}


		//--------------------------------------------------
		//Reassigned the current queue
		if(current_queue == 0)
		{
			while(!isQueueEmpty(highQueue))
			{
				deQueue(highQueue);
			}
			while(!isQueueEmpty(tempQueue))
			{
				int i = tempQueue->front->index;
				enQueue(highQueue, i);
				deQueue(tempQueue);
			}
		}
		else if(current_queue == 1)
		{
			while(!isQueueEmpty(midQueue))
			{
				deQueue(midQueue);
			}
			while(!isQueueEmpty(tempQueue))
			{
				int i = tempQueue->front->index;
				enQueue(midQueue, i);
				deQueue(tempQueue);
			}
		}
		else if(current_queue == 2)
		{
			while(!isQueueEmpty(lowQueue))
			{
				deQueue(lowQueue);
			}
			while(!isQueueEmpty(tempQueue))
			{
				int i = tempQueue->front->index;
				enQueue(lowQueue, i);
				deQueue(tempQueue);
			}
		}
		free(tempQueue);


		//--------------------------------------------------
		//Increment the next queue check
		current_queue = (current_queue + 1) % 3;

		//- CRITICAL SECTION -//
		incShmclock();

		//--------------------------------------------------
		//Check to see if a child exit, wait no bound (return immediately if no child has exit)
		int child_status = 0;
		pid_t child_pid = waitpid(-1, &child_status, WNOHANG);

		//Set the return index bit back to zero (which mean there is a spot open for this specific index in the bitmap)
		if(child_pid > 0)
		{
			int return_index = WEXITSTATUS(child_status);
			g_bitmap[return_index / 8] &= ~(1 << (return_index % 8));
		}

		//--------------------------------------------------
		//End the infinite loop when reached X forking times. Turn off timer to avoid collision.
		if(g_fork_number >= TOTAL_PROCESS)
		{
			timer(0);
			masterHandler(0);
		}
	}

	return EXIT_SUCCESS; 
}


/* ====================================================================================================
* Function    :  masterInterrupt(), masterHandler(), and exitHandler()
* Definition  :  Interrupt master process base on given time or user interrupt via keypress. Send a 
                  terminate signal to all the child process and "user" process.
* Parameter   :  Number of second.
* Return      :  None.
==================================================================================================== */
void masterInterrupt(int seconds)
{
	timer(seconds);
	signal(SIGUSR1, SIG_IGN);

	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_handler = &masterHandler;
	sa1.sa_flags = SA_RESTART;
	if(sigaction(SIGALRM, &sa1, NULL) == -1)
	{
		perror("ERROR");
	}

	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &masterHandler;
	sa2.sa_flags = SA_RESTART;
	if(sigaction(SIGINT, &sa2, NULL) == -1)
	{
		perror("ERROR");
	}
}
void masterHandler(int signum)
{
	finalize();

	//Print out basic statistic
	fprintf(stderr, "- Master PID: %d\n", getpid());
	fprintf(stderr, "- Number of forking during this execution: %d\n", g_fork_number);
	fprintf(stderr, "- Final simulation time of this execution: %d.%d\n", g_shmclock_shmptr->second, g_shmclock_shmptr->nanosecond);

	double throughput = g_shmclock_shmptr->second + ((double)g_shmclock_shmptr->nanosecond / 10000000000.0);
	throughput = (double)g_fork_number / throughput;
	fprintf(stderr, "- Throughput Time: %f (process per time)\n", throughput);

	cleanUp();

	//Final check for closing log file
	if(fpw != NULL)
	{
		fclose(fpw);
		fpw = NULL;
	}

	exit(EXIT_SUCCESS); 
}
void exitHandler(int signum)
{
	printf("%d: Terminated!\n", getpid());
	exit(EXIT_SUCCESS);
}


/* ====================================================================================================
* Function    :  timer()
* Definition  :  Create a timer that decrement in real time. Once the timer end, send out SIGALRM.
* Parameter   :  Number of second.
* Return      :  None.
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
* Function    :  finalize()
* Definition  :  Send a SIGUSR1 signal to all the child process and "user" process.
* Parameter   :  None.
* Return      :  None.
==================================================================================================== */
void finalize()
{
	fprintf(stderr, "\nLimitation has reached! Invoking termination...\n");
	kill(0, SIGUSR1);
	pid_t p = 0;
	while(p >= 0)
	{
		p = waitpid(-1, NULL, WNOHANG);
	}
}


/* ====================================================================================================
* Function    :  discardShm()
* Definition  :  Detach and remove any shared memory.
* Parameter   :  Shared memory ID, shared memory address, shared memory name, current executable name,
                  and current process type.
* Return      :  None.
==================================================================================================== */
void discardShm(int shmid, void *shmaddr, char *shm_name , char *exe_name, char *process_type)
{
	//Detaching...
	if(shmaddr != NULL)
	{
		if((shmdt(shmaddr)) << 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not detach [%s] shared memory!\n", exe_name, process_type, shm_name);
		}
	}
	
	//Deleting...
	if(shmid > 0)
	{
		if((shmctl(shmid, IPC_RMID, NULL)) < 0)
		{
			fprintf(stderr, "%s (%s) ERROR: could not delete [%s] shared memory! Exiting...\n", exe_name, process_type, shm_name);
		}
	}
}


/* ====================================================================================================
* Function    :  cleanUp()
* Definition  :  Release all shared memory and delete all message queue, shared memory, and semaphore.
* Parameter   :  None.
* Return      :  None.
==================================================================================================== */
void cleanUp()
{
	//Delete [queue] shared memory
	if(g_mqueueid > 0)
	{
		msgctl(g_mqueueid, IPC_RMID, NULL);
	}

	//Release and delete [shmclock] shared memory
	discardShm(g_shmclock_shmid, g_shmclock_shmptr, "shmclock", g_exe_name, "Master");

	//Delete semaphore
	if(g_semid > 0)
	{
		semctl(g_semid, 0, IPC_RMID);
	}

	//Release and delete [pcbt] shared memory
	discardShm(g_pcbt_shmid, g_pcbt_shmptr, "pcbt", g_exe_name, "Master");
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


/* ====================================================================================================
* Function    :  incShmclock()
* Definition  :  Increment the logical clock (simulation time).
* Parameter   :  None.
* Return      :  None.
==================================================================================================== */
void incShmclock()
{
	semaLock(0);

	//g_shmclock_shmptr->second++;
	g_shmclock_shmptr->nanosecond += rand() % 1000 + 1;
	if(g_shmclock_shmptr->nanosecond >= 1000000000)
	{
		g_shmclock_shmptr->second++;
		g_shmclock_shmptr->nanosecond = 1000000000 - g_shmclock_shmptr->nanosecond;
	}

	semaRelease(0);
}


/* ====================================================================================================
* Function    :  createQueue()
* Definition  :  A utility function to create an empty queue.
* Parameter   :  None.
* Return      :  Struct Queue.
==================================================================================================== */
struct Queue *createQueue()
{
	struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
	q->front = NULL;
	q->rear = NULL;
	return q;
}


/* ====================================================================================================
* Function    :  newNode()
* Definition  :  A utility function to create a new linked list node. 
* Parameter   :  
* Return      :  Struct QNode.
==================================================================================================== */
struct QNode *newNode(int index)
{ 
    struct QNode *temp = (struct QNode *)malloc(sizeof(struct QNode));
    temp->index = index;
    temp->next = NULL;
    return temp;
} 


/* ====================================================================================================
* Function    :  enQueue()
* Definition  :  The function to add a struct ProcessControlBlock to given queue.
* Parameter   :  Struct Queue, and struct ProcessControlBlock.
* Return      :  None.
==================================================================================================== */
void enQueue(struct Queue *q, int index) 
{ 
	//Create a new LL node
	struct QNode *temp = newNode(index);

	//If queue is empty, then new node is front and rear both
	if(q->rear == NULL)
	{
		q->front = q->rear = temp;
		return;
	}

	//Add the new node at the end of queue and change rear 
	q->rear->next = temp;
	q->rear = temp;
}


/* ====================================================================================================
* Function    :  deQueue()
* Definition  :  Function to remove a struct ProcessControlBlock from given queue.
* Parameter   :  Struct Queue.
* Return      :  Struct QNode.
==================================================================================================== */
struct QNode *deQueue(struct Queue *q) 
{
	//If queue is empty, return NULL
	if(q->front == NULL) 
	{
		return NULL;
	}

	//Store previous front and move front one node ahead
	struct QNode *temp = q->front;
	free(temp);
	q->front = q->front->next;

	//If front becomes NULL, then change rear also as NULL
	if (q->front == NULL)
	{
		q->rear = NULL;
	}

	return temp;
} 


/* ====================================================================================================
* Function    :  isQueueEmpty()
* Definition  :  Check if given queue is empty or not.
* Parameter   :  Struct Queue.
* Return      :  True or false.
==================================================================================================== */
bool isQueueEmpty(struct Queue *q)
{
	if(q->rear == NULL)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/* ====================================================================================================
* Function    :  initUserProcess()
* Definition  :  Sets up the user processes.
* Parameter   :  Fork index, and child pid.
* Return      :  Struct UserProcess.
==================================================================================================== */
struct UserProcess *initUserProcess(int index, pid_t pid)
{
	struct UserProcess *user = (struct UserProcess *)malloc(sizeof(struct UserProcess));
	user->index = index;
	user->actualPid = pid;
	user->priority = 0;
	return user;
}


/* ====================================================================================================
* Function    :  userToPCB()
* Definition  :  Transfer the user process information to the process control block.
* Parameter   :  Struct ProcessControlBlock, Struct UserProcess, and fork index.
* Return      :  Struct ProcessControlBlock.
==================================================================================================== */
void userToPCB(struct ProcessControlBlock *pcb, struct UserProcess *user)
{
	pcb->pidIndex = user->index;
	pcb->actualPid = user->actualPid;
	pcb->priority = user->priority;
	pcb->lastBurst = 0;
	pcb->totalBurst = 0;
	pcb->totalSystemSecond = 0;
	pcb->totalSystemNanosecond = 0;
	pcb->totalWaitSecond = 0;
	pcb->totalWaitNanosecond = 0;
}

