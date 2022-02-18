/**
 * aesdsocket.c
 * Author: Emma Harper
 * Date Created: 2/12/22
 *
 */

#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define DATA_FILE "/var/tmp/aesdsocketdata"
#define MAX_NUM_CONNECTIONS 10
#define BUF_SIZE 1024
#define PORT_NUM "9000"
static int client_fd = -1; 
static int server_fd = -1;
static struct addrinfo  *new_addr_info;




void shutdown_process(){

    if(client_fd > -1){
        close(client_fd);
    }

    if(new_addr_info != NULL)
        freeaddrinfo(new_addr_info);

    if(server_fd > -1){
        close(server_fd);
    }

    unlink(DATA_FILE);
  // check if file exists and if so, delete it
    if (access(DATA_FILE, F_OK) == 0)  {
       remove(DATA_FILE);
    }

    closelog();

}

static void sig_handler(int sig){
    syslog(LOG_INFO, "Signal Caught %d\n\r", sig);
    //printf("signal caughtt\n");
    shutdown_process();
    exit(0);

}

int main(int argc, char **argv) {

    openlog("aesdsocket", 0, LOG_USER);
    
    sig_t res = signal(SIGINT, sig_handler);
    if (res == SIG_ERR) {
        syslog(LOG_ERR, "ERROR: could not register SIGINT, error SIG_ERR\n\r");
        shutdown_process();
        return -1;
    }

    res = signal(SIGTERM, sig_handler);
    if (res == SIG_ERR) {
        syslog(LOG_ERR, "ERROR: could not register SIGTERM, error SIG_ERR\n\r");
        shutdown_process();
        return -1;
    }

    bool daemonize = false;
    //see if need to daemonize
    if (argc >= 2) {
        if (!strcmp(argv[1], "-d")) {
            daemonize = true;
        } else {
            printf("Useage incorrect for arg: %s\nUse -d option for daemonize", argv[1]);
            syslog(LOG_ERR, "Useage incorrect for arg: %s\nUse -d option for daemonize", argv[1]);

             return (-1);
        }
    }
    //open or create file
    int write_fd = creat(DATA_FILE, 0766);
    if(write_fd < 0){
        syslog(LOG_ERR, "ERROR: aesdsocketdata file could not be opened/created, error number %d", errno);
        shutdown_process();
        return -1;
    }
    close(write_fd);
  

    struct addrinfo addr_hints;

    memset(&addr_hints, 0, sizeof(addr_hints));

    //setup addr info
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_flags = AI_PASSIVE;
    int result = getaddrinfo(NULL, (PORT_NUM), &addr_hints, &new_addr_info);
    if (result != 0) {
        syslog(LOG_ERR, "ERROR in getaddrinfo() %s\n", gai_strerror(result));
        shutdown_process();
        return -1;
    }

    // Open socket connection
    server_fd = socket(new_addr_info->ai_family, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "ERROR opening socket, error number %d\n", errno);
        shutdown_process();
        return -1;
    }

   // Set sockopts for reuse of server socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        syslog(LOG_ERR, "ERROR set socket options failed with error number%d\n", errno);
        shutdown_process();
        return -1;
    }

    // Bind device address to socket
    if (bind(server_fd, new_addr_info->ai_addr, new_addr_info->ai_addrlen) < 0) {
        syslog(LOG_ERR, "ERROR binding socket error num %d\n", errno);
        shutdown_process();
        return -1;
    }


    // Listen for connection
    if (listen(server_fd, MAX_NUM_CONNECTIONS)) {
        syslog(LOG_ERR, "ERROR: listening for connection error num %d\n", errno);
        shutdown_process();
        return -1;
    }

    printf("Listening for connections\n\r");

    if (daemonize == true) {
        // ignore signals
        signal(SIGCHLD, SIG_IGN);
        signal(SIGHUP, SIG_IGN);
        // create process
        pid_t pid = fork();
        if (pid < 0) {
          shutdown_process();
          exit(-1);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS); 
        }

        // create new session
        if (setsid() == -1) {
            syslog(LOG_ERR, "setsid error%d\n", errno);
            exit(-1);
        }
        // change dir to root dir
        chdir("/");

        //redirect to /dev/null
        int devnull = open("/dev/null", O_RDWR);
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);
      }

    while(1) {

        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if(client_fd < 0){
            syslog(LOG_ERR, "ERROR: accepting new connection error is %s", strerror(errno));
            shutdown_process();
            return -1;
        }

        char client_info[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_info, INET_ADDRSTRLEN);

        syslog(LOG_INFO, "Accepted connection from %s \n\r",client_info);
        printf("Accepted connection from %s\n\r", client_info);
        char buf[BUF_SIZE];
        int curr_pos = 0;

        while(1){
            int read_bytes = read(client_fd, buf, (BUF_SIZE));
            if (read_bytes < 0) {
                syslog(LOG_ERR, "Error: reading from socket errno=%d\n", errno);
                curr_pos = 0; //re read
                continue; 
            }
            if (read_bytes == 0)
                continue; // no bytes to read

            printf("read %d bytes\n\r", read_bytes);
            //get file opened for writing
            int fd = open(DATA_FILE,O_RDWR | O_CREAT | O_APPEND, 0766);
            if (fd < 0)
                syslog(LOG_ERR, "error opening file errno is %d\n\r", errno);

            lseek(fd, 0, SEEK_END); //need to append to end of file

            int write_bytes = write(fd, &buf[curr_pos], read_bytes);
            if(write_bytes < 0){
                syslog(LOG_ERR, "Error writing to file errno is %d\n\r", errno);
                close(fd);
                continue; // read again
            }

            close(fd);
            printf("wrote %d bytes\n\r", write_bytes);

           // printf("buff val is %x\n",(uint8_t)buf[curr_pos]);


            if (strchr(&buf[curr_pos], '\n')) {  //check if at end of file line
               break; // break out of while 1

            } 
            if(read_bytes == write_bytes){

                curr_pos = 0;

            } else {
                printf("here1\n\r");

                curr_pos += read_bytes;
            }

        }
        curr_pos = 0;
        int read_offset = 0;

        while(1) {

            int fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0766);
            if(fd < 0){
                syslog(LOG_ERR, "Error: reading from socket errno=%d\n", errno);
                printf("error is %d\n\r", errno);
                curr_pos = 0; //re read
                continue; 
            }

            lseek(fd, read_offset, SEEK_SET);
            int read_bytes = read(fd, buf, (BUF_SIZE - curr_pos));
            
            close(fd);
            if(read_bytes < 0){
                syslog(LOG_ERR, "Error reading from file errno %d\n", errno);
                continue;
            }

            if(read_bytes == 0)
                break;

            int write_bytes = write(client_fd, &buf[curr_pos], read_bytes );
            printf("wrote %d bytes to client\n\r", write_bytes);

            if(write_bytes < 0){
                printf("errno is %d", errno);

                syslog(LOG_ERR, "Error writing to client fd %d\n", errno);
                continue;
            }


            if((write_bytes) == read_bytes){
                curr_pos = 0;

            } else{
                curr_pos += write_bytes;

            }
            read_offset += write_bytes;

        }

        close(client_fd);
        client_fd = -1;
        syslog(LOG_INFO, "CLOSING CLIENT SOCKET\n\r");
    }

    shutdown_process();
    return 0;

}