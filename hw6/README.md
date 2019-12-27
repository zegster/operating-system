## Homework 6 - Memory Management
##### DESCRIPTION:
Design and implement a memory management module for our Operating System Simulator oss. Implement and compare lru and fifo page replacement algorithms, both with dirty-bit optimization. When a page-fault occurs, it will be necessary to swap in that page. If there are no empty frames, your algorithm will select the victim frame based on the algorithm for that module (fifo or lru).
Each frame should also have an additional dirty bit, which is set on writing to the frame. This bit is necessary to consider dirty bit optimization when determining how much time these operations take. The dirty bit will be implemented as a part of the page table.

```
=====Operating System Simulator=====
1. This will be your main program and serve as the master process. You will start the operating system simulator 
		(call the executable oss) as one main process who will fork multiple children at random times. 

	2. The randomness will be simulated by a logical clock that will be updated by oss as well as user processes. 
		Thus, the logical clock resides in shared memory and is accessed as a critical resource using a semaphore. 

	3. You should have two unsigned integers for the clock; one will show the time in seconds and the other will 
		show the time in nanoseconds, offset from the beginning of a second.
		
	4. In the beginning, oss will allocate shared memory for system data structures, including page table. You can create
		fixed sized arrays for page tables, assuming that each process will have a requirement of less than 32K memory,
		with each page being 1K. 

	5. The page table should also have a delimiter indicating its size so that your programs do not access memory beyond 
		the page table limit. 
		
	6. The page table should have all the required fields that may be implemented by bits or character data types.
	
	7. Assume that your system has a total memory of 256K. You will require a frame table, with data required such as
		reference byte and dirty bit. Use a bit vector to keep track of the unallocated frames.

	8. After the resources have been set up, fork a user process at random times (between 1 and 500 milliseconds of your
		logical clock). Make sure that you never have more than 18 user processes in the system. If you already have
		18 processes, do not create any more until some process terminates. 18 processes is a hard limit and you should
		implement it using a #define macro. Thus, if a user specifies an actual number of processes as 30, your hard limit
		will still limit it to no more than 18 processes at any time in the system. 

	9. Your user processes execute concurrently and there is no scheduling performed. They run in a loop constantly 
		till they have to terminate.

	10. oss will monitor all memory references from user processes and if the reference results in a page fault, the process
		will be suspended till the page has been brought in. Implement the memory references through a semaphore for each
		process. 
		
	11. Effectively, if there is no page fault, oss just increments the clock by 10 nanoseconds and sends a signal
		on the corresponding semaphore. In case of page fault, oss queues the request to the device. Each request for disk
		read/write takes about 14ms to be fulfilled. 
		
	12. In case of page fault, the request is queued for the device and the process is suspended as no signal is sent on the semaphore. 
	
	13. The request at the head of the queue is fulfilled once the clock has advanced by disk read/write time since the time the request was
		found at the head of the queue. The fulfillment of request is indicated by showing the page in memory in the page table. 

	14. oss should periodically check if all the processes are queued for device and if so, advance the clock to fulfill the request at the
		head. We need to do this to resolve any possible deadlock in case memory is low and all processes end up waiting

	15. While a page is referenced, oss performs other tasks on the page table as well such as updating the page reference,
		setting up dirty bit, checking if the memory reference is valid and whether the process has appropriate permissions
		on the frame, and so on.
		
	16. When a process terminates, oss should log its termination in the log file and also indicate its effective memory
		access time. oss should also print its memory map every second showing the allocation of frames. You can display
		unallocated frames by a period and allocated frame by a +.
	
	For example at least something like...
		
	Master: P2 requesting read of address 25237 at time xxx:xxx
	Master: Address 25237 in frame 13, giving data to P2 at time xxx:xxx
	Master: P5 requesting write of address 12345 at time xxx:xxx
	Master: Address 12345 in frame 203, writing data to frame at time xxx:xxx
	Master: P2 requesting write of address 03456 at time xxx:xxx
	Master: Address 12345 is not in a frame, pagefault
	Master: Clearing frame 107 and swapping in p2 page 3
	Master: Dirty bit of frame 107 set, adding additional time to the clock
	Master: Indicating to P2 that write has happened to address 03456
	Current memory layout at time xxx:xxx is:
	
				Occupied RefByte DirtyBit
	Frame 0: 	No			0		0
	Frame 1: 	Yes			13		1
	Frame 2: 	Yes			1		0
	Frame 3: 	Yes			120		1

	where Occupied indicates if we have a page in that frame, the refByte is the value of our reference bits in the frame
		and the dirty bit indicates if the frame has been written to.
```

