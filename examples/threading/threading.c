#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

#define USEC_TO_MSEC 1000


void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data*) thread_param;

    //wait for specified ms
    int ret = usleep(thread_func_args->wait_to_obtain_ms * USEC_TO_MSEC);
    if(ret){
        ERROR_LOG("Failed to sleep for %dms", thread_func_args->wait_to_obtain_ms * USEC_TO_MSEC);
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }
    
    //lock mutex
    ret = pthread_mutex_lock(thread_func_args->mutex);
    if(ret){
        ERROR_LOG("Failed to lock mutex");
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }

    //wait for specified ms
    ret = usleep(thread_func_args->wait_to_release_ms * USEC_TO_MSEC);
    if(ret){
        ERROR_LOG("Failed to sleep for %dms", thread_func_args->wait_to_release_ms * USEC_TO_MSEC);
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }
    
    //unlock mutex
    ret = pthread_mutex_unlock(thread_func_args->mutex);
    if(ret){
        ERROR_LOG("Failed to unlock mutex");
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }
    thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
    struct thread_data* threadParams = (struct thread_data*)malloc(sizeof(struct thread_data));

    if(threadParams == NULL){
        ERROR_LOG("Could not allocate memory for thread data");
        return false;
    }

    threadParams->mutex = mutex;
    threadParams->wait_to_obtain_ms = wait_to_obtain_ms;
    threadParams->wait_to_release_ms = wait_to_release_ms;

    int ret = pthread_create(thread, NULL, &threadfunc, threadParams);
    if(ret){
      ERROR_LOG("Failed to create new thread");
      return false;
    }
    //thread.join();
    return true;

}

