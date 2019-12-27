## Homework 2 - Concurrent UNIX Processes and shared memory
##### DESCRIPTION:
The goal of this homework is to become familiar with using shared memory and creating multiple processes. 
We will be using getopt and perror as well as fork().

In this project, you will solve a number of subset sum problems, with each child solving one instance of the problem.
A simple definition of the subset sum problem is: Given a set of integers and a value sum, determine if there is a
subset of the given set with sum equal to the given sum.

Your project should consist of one program, which will fork off versions of itself to do some file processing. To do
this, it will start by taking some command line arguments. Your executable should be called logParse. You must
implement at least the following command line arguments using getopt:

```
-h                 : should display all legal command line options and how it is expected to run, as well as the default behavior.
-i inputfilename   : if input filenames are not specified, the defaults should be input.dat.
-o outputfilename  : if output filenames are not specified, the defaults should be output.dat.
-t time            : time specifies the maximum duration the code should run (default 10 seconds).
```

Once you have parsed the command line arguments and validated them, then you should attempt to open the input file. It will start with a number on a line by itself, with that number indicating the amount of subtasks your program will have to solve using copies of your process created with fork. Every other line of the file will contain a subtask, which will consist of a list of integers. An example of this input file is below:
```
3
9 3 34 4 12 5 2
6 3 2 7 1
256 3 5 7 8 9 1
```
1. Your data fille has the number of problems to be solved on the first line (3 in the example), and one instance of the
problem on each subsequent line.

2. The original process should read the first line of the file. Once it has read that line, it should then go into a loop
based on that number, with each iteration of the loop forking off a copy that will then process the next line. 

3. Once that child has finished its work (defined below), it will write some data to the output file and then terminate. 
At that point, the parent detects that its child has terminated, and it should initiate another iteration of the 
loop until all instances have been solved. 

4. After all children have terminated. the parent should write the pids of all of its children that it launched, 
as well as its own pid to the output file.

When a child process starts, it should read the next line of the file. We see in our example file that the first forked
child would read the line with 7 numbers.

The task the child must complete with these numbers is the subset sum problem. In particular, your process must
find if any subset of numbers in that list sum to the first number. If it does, it should output the set of numbers that
sums to that value to the output file, starting with its pid.

Code to do the subset sum problem can be found at: https://www.geeksforgeeks.org/subset-sum-problem-dp-25/

###### For example:
Given the first subtask in the above code, possible output might be:
```
13278: 4 + 5 = 9
```
	
The set of numbers might not contain a set of numbers that sums to the specified number (example 3 above). 
If the numbers are small, this might be quickly discovered. In that case, output as follows:
```
13278: No subset of numbers summed to 256.
```

However, given a sufficiently large list, this might take too long for our needs. I want your individual child processes
to give up after 1 second of clock time. In other words, after 1 second of elapsed time since the child started, it
should output to the file the following:
```
13278: No valid subset found after 1 second.
```

In addition to the above time limit, your overall project should have a maximum duration of t seconds (default 10 seconds).
This should be handled through a timed interrupt signal sent to your master process. If your master process receives this signal, 
it should terminate all children, close all files and exit. You can find an example on how to do this in our 
unix textbook, section 9.3, Program 9.7, periodicasterik.c

Please make any error messages meaningful. The format for error messages should be: logParse: Error: Detailed error message
- Where logParse is actually the name of the executable (argv[0]) that you are trying to execute. 
- These error messages should be sent to stderr using perror.

--------------------------------------------------------------------------------
##### HOW TO RUN:
1. In your command promt, type: make
2. This will generate .o file and executable file
3. To use the program, type: ./logParse
4. You then will solve a number of subset sum problems, with each child solving one 
	instance of the problem.
5. For more option with this program, type: ./logParse -h
6. Few examples:
    - ./logParse -h : print out the help page and exit.
    - ./logParse -i : specifies the input file (default is input.dat)
    - ./logParse -o : specifies the output file (default is output.dat)
    - ./logParse -t : specifies the maximum duration the code should run (default 10 seconds)
--------------------------------------------------------------------------------
##### CHANGELOG: 
- Date: Tue Sep 17 16:39:19 2019 -0500
    - Remove and clean up redundancy in code to make it more readable.


- Date: Tue Sep 17 13:49:52 2019 -0500
    - Continue fixing signal handling.


- Date: Tue Sep 17 03:31:05 2019 -0500
    - Fixing up signal handling.
    - Edit README.


- Date: Sat Sep 14 22:51:12 2019 -0500
    - Added timer function.
    - Added parent interrupt procedure.
    - Added child interrupt procedure.
    - Prototyping signal handling.
    - Modified README.
    - Added more useful comments for the reader and the past me (in case I need references).


- Date: Fri Sep 13 03:34:20 2019 -0500
    - Added fork and get it to working.
    - Added child process task.
    	- Added find subset sum function.
    		- Write to file when there is a result from subset sum function.
    - Added parent process task.
    	- Store all child PID process.
    - Added output all child PID and parent PID.
    - Added strfcat
    	- strcat with formatting.


- Date: Tue Sep 10 19:10:32 2019 -0500
    - Added Makefile to HW2.
    - Added prototype README to HW2.
    - Added logParse.c to HW2.
    	- Added getopt.