```
=====User Processes=====
	1. Each user process generates memory references to one of its locations. 
	
	2 This will be done by generating an actual byte address, from 0 to the limit of the process memory. 
		In addition, the user process will generate a random number to indicate whether the memory reference 
		is a read from memory or write into memory. 

	3. This information is also conveyed to oss. The user process will wait on its semaphore that will be signaled 
		by oss. 
	
	4. oss checks the page reference by extracting the page number from the address, increments the clock as specified 
		above, and sends a signal on the semaphore if the page is valid.

	5. At random times, say every 1000 +/- 100 memory references, the user process will check whether it should terminate.
		If so, all its memory should be returned to oss and oss should be informed of its termination.
		
	The statistics of interest are:
		- Number of memory accesses per second
		- Number of page faults per memory access
		- Average memory access speed
		
	6. Make sure that you have signal handling to terminate all processes, if needed. In case of abnormal termination, make
		sure to remove shared memory and semaphores.
```

```
=====Requirement=====
I suggest you implement these requirements in the following order:
	1. Get a makefile that compiles two source files, have master allocate shared memory, use it, then deallocate it.
		Make sure to check all possible error returns.
		
	2. Get Master to fork off and exec one child and have that child attach to shared memory and check the clock and
		verify it has correct resource limit. Then test having child and master communicate through message queues.
		Set up pcb and frame table/page tables

	3. Have child request a read/write of a memory address and have master always grant it and log it.
	
	4. Set up more than one process going through your system, still granting all requests.

	5. Now start filling out your page table and frame table; if a frame is full, just empty it (indicating in the process
		that you took it from that it is gone) and granting request.

	6. Implement a wait queue for I/O delay on needing to swap a process out.

	7. Do not forget that swapping out a process with a dirty bit should take more time on your device
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
	- ./oss -l : the log file used (default is fifolog.dat).
	- ./oss -d : turn on debug mode (default is off). Beware, result will be different and inconsistent.
	- ./oss -t : display result in the terminal as well (default is off).
	- ./oss -a : 0 is using FIFO algorithm, while 1 is using LRU algorithm (default is 0).
--------------------------------------------------------------------------------
##### CHANGELOG:
- Date:  Sun Nov 24 15:51:14 2019 -0600
    - Cleaning up and possibly final update.


- Date:  Sun Nov 24 13:56:11 2019 -0600
    - Fixing rare bug.


- Date:  Sun Nov 24 03:41:47 2019 -0600
    - Update...


- Date:  Sat Nov 23 22:28:05 2019 -0600
    - LRU algorithm update.


- Date:  Sat Nov 23 07:23:24 2019 -0600
    - Update display.


- Date:  Fri Nov 22 19:51:09 2019 -0600
    - Working on FIFO.


- Date:  Fri Nov 22 03:47:02 2019 -0600
    - LinkList update.


- Date:  Fri Nov 22 01:07:58 2019 -0600
    - Prototyping LinkList.


- Date:  Thu Nov 21 02:23:28 2019 -0600
    - More update. Preparing for paging algorithm.


- Date:  Thu Nov 21 01:36:47 2019 -0600
    - Added printWrite() function.


- Date:  Tue Nov 19 18:57:46 2019 -0600
    - Added request, granted request, killing process, and starting FIFO.


- Date:  Mon Nov 18 10:43:12 2019 -0600
    - Update.


- Date:  Sun Nov 17 00:00:47 2019 -0600
    - Init procedure hw6.