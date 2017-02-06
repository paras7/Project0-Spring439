#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"

//Ramon Driving
void try();
void bad();
/*
 * First, print out the process ID of this process.
 *
 * Then, set up the signal handler so that ^C causes
 * the program to print "Nice try.\n" and continue looping.
 *
 * Finally, loop forever, printing "Still here\n" once every
 * second.
 */
int main(int argc, char **argv)
{
	//Ramon driving
	int k;
	struct timespec* timer;
	timer = (struct timespec*)malloc(sizeof(struct timespec));
	timer->tv_nsec = 100000000;

	pid_t id = getpid();
	printf("%d\n", id);

	signal(SIGINT, try);
	signal(SIGUSR1, bad);

	for(; 1; ) {
		for(k= 0; k < 10; k++) {
			nanosleep(timer, 0);
		}
		if(write(1, "Still here\n", 11) != 11)
			exit(-1);
	}
	return 0;
}

//Paras Driving
void try() {
	if(write(1, "Nice try.\n", 10) != 10)
		exit(-1);
}

void bad() {
	if(write(1, "exiting\n", 8) != 8)
		exit(-1);
	exit(1);
}

