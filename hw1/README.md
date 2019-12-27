## Homework #1 - Linux System Calls and Library Functions
##### DESCRIPTION:
The goal of this homework is to become familiar with the environment in hoare while practicing system calls. We will be using ***getopt*** and ***perror***. The programming task requires you to create a utility to traverse a specified directory in depth first order. Depth-first search explores each branch of a tree to its leaves before looking at other branches. Depth-first search is naturally recursive, as indicated by following pseudocode:

```
depthfirst ( root )
{
	for each node at or below root
	{
		visit node;
			if node is a directory
				depthfirst ( node );
	}
}
```

The indentation of the filenames shows the level in the file system tree. A default indentation of 4 spaces for each level in the directory. You should be able to change the indentation spaces by using an option on command line followed by a number. The executable should be called dt. The program will be invoked by:
```
dt [-h] [-I n] [-L -d -g -i -p -s -t -u | -l] [dirname]
```
|Option   |The options are to be interpreted as follows
|:-------:|--------------------------------------------
|h        | Print a help message and exit.
|I n      | Change indentation to n spaces for each level.
|L        | Follow symbolic links, if any. Default will be to not follow symbolic links.
|t        | Print information on file type.
|p        | Print permission bits as rwxrwxrwx.
|i        | Print the number of links to file in inode table.
|u        | Print the UID associated with the file.
|g        | Print the GID associated with the file.
|s        | Print the size of file in bytes. If the size is larger than 1K, indicate the size in KB with a suffix K; if the size is larger than 1M, indicate the size in MB with a suffix M; if the size is larger than 1G, indicate the size in GB with a suffix G.
|d        | Show the time of last modification.
|l        | This option will be used to print information on the file as if the options **tpiugs** are all specified.

If the user does not specify dirname, run the command using current directory and print the tree accordingly.

With the use of **perror** with some meaningful error messages. The format for error messages should be:

	dt: Error: Detailed error message

Where dt is actually the name of the executable (argv[0]) and should be appropriately modified if the name of executable is changed without recompilation. These error messages should be sent to **stderr** using **perror**.

It is required for this project that you use **version control**, a **Makefile**, and a **README**. Your README file should consist at a minimum of a description of how I should compile and run your project, any outstanding problems that it still has, and any problems you encountered. Your Makefile should use suffix-rules or pattern-rules and have an option to clean up object files.

--------------------------------------------------------------------------------
##### HOW TO RUN:
1. In your command promt, type: make
2. This will generate .o file and executable file
3. To use the program, type: ./dt [dirname]
4. You then will see the list of directory and file in your current working directory or selected
	path directory.
5. For more option with this program, type: ./dt -h
6. Few example:
    - ./dt -L [dirname] : follow symbolic link in the path that was given.
    - ./dt -u [dirname] : display uid of the directory, file, and symbolic link.
    - ./dt -d [dirname] : display the last modification date of the directory, file, and symbolic link.
--------------------------------------------------------------------------------
##### CHANGELOG: 
- Date: Thu Sep 5 18:47:59 2019
    - Remove and clean up redundancy in code to make it more readable.
    - Found and fixed a bug in the dynamic width column.
	- Was displaying invalid character and column was misalign when encountering invalid character.


- Date:   Tue Sep 3 19:17:35 2019 -0500
    - Fix dynamic column display bug.
	- Fix the issue of not getting the corrected longest name size.
    - Modified gid and uid to display as text instead of numerical.
    - Modified inode to display hard link instead.
    - More cleaning up and formatting for readability.


- Date: Mon Sep 2 02:19:31 2019 -0500
    - Added gid, inode, filesize, filetype, and uid option.
    - More cleaning up for readability.


- Date: Sun Sep 1 22:10:01 2019 -0500
    - Fix traversal symbolic link bug.
	- Misused lstat and stat which causing annoying bug (read second bullet point).
	- There was an issue of traversing through symbolic link. The user was able go to the path, but was unable
		traverse back to the origin of the directory.
    - Updated dynamic column display.
    - Added modification date option.


- Date: Sun Sep 1 01:18:16 2019 -0500
    - Added dummy symbolic link.
    - Added useful comments for references.
    - Implemented indentation and permission options.
    - Prototyping dynamic column display.
	- Display the output into nicely column no matter what the longest name of the directory/file/symbolic link.
    - Clean up the file to make it more readable.


- Date: Fri Aug 30 16:57:15 2019 -0500
    - Added hw1 folder and move dt.c to hw1.
    - Created a dummy test directory.
	- Simple test directory to test out the program.
    - Created a makefile with suffix rule.
    - Added getopt and recursive depthfirst function in [dt.c].
	- First time using dirent.h and stat.h, thus took a bit of time researching and reading documentation.


- Date: Wed Aug 28 22:15:18 2019 -0500
    - Added a new file [dt.c].