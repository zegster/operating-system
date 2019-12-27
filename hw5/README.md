## Homework 5 - Resource Management
##### DESCRIPTION:
In this part of the assignment, you will design and implement a resource management module for our Operating System
Simulator (oss). In this project, you will use the deadlock avoidance strategy, using maximum claims, to manage resources.
There is no scheduling in this project, but you will be using shared memory; so be cognizant of possible race conditions.

```
=====Operating System Simulator=====
This will be your main program and serve as the master process. 

	1. You will start the operating system simulator (call the executable oss) as one main process who will fork multiple 
		children at random times. The randomness will be simulated by a logical clock that will be updated by oss as well as 
		user processes. Thus, the logical clock resides in shared memory and is accessed as a critical resource using a semaphore. 

	2. You should have two unsigned integers for the clock; one will show the time in seconds and 
		the other will show the time in nanoseconds, offset from the beginning of a second. 

	3. In the beginning, oss will allocate shared memory for system data structures, including resource descriptors for each resource. 

	4. All the resources are static but some of them may be shared. The resource descriptor is a fixed size structure and contains
		information on managing the resources within oss. 

	5. Make sure that you allocate space to keep track of activities that affect the resources, such as request (maximum), allocation, and release.

	6. The resource descriptors will reside in shared memory and will be accessible to the child. Create descriptors for 20 resources, 
		out of which about 20% should be sharable resources *1. After creating the descriptors, make sure that they are populated with 
		an initial number of resources; assign a number between 1 and 10 (inclusive) for the initial instances in each resource class. 

	7. You may have to initialize another structure in the descriptor to indicate the allocation of specific instances of a resource to a process.

	8. After the resources have been set up, fork a user process at random times (between 1 and 500 milliseconds of your logical clock). 
		Make sure that you never have more than 18 user processes in the system. If you already have 18 processes, do not
		create any more until some process terminates. 

	9. Your user processes execute concurrently and there is no scheduling performed. They run in a loop constantly till they have to terminate.

	10. oss also makes a decision based on the received requests whether the resources should be allocated to processes or not. It does
		so by running the deadlock detection algorithm with the current request from a process and grants the resources if there is no
		deadlock, updating all the data structures. 
		
	11. If a process releases resources, it updates that as well, and may give resources to some waiting processes. If it cannot allocate 
		resources, the process goes in a queue waiting for the resource requested and goes to sleep. It gets awakened when the resources 
		become available, that is whenever the resources are released by a process.

	*1 about implies that it should be 20 +/- 5% and you should generate that number with a random number generator.
```

```
=====User Processes=====
While the user processes are not actually doing anything, they will ask for resources at random times. 

	1. You should have a parameter giving a bound B for when a process should request (or let go of) a resource. 
	
	2. Each process, when it starts, should generate a random number in the range [0 : B] and when it occurs, it should try and either 
		claim a new resource or release an already acquired resource. 

	3. It should make the request by putting a request in shared memory. It will continue to loop and check to see if it is granted 
		that resource.
	
	4. Since we are simulating deadlock avoidance, each user process starts with declaring its maximum claims. The claims can be
		generated using a random number generator, taking into account the fact that no process should ask for more than the maximum
		number of resources in the system. 

	5. You will do that by generating a random number between 0 and the number of instances in the resource descriptor for the resource 
		that has already been set up by oss.
	
	6. The user processes can ask for resources at random times. Make sure that the process does not ask for more than the maximum
		number of available resource instances at any given time, the total for a process (request + allocation) should always be less
		than or equal to the maximum number of instances of a specified resources.

	7. At random times (between 0 and 250ms), the process checks if it should terminate. If so, it should deallocate all the resources
		allocated to it by communicating to oss that it is releasing all those resources. Make sure to do this only after a process has
		run for at least 1 second. 

	8. If the process is not to terminate, make the process request some resources. It will do so by putting a request in the shared memory. 
		The request should never exceed the maximum claims minus whatever the process already has.
		
	9. Also update the system clock. The process may decide to give up resources instead of asking for them.
```

```
=====Information Handling=====
	1. I want you to keep track of statistics during your runs. Keep track of how many requests have been granted.

	2. Make sure that you have signal handling to terminate all processes, if needed. In case of abnormal termination, make sure to
		remove shared memory and semaphores.

	3. When writing to the log file, you should have two ways of doing this. One setting (verbose on) should indicate in the log file
		every time oss gives someone a requested resource or when master sees that a user has finished with a resource. 
		
	4. It should also log the time when a deadlock is detected. 
	
	5. In addition, every 20 granted requests, output a table showing the current resources allocated to each process.
	
	6. When verbose is off, it should only log when a deadlock is detected.
	
	7. Regardless of which option is set, keep track of how many times oss has written to the file. If you have done 100000 lines of
		output to the file, stop writing any output until you have finished the run.
		- NOTE: Professor said not to worry about the number of lines. However, the skeleton structure is already setup.
		
	Note: I give you broad leeway on this project to handle notifications to oss and how you resolve the deadlock. Just make sure
	that you document what you are doing in your README.
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
	- ./oss -v : turn on verbose mode (default is turn off).
--------------------------------------------------------------------------------
##### CHANGELOG:
- Date: Thu Nov 7 15:39:37 2019 -0600
	- Cleaning up.

	
- Date: Wed Nov 6 06:19:15 2019 -0600
	- Working on banker algorithm PART 11.

	
- Date: Tue Nov 5 21:21:29 2019 -0600
	- Working on banker algorithm PART 10.

	
- Date: Tue Nov 5 00:31:24 2019 -0600
	- Working on banker algorithm PART 9.

	
- Date: Mon Nov 4 22:34:13 2019 -0600
	- Working on banker algorithm PART 8.

	
- Date: Mon Nov 4 09:30:27 2019 -0600
	- Working on banker algorithm PART 7.

	
- Date: Mon Nov 4 03:40:53 2019 -0600
	- Working on banker algorithm PART 6.

	
- Date: Sun Nov 3 21:10:56 2019 -0600
	- Working on banker algorithm PART 5.

	
- Date: Sun Nov 3 11:00:30 2019 -0600
	- Working on banker algorithm PART 4.

	
- Date: Sun Nov 3 00:15:54 2019 -0500
	- Working on banker algorithm PART 3.

	
- Date: Sat Nov 2 02:40:53 2019 -0500
	- Working on banker algorithm PART 2.

	
- Date: Sat Nov 2 00:57:36 2019 -0500
	- Working on banker algorithm.

	
- Date: Fri Nov 1 20:14:04 2019 -0500
	- Move constant, struct, standard lib to it own separate file. Prototyping banker algorithm.

	
- Date: Fri Nov 1 18:23:37 2019 -0500
	- Clean up queue.h and queue.c file.

	
- Date: Fri Nov 1 18:00:46 2019 -0500
	- Add init resource.

	
- Date: Thu Oct 31 02:43:06 2019 -0500
	- Clean up update.

	
- Date: Thu Oct 31 02:40:16 2019 -0500
	- Add fork a user process at random times (between 1 and 500 milliseconds of logical clock).

	
- Date: Thu Oct 31 00:20:06 2019 -0500
	- Add queue functionality.

	
- Date: Wed Oct 30 21:16:41 2019 -0500
	- Init procedure for hw5.
