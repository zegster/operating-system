/* ====================================================================================================
# Author: Duc Ngo
# Course: CS4760-001 - Operating System
# File Name: dt.c
# Date: 8/28/19
# Last Update: 9/5/19
# Purpose: 
	The goal of this homework is to become familiar with the environment in hoare while practicing 
	system calls. We will be using getopt and perror.
	
	The programming task requires you to create a utility to traverse a specified directory in 
	depth-first order. Depth-first search explores each branch of a tree to its leaves before 
	looking at other branches.
==================================================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>

//Function Prototype
void lookfirst(int *returnsize, char *dir, int level, int indentation, bool symlink);
char * formatdate(char *str, size_t size, time_t val);
void depthfirst(char *dir, int level, int width, int indentation, bool symlink, char *options);

/* ====================================================================================================
MAIN
==================================================================================================== */
int main(int argc, char *argv[])
{
	//Optional Variables
	int indentation_space = 4;
	bool follow_symlink = false;
	char option_string[10];

	//Options
	int opt;
	while((opt = getopt(argc, argv, "hI:Ldgipstul")) != -1)
	{
		switch(opt)
		{
			case 'h':
				printf("NAME:\n");
				printf("	%s - traverse a specified directory in depth-first order.\n", argv[0]);
				printf("\nUSAGE:\n");
				printf("	%s dt [-h] [-I n] [-L -d -g -i -p -s -t -u | -l] [dirname].\n", argv[0]);
				printf("\nDESCRIPTION:\n");
				printf("	-h	: Print a help message and exit.\n");
				printf("	-I n	: Change indentation to n spaces for each level.\n");
				printf("	-L	: Follow symbolic links, if any. Default will be to not follow symbolic links.\n");
				printf("	-d	: Show the time of last modification.\n");
				printf("	-g	: Print the GID associated with the file.\n");
				printf("	-i	: Print the number of links to file in inode table.\n");
				printf("	-p	: Print permission bits as rwxrwxrwx.\n");
				printf("	-s	: Print the size of file in bytes.\n");
				printf("	-t	: Print information on file type.\n");
				printf("	-u	: Print the UID associated with the file.\n");
				printf("	-l	: Print information on the file as if the options \"tpiugs\" are all specified.\n\n");
				return EXIT_SUCCESS;

			case 'I':
				indentation_space = atoi(optarg);
				break;
			
			case 'L':
				follow_symlink = true;
				break;
				
			case 'd':
				strcat(option_string, "d");
				break;
				
			case 'g':
				strcat(option_string, "g");
				break;
				
			case 'i':
				strcat(option_string, "i");
				break;
				
			case 'p':
				strcat(option_string, "p");
				break;
				
			case 's':
				strcat(option_string, "s");
				break;
				
			case 't':
				strcat(option_string, "t");
				break;
				
			case 'u':
				strcat(option_string, "u");
				break;
				
			case 'l':
				strcat(option_string, "tpiugs");
				break;

			default:
				fprintf(stderr, "%s: Please use \"-h\" option for more info.\n", argv[0]);
				return EXIT_FAILURE;
		}
	}

	//If no directory were given, default to current working directory.
	char *topdir, *targetdir, current[2]=".";
    	if(argv[optind] == NULL)
	{
		char origin[4096];
        	getcwd(origin, sizeof(origin));
        	topdir = origin;
		targetdir = current;
    	}
	else
	{
        	topdir=argv[optind];
		targetdir = topdir;
	}

	//Get longest name in the whole directory
	int longest_name;
	lookfirst(&longest_name, topdir, 0, indentation_space, follow_symlink);

	//Execute depthfirst search traverse directories
	printf("Directory scan of: %s\n", targetdir);
	depthfirst(topdir, 0, longest_name, indentation_space, follow_symlink, option_string);

	return EXIT_SUCCESS;
}

