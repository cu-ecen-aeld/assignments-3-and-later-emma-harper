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
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>
#include <sys/time.h>
#include <poll.h>

#define DATA_FILE "/var/tmp/aesdsocketdata"
#define MAX_NUM_CONNECTIONS 10
#define BUF_SIZE 1024
#define PORT_NUM "9000"
//static int client_fd = -1; 
static int server_fd = -1;
static struct addrinfo  *new_addr_info;
static pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

//pthread_t num_threads = 1;

bool sig_caught = false;

static void* run_socket_comm(void* thr_params);

typedef struct {
    pthread_t thread_id; 
    int client_fd;
    struct sockaddr_in addr; 
    pthread_mutex_t* mutex;
    bool done_flag;

} client_thread_params; 

//Linked list node
struct slist_data_s{
    client_thread_params   thread_params;
    SLIST_ENTRY(slist_data_s) entries;
};

typedef struct slist_data_s slist_data_t;


void shutdown_process(){

    // if(client_fd > -1){
    //     shutdown(client_fd, SHUT_RDWR);// secondly, terminate the 'reliable' delivery
    //     close(client_fd);
    // }

    if(server_fd > -1){
        shutdown(server_fd, SHUT_RDWR); // secondly, terminate the 'reliable' delivery

        close(server_fd);
    }

    pthread_mutex_destroy(&mutex_lock);
   // unlink(DATA_FILE);
  // check if file exists and if so, delete it
    // if (access(DATA_FILE, F_OK) == 0)  {
    //    remove(DATA_FILE);
    // }

    closelog();

}

static void sig_handler(int sig){
    syslog(LOG_INFO, "Signal Caught %d\n\r", sig);
    //printf("signal caughtt\n");
    sig_caught = true;
    //sleep(1);
    //shutdown_process();
\
}

// static inline void timespec_add( struct timespec *result,
//                         const struct timespec *ts_1, const struct timespec *ts_2)
// {
//     result->tv_sec = ts_1->tv_sec + ts_2->tv_sec;
//     result->tv_nsec = ts_1->tv_nsec + ts_2->tv_nsec;
//     if( result->tv_nsec > 1000000000L ) {
//         result->tv_nsec -= 1000000000L;
//         result->tv_sec ++;
//     }
// }

// static void timer_thread(union sigval sigval)
// {
//     //int *fd = (int*) sigval.sival_ptr;
//     struct tm *time_tm;

//     char format[100];

//     time_t time_stamp;
//     time(&time_stamp);
    
//     time_tm = localtime(&time_stamp);

//     memcpy(format, "",100);

//     size_t time_size = strftime(format,100,"timestamp: %a, %d %b %Y %T %z\n", time_tm);


//     // //get file opened for writing
//     int fd = open(DATA_FILE,O_RDWR | O_APPEND, 0766);
//     if (fd < 0)
//         syslog(LOG_ERR, "error opening file errno is %d\n\r", errno);

//     int ret = pthread_mutex_lock(&mutex_lock);
//     if(ret){
//         close(fd);
//         return;
//     }
//     lseek(fd, 0, SEEK_END); //need to append to end of file

//     int write_bytes = write(fd, format, time_size);
//     syslog( LOG_INFO, "Timestamp %s written to file\n", format);
//     printf("Timestamp %s written to file\n", format);

//     if (write_bytes < 0){
//         syslog(LOG_ERR, "Write of timestamp failed errno %d",errno);
//         printf("Cannot write timestamp to file\n\r");
//     }
    
//     ret = pthread_mutex_unlock(&mutex_lock);
//     if(ret){
//         close(fd);
//         return;
//     }
//     close(fd);
// }

static uint32_t TIMER_INTERVAL_SEC = 10;

