## Homework 4 - Process Scheduling
##### DESCRIPTION:
In this project, you will simulate the process scheduling part of an operating system. You will implement time-based scheduling,
ignoring almost every other aspect of the OS. In this project we will be using message queues for synchronization.
* NOTE: this is simulation, there will be no "real-time waiting". Time result WILL BE inconsistence or divergence.
* NOTE: to avoid confusion, I will be working with second and nanosecond ONLY.
* NOTE: most confusing project...

```
=====Operating System Simulator=====
The operating system simulator, or OSS, will be your main program and serve as the master process. 
You will start the simulator (call the executable oss) as one main process who will fork multiple children at random times. 
The randomness will be simulated by a logical clock that will also be updated by oss.

	1. In the beginning, oss will allocate shared memory for system data structures, including a process table with a process 
		control block for each user process. 
		*NOTE: g_shmclock_shmptr is the shared clock and g_pcbt_shmptr is the process control block table.

	2. The process control block is a fixed size structure and contains information on managing the child process scheduling. 
	
	3. Notice that since it is a simulator, you will not need to allocate space to save the context of child processes. 
	
	4. But you must allocate space for scheduling-related items such as:
		total CPU time used,
		total time in the system, 
		time used during the last burst,
		your local simulated pid, 
		and process priority, if any. 

	5. The process control block resides in shared memory and is accessible to the children.
	
	6. Since we are limiting ourselves to 20 processes in this class, you should allocate space for up to 18 process control 
		blocks. 
	
	7. Also create a bit vector, local to oss, that will help you keep track of the process control blocks (or process IDs) 
		that are already taken.
		*NOTE: g_bitmap is the bitmap (aka bit vector).
	
	8. The clock, as in the last project, will be in nanoseconds. Use two unsigned integers for the clock: one for seconds 
		and the other for nanoseconds. 
		
	9. oss will create user processes at random intervals, say every second on an average. The clock will be accessible to
		every process and hence, in shared memory. It will be advanced only by oss though it can be observed by all the 
		children to see the current time.

	10. oss will create user processes at random intervals (of simulated time), so you will have two constants; let us call them
		[maxTimeBetweenNewProcsNS] and [maxTimeBetweenNewProcsSecs]. oss will launch a new user process based on a random time 
		interval from 0 to those constants. It generates a new process by allocating and initializing the process control block for
		the process and then, forks the process. 
		*NOTE: this part doesn't work for me... Whenever I implement this part and ran my program, I get kick out of the hoare
			server without warning/error message. Only message that it's display is "Connection has been closed by remote host".
			Never found out the reason why (GDB debugger couldn't help me either). I already let Professor Bhatia know about 
			this issue and I suggest to him that I'm doing this differently. Instead of create user processes at random intervals,
			I decided to create a user process for every queue transition. Plus it will also yield more results.

	11. The child process will execl the binary. I would suggest setting these constants initially to spawning a new process about 
		every 1 second, but you can experiment with this later to keep the system busy. 
		*NOTE: look at bullet point number 10 note.
	
	12. New processes that are created can have one of two scheduling classes, either real-time or a normal user process, and will 
		remain in that class for their lifetime. 
		*NOTE: Professor Bhatia told me to not worry about the real-time implementation (not using sleep function).
	
	13. There should be constant representing the percentage of time a process is launched as a normal user process or a real-time
		one. While this constant is specified by you, it should be heavily weighted to generating mainly user processes.
		
	14. oss will run concurrently with all the user processes. After it sets up all the data structures, it enters a loop where it 
		generates and schedules processes. It generates a new process by allocating and initializing the process control block for 
		the process and then, forks the process. 

	15. The child process will execl the binary. oss will be in control of all concurrency, so starting there will be no processes 
		in the system but it will have a time in the future where it will launch a process. 
		
	17. If there are no processes currently ready to run in the system, it should increment the clock until it is the time where it 
		should launch a process. It should then set up that process, generate a new time where it will launch a process and then using
		a message queue, schedule a process to run by sending it a message. It should then wait for a message back from that process 
		that it has finished its task. 
		
	18. If your process table is already full when you go to generate a process, just skip that generation, but do determine
		another time in the future to try and generate a new process.
		
	19. Advance the logical clock by 1.xx seconds in each iteration of the loop where xx is the number of nanoseconds. xx will be a 
		random number in the interval [0, 1000] to simulate some overhead activity for each iteration.
		*NOTE: not sure why incrementing the logical clock by 1.xx seconds. For the sake of simplicity, I'm incrementing the clock by
			1000 to 1 nanoseconds each iteration. You may tweak the logical lock increment in the incShmclock() function.
		
	20. A new process should be generated every 1 second, on an average. So, you should generate a random number between 0 and 2
		assigning it to time to create new process. If your clock has passed this time since the creation of last process, generate 
		a new process (and execl it). If the process table is already full, do not generate any more processes.
		*NOTE: look at bullet point number 10 note.
```

