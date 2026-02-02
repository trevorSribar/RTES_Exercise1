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
#include <time.h>
#include <syslog.h>

#define MS_PER_SEC (1000)
#define NS_PER_MS (1000000)
#define NS_PER_SEC (1000000000)

#define MAX_PRIORITY (sched_get_priority_max(SCHED_FIFO))
#define MIN_PRIORITY (sched_get_priority_min(SCHED_FIFO))
#if (MAX_PRIORITY > MIN_PRIORITY)
    #define FIB10_PRIORITY (MIN_PRIORITY + 2)
    #define FIB20_PRIORITY (MIN_PRIORITY + 1)
#else
    #define FIB10_PRIORITY (MIN_PRIORITY - 2) 
    #define FIB20_PRIORITY (MIN_PRIORITY - 1)
#endif
#define MAIN_PRIORITY (MAX_PRIORITY)
#define MAX_FIB_DUR 93

// structs
static pthread_t main_thread, fib10_thread, fib20_thread;
static pthread_attr_t main_sched_attr, fib10_sched_attr, fib20_sched_attr;
static struct sched_param main_param;
static struct sched_param fib10_param;
static struct sched_param fib20_param;
static int errorHandler_main, erorrHandler_fib10, errorHandler_fib20;

//timing
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

void fib10(){
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    fib(MAX_FIB_DUR);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    syslog(LOG_INFO, "fib10 completed at %f s in %f ms", end.tv_sec + end.tv_nsec / NS_PER_SEC, (end.tv_sec - start.tv_sec) * MS_PER_SEC + (end.tv_nsec - start.tv_nsec) / NS_PER_MS);
}

void fib20(){

}

// functions
void main()
{
    //log initialization
    openlog("LOG_Exercise1_Part4_e_f_Fib", LOG_PID, LOG_USER);

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
       printf("ERROR; sched_setscheduler errorHandler was thrown\n");
       printf("Quick fix: try running with sudo.\n");
       perror("sched_setschduler"); exit(-1);
   }

   pthread_attr_setschedparam(&main_sched_attr, &main_param); // Sets the schedule parameters to rt_max_prio
   pthread_attr_setschedparam(&fib10_sched_attr, &fib10_param);
   pthread_attr_setschedparam(&fib20_sched_attr, &fib20_param);

   errorHandler_fib10 = pthread_create(&fib10_thread, &fib10_sched_attr, fib10, (void *)0); // creates a thread with the parameters defined above that executes delay_test
   errorHandler_fib20 = pthread_create(&fib20_thread, &fib20_sched_attr, fib20, (void *)0);

   if (errorHandler_fib10 || errorHandler_fib20)
   {
       printf("ERROR; pthread_create() errorHandler_fib10 is %d\n", errorHandler_fib10);
       printf("ERROR; pthread_create() errorHandler_fib20 is %d\n", errorHandler_fib20);
       perror("pthread_create");
       exit(-1);
   }

   //code for running scheduled events



   pthread_join(fib10_thread, NULL); // once the thread finnishes running, it will come back to main which kills the thread
   pthread_join(fib20_thread, NULL);

   if(pthread_attr_destroy(&main_sched_attr) != 0) // destroys the thread attributes (cleans memory) 
     perror("attr destroy"); // throws an error about destroying the attributes if it fails to
   if(pthread_attr_destroy(&fib10_sched_attr) != 0)
     perror("attr destroy fib10");
   if(pthread_attr_destroy(&fib20_sched_attr) != 0)
     perror("attr destroy fib20");
}