/* ====================================================================================================
lookfirst
	find out the max length for column formatting.
==================================================================================================== */
void lookfirst(int *returnsize, char *dir, int level, int indentation, bool symlink)
{
	DIR *dp;		//A type representing a directory stream.
	struct dirent *entry;	//Contains construct that facilitate directory traversing.
	struct stat filestat;	//Contains construct that facilitate getting infromation about files.
	char *buf;
	size_t size;
	int max_length = 0;

	//The number of indentation space when outputing directory information.
	int spaces = level * indentation;

	//Check if opendir able to open a directory stream corresponding to the the directory name.
	if((dp = opendir(dir)) == NULL)
	{
		return;
	}

	//System call: change the current working directory.
	chdir(dir);

	//Get path of the current directory
	char cwd[4096];
	getcwd(cwd, sizeof(cwd));
	
	//Read a directory and returns a pointer to a dirent structure representing the next directory entry in the directory stream pointed to by dirp.
	//Returns NULL on reaching the end of the directory stream or if an error occurred.
	while((entry = readdir(dp)) != NULL)
	{
		//Returns information about the attributes of the file named by filename in the structure pointed to by buf.
		lstat(entry->d_name, &filestat);

		//Is it a directory?
		if(S_ISDIR(filestat.st_mode))
		{
			//Found a directory, but ignore . and ..
			if(strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
			{
				continue;
			}

			//Allocate space to find out the longest name in the whole directory
			size = snprintf(NULL, 0, "%*s%s/", spaces, "", entry->d_name);
			buf = (char *)malloc(size + 1);
			snprintf(buf, size + 1, "%*s%s/", spaces, "", entry->d_name);
			max_length = strlen(buf);
			free(buf);

			//Determine the longest name
			if(*returnsize < max_length)
			{
				*returnsize = max_length;
			}

			//Recurse at a new indent level
			lookfirst(returnsize, entry->d_name, level + 1, indentation, symlink);
		}
		else if(S_ISLNK(filestat.st_mode))
                {
			//Only execute if follow symbolic link is true
			if(symlink)
			{
				//Found a directory, but ignore . and ..
				if(strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				{
					continue;
				}

				stat(entry->d_name, &filestat);

				//Allocate space to find out the longest name in the whole directory
				size = snprintf(NULL, 0, "%*s%s/", spaces, "", entry->d_name);
				buf = (char *)malloc(size + 1);
				snprintf(buf, size + 1, "%*s%s/", spaces, "", entry->d_name);
				max_length = strlen(buf);
				free(buf);

				//Determine the longest name
				if(*returnsize < max_length)
				{
					*returnsize = max_length;
				}
				
				//Recurse at a new indent level
				lookfirst(returnsize, entry->d_name, level + 1, indentation, symlink);

				//Go back to previous path
				chdir(cwd);
			}
                }
		else
		{
			//Allocate space to find out the longest name in the whole directory
			size = snprintf(NULL, 0, "%*s%s", spaces, "", entry->d_name);
                        buf = (char *)malloc(size + 1);
                        snprintf(buf, size + 1, "%*s%s", spaces, "", entry->d_name);
			max_length = strlen(buf);
			free(buf);


			//Determine the longest name
			if(*returnsize < max_length)
			{
				*returnsize = max_length;
			}
		}
	}

	//Change the current working directory one level up in hierachy and close directory one level down in hierachy. 
	chdir("..");
	closedir(dp);
}


/* ====================================================================================================
formatdate
	formats the time represented in the structure timeptr according to the formatting rules 
	defined in format and return it as char of string.
==================================================================================================== */
char * formatdate(char *str, size_t size, time_t val)
{
	strftime(str , size, "%b %d, %Y", localtime(&val));
	return str;
}

/* ====================================================================================================
depthfirst
	Explores each branch of a tree to its leaves before looking at other branches. Depth-first
	search is naturally recursive.
==================================================================================================== */
void depthfirst(char *dir, int level, int width, int indentation, bool symlink, char *options)
{
	DIR *dp;		//A type representing a directory stream.
	struct dirent *entry;	//Contains construct that facilitate directory traversing.
	struct stat filestat;	//Contains construct that facilitate getting infromation about files.
	struct group *grp;	//provides a definition, which includes following members: the name of the group, num group ID, and pointers to member names
	struct passwd *pwp;	//provides a definition, which includes following members: user's login name, num user ID, num group ID, initial working directory, and shell

	//The number of indentation space when outputing directory information.
	//Default indentation is 4.
	//Level 0 (root) = 0, level 1 = 4, level 2 = 8, ...
	int spaces = level * indentation;

	//Check if opendir able to open a directory stream corresponding to the the directory name.
	if((dp = opendir(dir)) == NULL)
	{
		fprintf(stderr, "%*sERROR: %s\n", spaces, "", strerror(errno));
		return;
	}
	
	//System call: change the current working directory.
	chdir(dir);

	//Get path of the current directory
	char cwd[4096];
	getcwd(cwd, sizeof(cwd));

	//Read a directory and returns a pointer to a dirent structure representing the next directory entry in the directory stream pointed to by dirp.
	//Returns NULL on reaching the end of the directory stream or if an error occurred.
	while((entry = readdir(dp)) != NULL)
	{
		//Returns information about the attributes of the file named by filename in the structure pointed to by buf.
		//lstat function is like stat, except that it does not follow symbolic links.
		lstat(entry->d_name, &filestat);

		//Reference: https://en.wikibooks.org/wiki/C_Programming/POSIX_Reference/sys/stat.h
		//Get permission bit and store it into array of char for later use.
		char permission_bit[11] = "";

		//Check to see if it a file, directory, or link list.
		if(S_ISDIR(filestat.st_mode))
		{
			strcat(permission_bit, "d");
		}
		else if(S_ISLNK(filestat.st_mode))
		{
			strcat(permission_bit, "l");
		}
		else if(S_ISREG(filestat.st_mode))
		{
			strcat(permission_bit, "-");
		}

		//Check to see if owner can: read, write, execute
		(filestat.st_mode & S_IRUSR) ? strcat(permission_bit, "r") : strcat(permission_bit, "-");
		(filestat.st_mode & S_IWUSR) ? strcat(permission_bit, "w") : strcat(permission_bit, "-");
		(filestat.st_mode & S_IXUSR) ? strcat(permission_bit, "x") : strcat(permission_bit, "-");

		//Check to see if group can: read, write, execute
		(filestat.st_mode & S_IRGRP) ? strcat(permission_bit, "r") : strcat(permission_bit, "-");
		(filestat.st_mode & S_IWGRP) ? strcat(permission_bit, "w") : strcat(permission_bit, "-");
		(filestat.st_mode & S_IXGRP) ? strcat(permission_bit, "x") : strcat(permission_bit, "-");

		//Check to see if other can: read, write, execute
		(filestat.st_mode & S_IROTH) ? strcat(permission_bit, "r") : strcat(permission_bit, "-");
		(filestat.st_mode & S_IWOTH) ? strcat(permission_bit, "w") : strcat(permission_bit, "-");
		(filestat.st_mode & S_IXOTH) ? strcat(permission_bit, "x") : strcat(permission_bit, "-");

		//Determine the size of a directory/file for later use.
		long long int byte = (long long)filestat.st_size;
		char *sizesuffix = " ";
		if(byte >= 1000000000)
		{
			byte = (long long)(byte/1000000000);
			sizesuffix = "G";
		}
		else if(byte >= 1000000)
		{
			byte = (long long)(byte/1000000);
			sizesuffix = "M";
		}
		else if(byte >= 1000)
		{
			byte = (long long)(byte/1000);
			sizesuffix = "K";
		}

		//Determine the file type.
		char *filecategory = "";
		if(S_ISDIR(filestat.st_mode))
		{
			filecategory = "directory";
		}
		else if(S_ISLNK(filestat.st_mode))
		{
			filecategory = "symlink";
		}
		else
		{
			filecategory = "file";
		}
	
		//Find out GID and UID
		grp = getgrgid(filestat.st_gid);
		pwp = getpwuid(filestat.st_uid);

		//Found a directory, but ignore . and ..
		if(strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
		{
			continue;
		}

		//Print out if it directory, symlink, or file
		if(S_ISDIR(filestat.st_mode))
		{
			printf("%*s%s/%*s", spaces, "", entry->d_name, 20 - spaces - (int)strlen(entry->d_name) + width, "");
		}
		else if(S_ISLNK(filestat.st_mode))
		{
			printf("%*s%s/%*s", spaces, "", entry->d_name, 20 - spaces - (int)strlen(entry->d_name) + width, "");
		}
		else
		{
			printf("%*s%s%*s", spaces, "", entry->d_name, 21 - spaces - (int)strlen(entry->d_name) + width, "");
		}

		//Print out attribute of the directory, symlink, or file
		int i;
		char date[20];
		for(i = 0; i < strlen(options); i++)
		{
			switch(options[i])
			{
				case 'd':
					printf("[%s]  ", formatdate(date, sizeof(date), filestat.st_mtime));
					break;

				case 'g':
					if(grp != NULL)
					{
						printf("[%*.8s]  ", 8, grp->gr_name);
					}
					else
					{
						printf("[%*d]  ", 8, filestat.st_gid);
					}
					break;

				case 'i':
					printf("[%*d]  ", 3, (unsigned int)filestat.st_nlink);
					break;

				case 'p':
					printf("[%s]  ", permission_bit);
					break;

				case 's':
					printf("[%*lld%s]  ", 4, byte, sizesuffix);
					break;

				case 't':
					printf("[%*s]  ", 9, filecategory);
					break;

				case 'u':
					if(pwp != NULL)
					{
						printf("[%*.8s]  ", 8, pwp->pw_name);
					}
					else
					{
						printf("[%*d]  ", 8, filestat.st_uid);
					}
					break;

				default:
					return;
                                }
                        }
                        printf("\n");


		//Is it a directory? link? or file?
		if(S_ISDIR(filestat.st_mode))
		{
			//Recurse at a new indent level
			depthfirst(entry->d_name, level + 1, width, indentation, symlink, options);
		}
		else if(S_ISLNK(filestat.st_mode))
		{
			//Follow it if symbolic link is true
			if(symlink)
			{
				//Returns information about the attributes of the file named by filename in the structure pointed to by buf.
				stat(entry->d_name, &filestat);

				//Recurse at a new indent level
				depthfirst(entry->d_name, level + 1, width, indentation, symlink, options);

				//Go back to previous path
				chdir(cwd);
			}
		}
	}

	//Change the current working directory one level up in hierachy and close directory one level down in hierachy. 
	chdir("..");
	closedir(dp);
}

