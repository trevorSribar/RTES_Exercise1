// Date: 2/1/2026
// Author: Trevor Sribar, Julian Werder
// Class ECEN 5623 Real-Time Embedded Systems: Exercise 1 Part 4 Section e & f
// Description: Synthetic Load Generation and Simuation using Fibonacci

// OUTLINE
// Fib10 that takes 10ms to compute
// Fib20 that takes 20ms to compute
// Run both Fib10 and Fib20 in separate threads
// Release Fib10 every 20ms and Fib20 every 50ms
// Measure the time for each thread to complete & LCM (100ms)

// include files
#define _GNU_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h> 
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>

#define MS_PER_SEC (1000)
#define NS_PER_US (1000)
#define NS_PER_SEC (1000000000)
#define TWENTY_NS_IN_MS 20000000
#define TEN_NS_IN_MS 10000000

#define MAX_PRIORITY 99
#define MIN_PRIORITY 88
    #define FIB10_PRIORITY (MIN_PRIORITY + 2)
    #define FIB20_PRIORITY (MIN_PRIORITY + 1)
#define MAIN_PRIORITY (MAX_PRIORITY)
#define MAX_FIB_DUR 93
#define ClockType CLOCK_MONOTONIC_RAW

static int NumRunFibInFib10 = 139; // Ntimes fib(MAX_FIB_DUR) is run so fib10 ~ 10ms
static int NumRunFibInFib20 = 278;

// structs
static pthread_t fib10_thread, fib20_thread;
static pthread_attr_t main_sched_attr, fib10_sched_attr, fib20_sched_attr;
static struct sched_param main_param;
static struct sched_param fib10_param;
static struct sched_param fib20_param;
static int errorHandler_main, errorHandler_fib10, errorHandler_fib20;

//semaphores
sem_t fib10sem;
sem_t fib20sem;

//timing
static struct timespec scheduletime = {0, 0};
static struct timespec start = {0, 0};
static struct timespec end = {0, 0};

unsigned long long fib(int n){
    volatile unsigned long long first = 0, second = 1, next;

    for (int i = 0; i < n; i++) {
        if (i <= 1) {
            next = i;
        } else {
            next = first + second;
            first = second;
            second = next;
        }
    }
    return next;
}

void* fib10(){
    while(1){
        sem_wait(&fib10sem);

        clock_gettime(ClockType, &start);
        for(int i = 0; i < NumRunFibInFib10; i++)
        fib(MAX_FIB_DUR);
        clock_gettime(ClockType, &end);
        syslog(LOG_INFO, "fib10 completed at %f s, which took %ld.%lds and %ld ns", 
            (0.0 + end.tv_sec + end.tv_nsec) / NS_PER_SEC, 
            end.tv_sec - start.tv_sec,
            (long)((end.tv_nsec - start.tv_nsec)/NS_PER_US),
            (end.tv_nsec - start.tv_nsec)%NS_PER_US);
    }
}

void* fib20(){
    while(1){
        sem_wait(&fib20sem);

        clock_gettime(ClockType, &start);
        for(int i = 0; i < NumRunFibInFib20; i++)
        fib(MAX_FIB_DUR);
        clock_gettime(ClockType, &end);
        syslog(LOG_INFO, "fib20 completed at %f s, which took %ld.%lds and %ld ns", 
            (0.0 + end.tv_sec + end.tv_nsec) / NS_PER_SEC, 
            end.tv_sec - start.tv_sec,
            (long)((end.tv_nsec - start.tv_nsec)/NS_PER_US),
            (end.tv_nsec - start.tv_nsec)%NS_PER_US);
    }
}

void setFib10NumRun(){//should be called after initializing threads
    int lastRunTimeNS = 0;
    for(int i = 0; i < 10000; i++){
        int avgRuntime = 0;
        for(int j = 0; j<4; j++){
            clock_gettime(ClockType, &start);
            //release thread for fib10

            clock_gettime(ClockType, &end);
            avgRuntime += ((end.tv_sec - start.tv_sec) * NS_PER_SEC + (end.tv_nsec - start.tv_nsec))/4;
        }
        lastRunTimeNS = (int)avgRuntime;
        if(lastRunTimeNS > 10000000){//10ms
            NumRunFibInFib10--;
        }
        else if(lastRunTimeNS < 10000000){
            NumRunFibInFib10++;
        }
        else {
            syslog(LOG_INFO, "got exactly 10ms avg with iteration amount of %d", NumRunFibInFib10);
            break;
        }
    }
    syslog(LOG_INFO, "Never got exactly 10ms, most recent was was %d", NumRunFibInFib10);
}
void setFib20NumRun(){//should be called after initializing threads
    int lastRunTimeNS = 0;
    for(int i = 0; i < 10000; i++){
        int avgRuntime = 0;
        for(int j = 0; j<4; j++){
            clock_gettime(ClockType, &start);
            //release thread for fib20

            clock_gettime(ClockType, &end);
            avgRuntime += ((end.tv_sec - start.tv_sec) * NS_PER_SEC + (end.tv_nsec - start.tv_nsec))/4;
        }
        lastRunTimeNS = (int)avgRuntime;
        if(lastRunTimeNS > 20000000){//20ms
            NumRunFibInFib20--;
        }
        else if(lastRunTimeNS < 20000000){
            NumRunFibInFib20++;
        }
        else {
            syslog(LOG_INFO, "got exactly 20ms avg with iteration amount of %d", NumRunFibInFib20);
            break;
        }
    }
    syslog(LOG_INFO, "Never got exactly 10ms, most recent was was %d", NumRunFibInFib10);
}

