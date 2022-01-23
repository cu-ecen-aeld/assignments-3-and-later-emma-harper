#include "systemcalls.h"
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
*/
    int result = system(cmd);

    if(result <= 0)
        return false; // system call failed
    else
        return true; 
    
    
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/
    int ret = 1; //optimization took out my return false statements
    int result; //changing for return values in functions called below

    openlog(NULL, 0, LOG_USER);

    pid_t pid = fork();
    if( pid == -1){
        syslog(LOG_ERR, "Fork failed, child process did not launch\n\r");

        ret = 0; 

    } else if(!pid){
        //printf("Command 0 is %s\n\r", command[0]);
        result = execv(command[0], command); //pass in path and vector of args to execute process        
        syslog(LOG_ERR, "execv call returned (failed) - child process exited with return %d\n\r", result);
        ret = 0; 
        exit(EXIT_FAILURE);

    } else {
        int status; 
        result = waitpid(pid, &status, 0); 
   
        if(result == -1){
            syslog(LOG_ERR, "waitpid error\n\r");
            ret = 0; 
        }else {
            //taken from LSP book, page 217 (online edition)
            syslog(LOG_DEBUG, "pid=%d\n", pid);

            if (!(WIFEXITED (status))){
                syslog(LOG_DEBUG, "Abnormal termination with exit status=%d\n", WEXITSTATUS(status));
                ret = 0;
            }

            if (WIFSIGNALED (status))
                syslog(LOG_DEBUG, "Killed by signal=%d%s\n", WTERMSIG(status), WCOREDUMP(status) ? " (dumped core)" : "");
            
            if(WEXITSTATUS (status)){
                syslog(LOG_DEBUG, "Exit status=%d\n", WEXITSTATUS(status));
                ret = 0;
            }
        }
    }
    va_end(args);

    return ret; 
}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/
    int ret = 1; //optimization took out my return false statements
    int result; //changing for return values in functions called below
    openlog(NULL, 0, LOG_USER);

    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644 );

    if(fd < 0){
        syslog(LOG_ERR, "Error opening/creating outputfile\n\r");
    }
    
    pid_t pid = fork();
    if( pid == -1){
        syslog(LOG_ERR, "Fork failed, child process did not launch\n\r");

        ret = 0; 

    } else if(!pid){
        if(!(fd < 0)){
            if((dup2(fd, 1) < 0)){
                syslog(LOG_ERR, "dup2 failed to redirect\n\r");
            }
        }

        close(fd);

        result = execv(command[0], command); //pass in path and vector of args to execute process

        syslog(LOG_ERR, "execv call returned (failed) - child process exited with return %d\n\r", result);

        ret = 0;

    }else {
        int status; 

        result = waitpid(pid, &status, 0); 
   
        if(result == -1){
            syslog(LOG_ERR, "waitpid error\n\r");

            ret = 0; 
        }else {
            //taken from LSP book, page 217 (online edition)
            syslog(LOG_DEBUG, "pid=%d\n", pid);

            if (!(WIFEXITED (status))){
                syslog(LOG_DEBUG, "Abnormal termination with exit status=%d\n", WEXITSTATUS(status));
                ret = 0;
            }

            if (WIFSIGNALED (status))
                syslog(LOG_DEBUG, "Killed by signal=%d%s\n", WTERMSIG(status), WCOREDUMP(status) ? " (dumped core)" : "");
            
            if(WEXITSTATUS (status)){
                syslog(LOG_DEBUG, "Exit status=%d\n", WEXITSTATUS(status));
                ret = 0;
            }
        }
    }


    va_end(args);
    
    return ret;
}
    