```
=====Scheduling Algorithm=====
***SIMPLE Round Robin***
Assuming you have more than one process in your simulated system, oss will select a process to run and schedule it for execution. It
will select the process by using a scheduling algorithm with the following features:

	1. Implement a version of multi-level scheduling. There are three priority queues – a high-priority queue (queue 0), a medium priority
	queue (queue 1), and a low-priority queue (queue 2).
	
	2. Initially, all processes enter queue 0. A process is moved to queue 1 whenever its waiting time is higher than some threshhold 
	(to be decided by you) and is also higher than
		ALPHA × average waiting time of processes in queue #1

	3. A process is moved from queue 1 to queue 2 whenever its waiting time is higher than some threshold (to be decided by you) and is
	also higher than
		BETA × average waiting time of processes in queue #2

	4. Here, ALPHA and BETA are two constants chosen by you. You can change them to tune performance. I’ll prefer if you #define them in the
		beginning of code. You should play with the values of the threshold, ALPHA, BETA, the time slices for queues, and the time quantum to 
		obtain good throughput.
		
	5. Every time a process is scheduled, you will generate a random number in the range [0, 3] 
		where 0 indicates that the process terminates,
		1 indicates that the process terminates at its time quantum, 
		2 indicates that process starts to wait for an event that will last for r.s seconds where r and s are random numbers with range [0, 5) and [0, 1000) respectively, 
		and 3 indicates that the process gets preempted after using p% of its assigned quantum, where p is a random number in the range [1, 99].
		*NOTE: please see bullet point number 19 in the Operating System Simulator first. For the sake of simplicity and the way I implemented the logical clock, 
			I'm randomly generating r.s with the range [0 , 0) and [0, 1000), where r is the second and s is the millisecond. You may tweak incShmclock() function 
			and the range to get a different result.
		
	6. oss will start by picking processes from queue 0, then 1, and finally 2. It will assign a quantum q to processes in queue 0, q/2 to
		processes in queue 1, and q/4 to processes in queue 2. Once it has gone through all the processes, it will start with processes in queue
		0 again.

	7. The process will be dispatched by putting the process ID and the time quantum in a predefined location in shared memory. The user
		process will pick up the quantum value from the shared memory and schedule itself to run for that long a time.
		*NOTE: I did this differently, instead passing the next process ID and quantum in shared memory, everything will be done using message
			queue. I find this way has better synchronization.
```

```
=====User Processes=====
All user processes are alike but simulate the system by performing some tasks at random times. 

	1. The user process will keep checking in the shared memory location if it has been scheduled and once done, it will start to run. 
	
	2. It should generate a random number to check whether it will use the entire quantum, or only a part of it (a binary random number 
		will be sufficient for this purpose). If it has to use only a part of the quantum, it will generate a random number in the range 
		[0, quantum] to see how long it runs. 

	3. After its allocated time (completed or partial), it updates its process control block by adding to the accumulated CPU time. 
		It joins the ready queue at that point and sends a signal on a semaphore so that oss can schedule another process.
		
	4. While running, generate a random number again to see if the process is completed. This should be done if the process has accumulated
		at least 50 milliseconds. If it is done, the message should be conveyed to oss who should remove its process control block.

	5. Your simulation should end with a report on average turnaround time and average wait time for the processes. Also include how long
		the CPU was idle.

	6. Make sure that you have signal handing to terminate all processes, if needed. In case of abnormal termination, make sure to remove
		shared memory and message queues.
```

```
=====Log Output=====
Your program should send enough output to a log file such that it is possible for me to determine its operation. For example:

	OSS: Generating process with PID 3 and putting it in queue 1 at time 0:5000015
	OSS: Dispatching process with PID 2 from queue 1 at time 0:5000805,
	OSS: total time this dispatch was 790 nanoseconds
	OSS: Receiving that process with PID 2 ran for 400000 nanoseconds
	OSS: Putting process with PID 2 into queue 2
	OSS: Dispatching process with PID 3 from queue 1 at time 0:5401805,
	OSS: total time this dispatch was 1000 nanoseconds
	OSS: Receiving that process with PID 3 ran for 270000 nanoseconds,
	OSS: not using its entire time quantum
	OSS: Putting process with PID 3 into queue 1
	OSS: Dispatching process with PID 1 from queue 1 at time 0:5402505,
	OSS: total time spent in dispatch was 7000 nanoseconds
	etc...
	
I suggest not simply appending to previous logs, but start a new file each time. Also be careful about infinite loops that could generate
excessively long log files. So for example, keep track of total lines in the log file and terminate writing to the log if it exceeds 10000
lines.

Note that the above log was using arbitrary numbers, so your times spent in dispatch could be quite different.
```