// functions
int main()
{
    //log initialization
    openlog("LOG_Exercise1_Part4_e_f_Fib", LOG_PID, LOG_USER);

    //initialize semaphores
    sem_init(&fib10sem, 0, 0);
    sem_init(&fib20sem, 0, 0);

    //thread initialization
    pthread_attr_init(&main_sched_attr);// initializes the thread to default values, stored in the passed variable
    pthread_attr_init(&fib10_sched_attr);
    pthread_attr_init(&fib20_sched_attr);
    pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);// sets the new thread to use the schedule explicitly defined in the attribute object
    pthread_attr_setinheritsched(&fib10_sched_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&fib20_sched_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);// Sets the priority to FIFO
    pthread_attr_setschedpolicy(&fib10_sched_attr, SCHED_FIFO);
    pthread_attr_setschedpolicy(&fib20_sched_attr, SCHED_FIFO);

    main_param.sched_priority = MAIN_PRIORITY;// sets main to max priority
    fib10_param.sched_priority = FIB10_PRIORITY;
    fib20_param.sched_priority = FIB20_PRIORITY;
    errorHandler_main=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);// updates the scheduler with max priority for this thread (main)
    errorHandler_fib10=sched_setscheduler(getpid(), SCHED_FIFO, &fib10_param);
    errorHandler_fib20=sched_setscheduler(getpid(), SCHED_FIFO, &fib20_param);

    if (errorHandler_main || errorHandler_fib10 || errorHandler_fib20) // returns 0 on success, so on an error it will throw the errors & print below.
   {
       //printf("ERROR; sched_setscheduler errorHandler was thrown\n");
       //printf("Quick fix: try running with sudo.\n");
       perror("sched_setschduler, try running with sudo"); exit(-1);
   }

   pthread_attr_setschedparam(&main_sched_attr, &main_param); // Sets the schedule parameters to rt_max_prio
   pthread_attr_setschedparam(&fib10_sched_attr, &fib10_param);
   pthread_attr_setschedparam(&fib20_sched_attr, &fib20_param);

   errorHandler_fib10 = pthread_create(&fib10_thread, &fib10_sched_attr, fib10, (void *)0); // creates a thread with the parameters defined above that executes delay_test
   errorHandler_fib20 = pthread_create(&fib20_thread, &fib20_sched_attr, fib20, (void *)0);

   if (errorHandler_fib10 || errorHandler_fib20)
   {
       //printf("ERROR; pthread_create() errorHandler_fib10 is %d\n", errorHandler_fib10);
       //printf("ERROR; pthread_create() errorHandler_fib20 is %d\n", errorHandler_fib20);
       perror("pthread_create, failed to create thread");
       exit(-1);
   }

   while(1){
        //release 10 & 20, t0
        sem_post(&fib10sem);
        sem_post(&fib20sem);

        clock_gettime(CLOCK_MONOTONIC_RAW, &scheduletime);
        scheduletime.tv_nsec += TWENTY_NS_IN_MS;
        if(scheduletime.tv_nsec > 999999999){
            scheduletime.tv_nsec -= 1000000000;
            scheduletime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &scheduletime, NULL);

        //release 10, t20
        sem_post(&fib10sem);
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &scheduletime);
        scheduletime.tv_nsec += TWENTY_NS_IN_MS;
        if(scheduletime.tv_nsec > 999999999){
            scheduletime.tv_nsec -= 1000000000;
            scheduletime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &scheduletime, NULL);

        //release 10, t40
        sem_post(&fib10sem);

        clock_gettime(CLOCK_MONOTONIC_RAW, &scheduletime);
        scheduletime.tv_nsec += TEN_NS_IN_MS;
        if(scheduletime.tv_nsec > 999999999){
            scheduletime.tv_nsec -= 1000000000;
            scheduletime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &scheduletime, NULL);

        //release 20, t50
        sem_post(&fib20sem);

        clock_gettime(CLOCK_MONOTONIC_RAW, &scheduletime);
        scheduletime.tv_nsec += TEN_NS_IN_MS;
        if(scheduletime.tv_nsec > 999999999){
            scheduletime.tv_nsec -= 1000000000;
            scheduletime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &scheduletime, NULL);

        //release 10, t60
        sem_post(&fib10sem);

        clock_gettime(CLOCK_MONOTONIC_RAW, &scheduletime);
        scheduletime.tv_nsec += TWENTY_NS_IN_MS;
        if(scheduletime.tv_nsec > 999999999){
            scheduletime.tv_nsec -= 1000000000;
            scheduletime.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &scheduletime, NULL);

        //release 10, t80
        sem_post(&fib10sem);
    }

   pthread_join(fib10_thread, NULL); // once the thread finnishes running, it will come back to main which kills the thread
   pthread_join(fib20_thread, NULL);

   if(pthread_attr_destroy(&main_sched_attr) != 0) // destroys the thread attributes (cleans memory) 
     perror("attr destroy"); // throws an error about destroying the attributes if it fails to
   if(pthread_attr_destroy(&fib10_sched_attr) != 0)
     perror("attr destroy fib10");
   if(pthread_attr_destroy(&fib20_sched_attr) != 0)
     perror("attr destroy fib20");

    return 0;
}