void *handle_timer(void *args)
{
  //int *filefd = (int *)args;
  size_t len;
  time_t ts;
  struct tm *localTime;
  struct timespec currTime = {0, 0};
  int count = TIMER_INTERVAL_SEC;

  while (!sig_caught)
  {
    // Get current time
    if (clock_gettime(CLOCK_MONOTONIC, &currTime), 0)
    {
      //log_message(LOG_ERR, "Error: could get monotonic time, [%s]\n", strerror(errno));
      continue;
    }

    if ((--count) <= 0)
    {
        char buf[100] = {0};
        time(&ts);         // Get the timestamp
        localTime = localtime(&ts); // Convert to local time
        len = strftime(buf, 100, "timestamp:%a, %d %b %Y %T %z\n", localTime);

        //log_message(LOG_DEBUG, "%s", buf);

    // //get file opened for writing
        int fd = open(DATA_FILE,O_RDWR | O_APPEND, 0766);
        if (fd < 0)
            syslog(LOG_ERR, "error opening file errno is %d\n\r", errno);

        int ret = pthread_mutex_lock(&mutex_lock);
        if(ret){
            syslog(LOG_ERR, "Could not lock mutex\n\r");
            close(fd);
        }
        lseek(fd, 0, SEEK_END); //need to append to end of file

        int write_bytes = write(fd, buf, len);
        syslog( LOG_INFO, "Timestamp %s written to file\n", buf);
        printf("Timestamp %s written to file\n", buf);

        if (write_bytes < 0){
            syslog(LOG_ERR, "Write of timestamp failed errno %d",errno);
            printf("Cannot write timestamp to file\n\r");
        }
        
        ret = pthread_mutex_unlock(&mutex_lock);
        if(ret){
            syslog(LOG_ERR, "Could not unlock mutex\n\r");
            close(fd);
        }
        close(fd);
        count = TIMER_INTERVAL_SEC;
    }

    currTime.tv_sec += 1; 
    currTime.tv_nsec += 1000000;
    if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &currTime, NULL) != 0)
    {
      // The sleep was interrupted by a signal handler; see signal(7).
      if (errno == EINTR)
        break; // Exit thread

      //log_message(LOG_ERR, "Error: could not sleep, [%s]\n", strerror(errno));
    }     
  }

  //log_message(LOG_INFO, "<<< Timer thread done >>>\n");
  pthread_exit(NULL);
}

// static void init_timer(timer_t* timer_id){

//     struct sigevent sig;
//     memset(&sig, 0, sizeof(struct sigevent));

//     sig.sigev_notify = SIGEV_THREAD;
//     //sig.sigev_value.sival_ptr = fd;
//     sig.sigev_notify_function = timer_thread;

//     int res = timer_create(CLOCK_MONOTONIC, &sig, timer_id);
//     if(res){
//         syslog(LOG_ERR, "ERROR - creating timer failed\n\r");
//         return;
//     }

//     struct timespec start_time = {0};

//     res = clock_gettime(CLOCK_MONOTONIC, &start_time);
//     if(res){
//         syslog(LOG_ERR, "Error getting clock time errno %d\n\r", errno);
//         return;
//     }

//     struct itimerspec itimer;
//     itimer.it_interval.tv_sec = 10;
//     itimer.it_interval.tv_nsec = 1000000; //check this

//     timespec_add(&itimer.it_value, &start_time, &itimer.it_interval);

//     res = timer_settime(*timer_id, TIMER_ABSTIME, &itimer, NULL);
//     if(res){
//         syslog(LOG_ERR, "error seting time back to start time %d\n\r", errno);
//         return;
//     }

//     return;

// }


