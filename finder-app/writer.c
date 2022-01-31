/**
 * Filename: writer.c
 * Brief: 
 * 
 * Author: Emma Harper
 * Date Created: 01/15/2022
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>

#define NUM_ARGS 3

int main(int argc, char *argv[])
{
	//open new log file with LOG_USER
	openlog(NULL, LOG_CONS, LOG_USER);

	//check arguments
	if(argc < NUM_ARGS)
	{
		syslog(LOG_ERR,"Error: Incorrect number of arguments\n");
		return 1;
	}
	
	//open/create file for read/write and do not allow others to write to file 
	int fd = open(argv[1], O_CREAT | O_RDWR,0644);
	//make sure file desciptor exists
	if(fd == -1) {
		syslog(LOG_ERR,"File %s failed to open/does not exist\n", argv[1]);
		return 1;
	}
	else {
		//attempt to write string into file
		size_t size = strlen(argv[2]);
		int write_count =write(fd, argv[2], size);
		// make sure writing was successful
		if(write_count == -1){
			syslog(LOG_ERR,"Did not complete write\n");
		}
		else if(write_count != strlen(argv[2]))
			syslog(LOG_ERR,"Did not complete write\n");
		else {
			printf("Writing %s to %s\n",argv[2],argv[1]);

			syslog(LOG_DEBUG,"Writing %s to %s",argv[2],argv[1]);
		}
	}

	close(fd);
	return 0;
}