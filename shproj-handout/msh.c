/* 
 * msh - A mini shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "util.h"
#include "jobs.h"


/* Global variables */
int verbose = 0;            /* if true, print additional output */

extern char **environ;      /* defined in libc */
static char prompt[] = "msh> ";    /* command line prompt (DO NOT CHANGE) */
static struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
void usage(void);
void sigquit_handler(int sig);

/*Declaration of our function*/
void forNotParent(char **cmdline, sigset_t set);



/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
        break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
        break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
        break;
    default:
            usage();
    }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

    /* Read command line */
    if (emit_prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
        app_error("fgets error");
    if (feof(stdin)) { /* End of file (ctrl-d) */
        fflush(stdout);
        exit(0);
    }

    /* Evaluate the command line */
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char* argument[MAXARGS];
    int back = parseline(cmdline, argument);

    sigset_t set, set2;
    pid_t kid;

    if(!builtin_cmd(argument)) {
        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);
        sigprocmask(SIG_SETMASK, &set, &set2);

        kid = fork(); //child made
        
        // child
        if(kid == 0) {
            forNotParent(argument, set2);      
        }
        //parent
        else {
            sigdelset(&set, SIGCHLD);
            if(back == 0) { //foreground
                addjob(jobs, kid, 1, cmdline);
                sigprocmask(SIG_SETMASK, &set2, NULL);
                //gotta bg
                waitfg(kid);
            }
            else { //background
                addjob(jobs, kid, 2, cmdline);
                sigprocmask(SIG_SETMASK, &set2, NULL);
                printf("[%d] (%d) %s", getjobpid(jobs, kid)->jid, kid, cmdline);
            }
            return;
        }
        return;
    }
    else
        return;
    return;
}


/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
int builtin_cmd(char **argv) 
{
    //Paras Driving
    //Tried using switch statements, realized it doesn't work on strings
    //so switched to if statement.
    if(strcmp(argv[0], "quit") == 0) //quit
        exit(0);
    else if(strcmp(argv[0], "fg") == 0) //fg
        do_bgfg(argv);
    else if(strcmp(argv[0], "jobs") == 0) //jobs
        listjobs(jobs);
    else if(strcmp(argv[0], "bg") == 0) //bg
        do_bgfg(argv);
    else if(argv[0] == NULL) //for safety
        return 1; 
    else
        return 0;
    return 1;
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    struct job_t *notPid;
    //to check if background or foreground
    int back = (*argv[0] == 'b');

    if(argv[1] == NULL) {
        if(back)
            printf("bg command requires PID or %%jobid argument\n");
        else if(!back)
            printf("fg command requires PID or %%jobid argument\n");
        else
            return;
    }
    else if((argv[1] != NULL)) {
        if(strchr(argv[1], '%') == NULL) {
            //FOR PID
            pid_t pId = atoi(argv[1]);
            notPid = getjobpid(jobs, pId);

            if(atoi(argv[1]) == 0) {
                printf("%s: argument must be a PID or %%jobid\n", argv[0]);
                return; 
            }

            if(notPid == NULL) {
                printf("(%d): No such process\n", pId);
                return; 
           }
        }
        else {
            //FOR JID
            char* argument = argv[1] + sizeof(char);
            int pp = atoi(argument);
            notPid = getjobjid(jobs, pp);
            if(notPid == NULL) {
                printf("%s: No such job\n", argv[1]);
                return; 
            }
            if(atoi(argument) == 0) {
                printf("%s: argument must be a PID or %%jobid\n", argv[0]);
                return; 
            }
        }

        if(back) {
            //printf("[%d] (%d) %s", notPid->jid, notPid->pid, notPid->cmdline);
            //Relized we dont need this^ after 2 hours of figuring out what was wrong! RIP
            kill(-(notPid->pid), SIGCONT);
            notPid->state = BG;
        }
        else {
            kill(-(notPid->pid), SIGCONT);
            notPid->state = FG;
            waitfg(notPid->pid);
        }
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    //while(fgpid(jobs) == pid) {}
    //Did this^ but causes busy waiting, so we tried the new thing below.

    sigset_t set, set2;

    //similar format to what we did in eval()
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    //block
    sigprocmask(SIG_BLOCK, &set, &set2);

    while(fgpid(jobs) == pid)
        sigsuspend(&set2);

    sigprocmask(SIG_SETMASK, &set2, NULL);
    //time to unblock ^
    return;
}


/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int pp;
    pid_t id = waitpid(-1, &pp, WUNTRACED|WNOHANG);
    char argument[MAXLINE];

    if(id <= 0)
        return;

    else if(id > 0) {
        struct job_t* notPid = getjobpid(jobs, id);

        if(notPid == NULL)
        return;

        if(WIFSTOPPED(pp)) {
            notPid->state = ST;
            sprintf(argument, "Job [%d] (%d) stopped by signal %d\n", notPid->jid, id, WSTOPSIG(pp));
            if(write(1, argument, strlen(argument)) != strlen(argument))
                return;
        }
        else if(WIFSIGNALED(pp)) {
            sprintf(argument, "Job [%d] (%d) terminated by signal %d\n", notPid->jid, id, WTERMSIG(pp));
            if(write(1, argument, strlen(argument)) != strlen(argument)) 
                return;
            deletejob(jobs, id);
            return;
        }
        else if(WIFEXITED(pp)) {
            deletejob(jobs, id);
            return;
        }
        else
            return;
    }
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    pid_t id = fgpid(jobs);
    if(id) {
        kill(-id, SIGINT);
        return;
    }
    return;
}


 // * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 // *     the user types ctrl-z at the keyboard. Catch it and suspend the
 // *     foreground job by sending it a SIGTSTP.  
 