int main(int argc, char **argv) {

    openlog("aesdsocket", 0, LOG_USER);
    
    sig_t res = signal(SIGINT, sig_handler);
    if (res == SIG_ERR) {
        syslog(LOG_ERR, "ERROR: could not register SIGINT, error SIG_ERR\n\r");
        shutdown_process();
        exit(1);
    }

    res = signal(SIGTERM, sig_handler);
    if (res == SIG_ERR) {
        syslog(LOG_ERR, "ERROR: could not register SIGTERM, error SIG_ERR\n\r");
        shutdown_process();
        exit(1);
    }

    bool daemonize = false;
    //see if need to daemonize
    if (argc >= 2) {
        if (!strcmp(argv[1], "-d")) {
            daemonize = true;
        } else {
            printf("Useage incorrect for arg: %s\nUse -d option for daemonize", argv[1]);
            syslog(LOG_ERR, "Useage incorrect for arg: %s\nUse -d option for daemonize", argv[1]);

             exit(1);
        }
    }
    //open or create file
    int write_fd = creat(DATA_FILE, 0766);
    if(write_fd < 0){
        syslog(LOG_ERR, "ERROR: aesdsocketdata file could not be opened/created, error number %d", errno);
        shutdown_process();
        exit(1);
    }
    close(write_fd);
  
    slist_data_t *data_ptr = NULL;
    SLIST_HEAD(slisthead, slist_data_s) head;
    SLIST_INIT(&head);

    struct addrinfo addr_hints;

    memset(&addr_hints, 0, sizeof(addr_hints));

    //setup addr info
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_flags = AI_PASSIVE;
    int result = getaddrinfo(NULL, (PORT_NUM), &addr_hints, &new_addr_info);
    if (result != 0) {
        syslog(LOG_ERR, "ERROR in getaddrinfo() %s\n", gai_strerror(result));
        freeaddrinfo(new_addr_info);
        shutdown_process();
        exit(1);
    }

    // Open socket connection
    server_fd = socket(new_addr_info->ai_family, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "ERROR opening socket, error number %d\n", errno);
        freeaddrinfo(new_addr_info);
        shutdown_process();
        exit(1);
    }

   // Set sockopts for reuse of server socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        syslog(LOG_ERR, "ERROR set socket options failed with error number%d\n", errno);
        freeaddrinfo(new_addr_info);
        shutdown_process();
        exit(1);
    }

    // Bind device address to socket
    if (bind(server_fd, new_addr_info->ai_addr, new_addr_info->ai_addrlen) < 0) {
        syslog(LOG_ERR, "ERROR binding socket error num %d\n", errno);
        freeaddrinfo(new_addr_info);
        shutdown_process();
        exit(1);
    }

    freeaddrinfo(new_addr_info);
    
    // Listen for connection
    if (listen(server_fd, MAX_NUM_CONNECTIONS)) {
        syslog(LOG_ERR, "ERROR: listening for connection error num %d\n", errno);
        shutdown_process();
        exit(1);
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
          exit(1);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS); 
        }

        // create new session
        if (setsid() == -1) {
            syslog(LOG_ERR, "setsid error%d\n", errno);
            exit(1);
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

    //timer_t timer_id;

     //   init_timer(&timer_id);
    int dummy_var = 1; 
    pthread_t time_id; 
    pthread_create(&time_id, NULL, handle_timer, (void*)&dummy_var);

    struct pollfd poll_socket[1];
    poll_socket[0].fd = server_fd;
    poll_socket[0].events = POLLIN;
    int poll_time_ms = 100;

    while(!(sig_caught)) {

        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        poll(poll_socket, 1, poll_time_ms);
        if (poll_socket[0].revents != POLLIN)
            continue;

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_size);

        if(sig_caught)
            break;

        if(client_fd < 0){
            syslog(LOG_ERR, "ERROR: accepting new connection error is %s", strerror(errno));
            shutdown_process();
            exit(1);
        }



        
        char client_info[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_info, INET_ADDRSTRLEN);

        syslog(LOG_INFO, "Accepted connection from %s \n\r",client_info);
        printf("Accepted connection from %s\n\r", client_info);
        
        //allocating new node for the data
        data_ptr = (slist_data_t *) malloc(sizeof(slist_data_t));
        SLIST_INSERT_HEAD(&head,data_ptr,entries);

        data_ptr->thread_params.client_fd = client_fd;
        data_ptr->thread_params.addr = client_addr;
        data_ptr->thread_params.mutex = &mutex_lock;
        data_ptr->thread_params.done_flag = false;

        pthread_create(&(data_ptr->thread_params.thread_id), NULL, run_socket_comm, (void*)&data_ptr->thread_params);

        SLIST_FOREACH(data_ptr,&head,entries){
            
            if(data_ptr->thread_params.done_flag == true){
                pthread_join(data_ptr->thread_params.thread_id,NULL);
                shutdown(data_ptr->thread_params.client_fd, SHUT_RDWR);
                close(data_ptr->thread_params.client_fd);
                syslog(LOG_INFO, "Joining thread with id %d\n\r",(int)data_ptr->thread_params.thread_id);
                printf("Joining thread with id %d\n\r",(int)data_ptr->thread_params.thread_id);

            }
        }



    }

    pthread_join(time_id, NULL);

    //Kill all threads
    while (!SLIST_EMPTY(&head)) {
        //pthread_cancel(data_ptr->thread_params.thread_id);
        data_ptr = SLIST_FIRST(&head);
        syslog(LOG_INFO, "Killing thread %d\n\r", (int) data_ptr->thread_params.thread_id);
        SLIST_REMOVE_HEAD(&head, entries);
        free(data_ptr); 
        data_ptr = NULL;// Free allocate memory
    }

    printf("HERERERER\n\r");
    
    //close(data_fd);
    unlink(DATA_FILE);
    if (access(DATA_FILE, F_OK) == 0)  {
       remove(DATA_FILE);
    }
    shutdown_process();

    exit(0);

}

