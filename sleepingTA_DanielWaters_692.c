/*
 * sleepingTA_DanielWaters_692.c
 *
 *  Created on: Mar 29, 2018
 *      Author: Daniel Waters
 *	CSUID : 692
 */

#define MAX_SLEEP_TIME 3
#define NUM_OF_STUDENTS 4
#define NUM_OF_HELPS 2
#define NUM_OF_SEATS 2

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *student(void*);
void *assistant(void*);
int seekAssistant(int);

pthread_mutex_t mutex_lock;
sem_t students_sem;
sem_t ta_sem;

int waiting_students;

int main() {

	printf("CS149 SleepingTA from Daniel Waters\n");

	// Initialize mutex_lock and both semaphores with the value of 1
	sem_init(&students_sem, 0, 1);
	sem_init(&ta_sem, 0, 1);
	pthread_mutex_init(&mutex_lock, NULL);

	// Create an array of pthreads for the students and one for the TA
	pthread_t studentTid[NUM_OF_STUDENTS];
	pthread_t assistantTid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	// Initialize an array to pass in as student number ID's for the threads
	// and initialize the TA thread to seed of 4
	int seedAndStudentIds[NUM_OF_STUDENTS+1];
	for (int i = 0; i < NUM_OF_STUDENTS+1; i++) {
		seedAndStudentIds[i] = i;
	}

	// create the array of student threads, passing in the ID for parameters
	for (int i = 0; i < NUM_OF_STUDENTS; i++) {
		pthread_create(&studentTid[i], &attr, student, &seedAndStudentIds[i]);
	}
	// create the TA thread with no parameters
	pthread_create(&assistantTid, &attr, assistant, &seedAndStudentIds[NUM_OF_STUDENTS]);

	// Wait for all 4 student threads to terminate
	// They terminate once they have been helped twice
	for (int i = 0; i < NUM_OF_STUDENTS; i++) {
		pthread_join(studentTid[i], NULL);
	}
	// After all students are done, stop the TA
	pthread_cancel(assistantTid);

	return 0;
}

// Student thread that programs 1-3 seconds
void *student(void* param) {
	// variable to track the number of times they have
	// been helped.  Thread should exit after 2 TA helps
	int numTimesHelped = 0;

	// Seed the random function with a seed based on
	// the student ID to randomize the amount of
	// time the student will program for
	int studentNum = *(int* ) param;
	unsigned int seed = studentNum;

	// continue programming and seeking help until
	// they have been helped twice.
	while (numTimesHelped < NUM_OF_HELPS) {
		// find the programming time randomly from 1 to 3
		int programmingTime = (rand_r(&seed) % MAX_SLEEP_TIME) + 1;
		printf("\tStudent %d programming for %d seconds\n", studentNum, programmingTime);
		sleep(programmingTime);

		// if student has been helped by the TA, than
		// increase counter
		// seekAssistant() returns 0 if not helped, 1 if helped
		numTimesHelped += seekAssistant(studentNum);
	}

	// Been helped twice so return
	pthread_exit(0);
}

// TA assistant thread
void *assistant(void * param) {

	// set the TA seed to 4
	int seedNum = *(int* ) param;
	unsigned int seed = seedNum;

	// infinite loop, will be canceled by the
	// main thread when all 4 student threads are
	// finished
	while(1) {

		// acquire mutex lock and check if there
		// are any waiting students
		pthread_mutex_lock(&mutex_lock);
		if (waiting_students > 0) {

			// If there are waiting students, grab one
			// and help them for 1-3 seconds as they program
			waiting_students--;
			int helpingTime = (rand_r(&seed) % MAX_SLEEP_TIME) + 1;
			printf("Helping a student for %d seconds, # of waiting students = %d\n", helpingTime, waiting_students);

			// done mutating the count, so release lock
			pthread_mutex_unlock(&mutex_lock);

			// sleep 1-3 seconds (helping student)
			sleep(helpingTime);

			// let students know that you are available
			// to help the next student by posting
			// to the TA semaphore
			sem_post(&ta_sem);
		} else {

			// If there are not students waiting
			// release the lock
			pthread_mutex_unlock(&mutex_lock);

			// sleep and wait for a student
			// to become available
			sem_wait(&students_sem);
		}
	}

	// should not be called as the main thread will
	// terminate this thread when all students
	// are done
	pthread_exit(0);
}

// function for the students that seek TA
// help after doing their programming
int seekAssistant(int studentNum) {

	// acquire lock to check if there are seats available
	pthread_mutex_lock(&mutex_lock);

	if (waiting_students < NUM_OF_SEATS) {

		// if there are seats available, take one
		waiting_students++;

		printf("\t\tStudent %d takes a seat, # of waiting students = %d\n", studentNum, waiting_students);

		// already mutated counter, so release lock
		pthread_mutex_unlock(&mutex_lock);

		// let TA know that there is a student waiting
		// if they sleeping
		sem_post(&students_sem);

		// wait for the TA to be finished with current student
		sem_wait(&ta_sem);

		// TA is available, so have them program for 3 seconds
		printf("Student %d receiving help\n", studentNum);

		// student was helped so return 1 to increase
		// the number of times they have been helped
		return 1;
	} else {

		// if no seats available, just print that out
		// and release the lock
		printf("\t\t\tStudent %d will try later\n", studentNum);
		pthread_mutex_unlock(&mutex_lock);

		// return 0 because student was not helped
		// this time around
		// go back to programming again
		return 0;
	}
}