void sigtstp_handler(int sig) 
{
    //Same as sigint_handler but here we are using SIGSTP
    pid_t id = fgpid(jobs);
    if(id) {
        kill(-id, SIGTSTP);
        return;
    }
    return;
}
//  void sigint_handler(int sig) 
// {
//   char section[MAXLINE];
//   pid_t pId = fgpid(jobs);
//   if(pId != 0) { //if there is no fg job, ignore
//     struct job_t* jobstruct = getjobpid(jobs, pId);
//     sprintf(section, "Job [%d] (%d) terminated by signal %d\n", jobstruct->jid, pId, sig);
//     if(write(1, section, strlen(section)) != strlen(section))
//       return;
//     kill(-pId, SIGINT); //kill all jobs in this group
//     deletejob(jobs, jobstruct->pid);
//     return;
//   }
//   else {
//     return;
//   }
// }


//  * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//  *     the user types ctrl-z at the keyboard. Catch it and suspend the
//  *     foreground job by sending it a SIGTSTP.  
 
// void sigtstp_handler(int sig) 
// {
//   char section[MAXLINE];
//   pid_t pId = fgpid(jobs);
//   if(pId != 0) { //if there is no bg job, ignore
//     struct job_t* joby = getjobpid(jobs, pId);
//     getjobpid(jobs, pId)->state = 3;
//     sprintf(section, "Job [%d] (%d) stopped by signal %d\n", joby->jid, pId, sig);
//     if(write(1, section, strlen(section)) != strlen(section))
//       return;
//     kill(-pId, SIGTSTP); //kill all jobs in this group
//     return;
//   }
//   else {
//     return;
//   }
// }

/*********************
 * End signal handlers
 *********************/



/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "Terminating after receipt of SIGQUIT signal\n", 45);
    if(bytes != 45)
       exit(-999);
    exit(1);
}

void forNotParent(char **cmdline, sigset_t set) {
    // sigprocmask(SIG_SETMASK, &set, NULL);
    // if(execv(cmdline[0], cmdline) < 0) {
    //     printf("%s: Command not found\n", cmdline[0]);
    //     exit(0);
    // }
    setpgid(0,0);
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
    execv(cmdline[0], cmdline);
    // printf("%s: Command not found\n",section[0]);
    exit(0);

}