void* run_socket_comm(void* thr_params){


    char buf[BUF_SIZE];

    client_thread_params* params = (client_thread_params*)thr_params;

    char* thread_buf = (char*)malloc(sizeof(char) * BUF_SIZE);
    if(thread_buf == NULL){
        syslog(LOG_ERR,"Error allocating buffer for thread %d\n\r", (int)(params->thread_id));
        params->done_flag = true; //exit

    } else{
        memset(thread_buf, 0, BUF_SIZE);

    }

    int curr_pos = 0;
    uint32_t buf_space = BUF_SIZE;
    while(!(params->done_flag)){

        int read_bytes = read(params->client_fd, buf, (BUF_SIZE));
        if (read_bytes < 0) {
            syslog(LOG_ERR, "Error: reading from socket errno=%d\n", errno);
            free(thread_buf);
            params->done_flag = true;
            pthread_exit(NULL);
        }

        printf("read %d bytes\n\r", read_bytes);

        if (read_bytes == 0)
            continue; // no bytes to read

        if(read_bytes > (buf_space)){
            thread_buf = (char*)realloc(thread_buf, sizeof(char) * (curr_pos + BUF_SIZE));
            if(thread_buf == NULL){
                syslog(LOG_ERR,"Error allocating buffer for thread %d\n\r", (int)params->thread_id);
                free(thread_buf);
                params->done_flag = true; //exit
                pthread_exit(NULL);
            }

            buf_space += curr_pos;

        }

        memcpy(&thread_buf[curr_pos], buf, read_bytes);

        curr_pos += read_bytes;
        buf_space -= read_bytes;

        if (strchr(buf, '\n')) {  //check if at end of file line
           break; // break out of while 1

        } 

    }

    // //get file opened for writing
    int fd = open(DATA_FILE,O_RDWR | O_APPEND, 0766);
    if (fd < 0)
        syslog(LOG_ERR, "error opening file errno is %d\n\r", errno);

    lseek(fd, 0, SEEK_END); //need to append to end of file
    
    int ret = pthread_mutex_lock(params->mutex);
    if(ret){
        syslog(LOG_ERR, "Mutex cannot be locked\n\r");
        printf("Mutex cannot be locked\n\r");
        free(thread_buf); //free mem
        params->done_flag = true;
        pthread_exit(NULL);
    }

    int write_bytes = write(fd, thread_buf, curr_pos);
    if(write_bytes < 0){
        syslog(LOG_ERR, "Error writing to file errno is %d\n\r", errno);
        free(thread_buf); //free mem
        params->done_flag = true;
        close(fd);
        pthread_exit(NULL);
    }

    lseek(fd, 0, SEEK_SET); //need to append to end of file


    ret = pthread_mutex_unlock(params->mutex);
    if(ret){
        syslog(LOG_ERR, "Mutex cannot be locked\n\r");
        printf("Mutex cannot be locked\n\r");
        free(thread_buf); //free mem
        params->done_flag = true;
        pthread_exit(NULL);
    }

    close(fd);
    printf("wrote %d bytes\n\r", write_bytes);

   // printf("buff val is %x\n",(uint8_t)buf[curr_pos]);

    memset(buf,0, BUF_SIZE);

    
    int read_offset = 0;

    while(1) {

        int fd = open(DATA_FILE, O_RDWR | O_APPEND, 0766);
        if(fd < 0){
            syslog(LOG_ERR, "Error: reading from socket errno=%d\n", errno);
            printf("error is %d\n\r", errno);
            curr_pos = 0; //re read
            continue; 
        }

        lseek(fd, read_offset, SEEK_SET);


        int ret = pthread_mutex_lock(params->mutex);
        if(ret){
            syslog(LOG_ERR, "Mutex cannot be locked\n\r");
            printf("Mutex cannot be locked\n\r");
            free(thread_buf); //free mem
            params->done_flag = true;
            pthread_exit(NULL);
        }
        
        int read_bytes = read(fd, buf, BUF_SIZE);

        ret = pthread_mutex_unlock(params->mutex);   
        if(ret){
            syslog(LOG_ERR, "Mutex cannot be locked\n\r");
            printf("Mutex cannot be locked\n\r");
            free(thread_buf); //free mem
            params->done_flag = true;
            pthread_exit(NULL);
        }
        
        close(fd);
        if(read_bytes < 0){
            syslog(LOG_ERR, "Error reading from file errno %d\n", errno);
            continue;
        }

        if(read_bytes == 0)
            break;

        //write to socket
        int write_bytes = write(params->client_fd, buf, read_bytes );
        printf("wrote %d bytes to client\n\r", write_bytes);

        if(write_bytes < 0){
            printf("errno is %d", errno);

            syslog(LOG_ERR, "Error writing to client fd %d\n", errno);
            continue;
        }

        read_offset += write_bytes;

    }

    free(thread_buf);
    params->done_flag = true;
    pthread_exit(NULL);

}