```
=====HINT=====
I highly suggest you do this project incrementally. A suggested way to break it down...
	1. Have oss create a process control table with one user process (of real-time class) to verify it is working
	2. Schedule the one user process over and over, logging the data
	3. Create the round robin queue, add additional user processes, making all user processes alternate in it
	4. Keep track of and output statistics like throughput, idle time, etc
	5. Implement an additional user class and the multi-level feedback queue.
	6. Add the chance for user processes to be blocked on an event, keep track of statistics on this
	
	Do not try to do everything at once and be stuck with no idea what is failing.
```

```
=====Termination Criteria=====
	1. oss should stop generating processes if it has already generated 100 processes or if more than 3 real-life seconds have passed. 
	
	2. If you stop adding new processes, the system should eventually empty of processes and then it should terminate. 
	*NOTE: instead of doing this, I decided to terminate all processes regardless what it current state is. This is to avoid any error and collision
		problems. However, this is not a hard thing to implement, but I believe it is better to terminate everything once 3 real-life second or 100 processes
		has exceeded. Plus the way I'm currently implementing the program will require a lot of rewriting code.
	
	3. What is important is that you tune your parameters so that the system has processes in all the queues at some point and that 
	I can see that in the log file. 
	
	4. As discussed previously, ensure that appropriate statistics are displayed.
```

--------------------------------------------------------------------------------
##### HOW TO RUN:
1. In your command promt, type: make
2. This will generate .o file and executable file
3. To use the program, type: ./oss
4. You will be creating an empty shell of an os simulator and doing some very basic tasks
	in preparation for a more comprehensive simulation later. 
5. For more option with this program, type: ./oss -h
6. Few examples:
	- ./oss -h : print out the help page and exit.
	- ./oss -l : specifies the log file (default is log.dat)
--------------------------------------------------------------------------------
##### CHANGELOG: 
- Date: Fri Oct 18 21:34:46 2019 -0500
    - Adjusting logical clock. Working with second and nanosecond to avoid confusion.


- Date: Fri Oct 18 13:23:23 2019 -0500
    - Got bitmap working as intended.


- Date: Fri Oct 18 12:12:57 2019 -0500
    - Bought back bitmap and prototyping bitmap.


- Date: Thu Oct 17 01:28:14 2019 -0500
    - Added universal termination handler.


- Date: Wed Oct 16 22:33:15 2019 -0500
    - Fixing scheduling queue PART 9.


- Date: Wed Oct 16 16:02:37 2019 -0500
    - Fixing scheduling queue PART 8 (fixed traverse queue).


- Date: Wed Oct 16 15:42:42 2019 -0500
    - Fixing scheduling queue PART 7 (unable to traverse queue).


- Date: Wed Oct 16 10:47:26 2019 -0500
    - Fixing scheduling queue PART 6 (multi-process is not working...).


- Date: Tue Oct 15 22:45:33 2019 -0500
    - Fixing scheduling queue PART 5 (multi-process is not working...).


- Date: Tue Oct 15 18:51:06 2019 -0500
    - Fixing scheduling queue PART 4.


- Date: Mon Oct 14 23:48:24 2019 -0500
    - Fixing scheduling queue PART 3.


- Date: Mon Oct 14 22:40:04 2019 -0500
    - Using message queue to send message to master and child for sync.


- Date: Mon Oct 14 09:37:48 2019 -0500
    - Fixing scheduling queue PART 2.


- Date: Sun Oct 13 19:15:29 2019 -0500
    - Fixing scheduling queue PART 1.


- Date: Sun Oct 13 17:00:37 2019 -0500
    - Prototyping linked list queue.


- Date: Sun Oct 13 01:18:25 2019 -0500
    - Testing oss by creating a process control table with one user process.


- Date: Sat Oct 12 23:26:42 2019 -0500
    - Prototyping saving information to PCB.


- Date: Fri Oct 11 23:16:17 2019 -0500
    - Temporary remove bit map...


- Date: Fri Oct 11 21:42:01 2019 -0500
    - Prototyping bit map.


- Date: Fri Oct 11 19:00:57 2019 -0500
    - Prototyping forking procedure.

	
- Date: Fri Oct 11 15:42:14 2019 -0500
    - Prototyping message queue.
    - Added clean up function that release and delete all shared resources.


- Date: Fri Oct 11 12:56:58 2019 -0500
    - Prototyping shared memory in oss.c.


- Date: Thu Oct 10 23:30:04 2019 -0500
    - Added Makefile to HW4.
    - Added shared.h to HW4.
    - Added oss.c to HW4.
        - Added getopt.
    - Added user.c to HW4.