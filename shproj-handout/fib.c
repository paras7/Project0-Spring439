#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

const int MAX = 13;

static void doFib(int n, int doPrint);


/*
 * unix_error - unix-style error routine.
 */
inline static void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


int main(int argc, char **argv)
{
    int arg;
    int print=1;

    if(argc != 2){
        fprintf(stderr, "Usage: fib <num>\n");
        exit(-1);
    }

    arg = atoi(argv[1]);
    if(arg < 0 || arg > MAX){
        fprintf(stderr, "number must be between 0 and %d\n", MAX);
        exit(-1);
    }

    doFib(arg, print);

    return 0;
}

/* 
 * Recursively compute the specified number. If print is
 * true, print it. Otherwise, provide it to my parent process.
 *
 * NOTE: The solution must be recursive and it must fork
 * a new child for each call. Each process should call
 * doFib() exactly once.
 */
static void doFib(int n, int doPrint)
{
    //Paras Driving
    //adding Variable
    int go1, go2, thread1, thread2, solution;
    pid_t notparent1, notParent2;

    //if it is 0 or 1 it should just print that because that's
    //how fibonacci sequence works.
    if(n==0 || n == 1) {
        if(doPrint != 0)
            printf("%d\n", n);
        exit(n);
    }
    else {
        notparent1 = fork();

        if(notparent1 != 0) {
            //wait and tell us the status
            waitpid(notparent1, &thread1, 0);
            go1 = WEXITSTATUS(thread1);
            //Ramon Driving
            notParent2 = fork();
            if(notParent2 != 0) {
                waitpid(notParent2, &thread2, 0);
                go2 = WEXITSTATUS(thread2);
                if(doPrint == 1)
                    printf("%d\n", (go2 + go1));
                else
                    exit((go2 + go1));
            }
            //Now time to recurse it
            else
                doFib(n-2, 0);
        }
        //recursing again
        else
            doFib(n-1, 0);
    }
}


