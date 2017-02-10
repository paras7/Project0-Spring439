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
	//using nano seconds
	timer = (struct timespec*)malloc(sizeof(struct timespec));
	timer->tv_nsec = 100000000;

	pid_t id = getpid();
	printf("%d\n", id);

	//sending out signal based on what it is
	signal(SIGINT, try);
	signal(SIGUSR1, bad);

	for(; 1; ) {
		//We have 8 zeros above for nanosleep and this for loop below
		//makes sure Still here is printed every second

		//We tried doing it with 9 zeros above and no for loop below, but
		//doesn't work for some reason

		//Okay, after talking to someone it might be because nanosleep can't
		//have 9 zeros and that is why it doesn't work
		for(k= 0; k < 10; k++) {
			nanosleep(timer, 0);
		}
		if(write(1, "Still here\n", 11) != 11)
			exit(-1);
	}
	return 0;
}

//Paras Driving
//Helper function to print Nice try
void try() {
	if(write(1, "Nice try.\n", 10) != 10)
		exit(-1);
}

//Helper function to print Exiting
void bad() {
	if(write(1, "exiting\n", 8) != 8)
		exit(-1);
	exit(1);
}

