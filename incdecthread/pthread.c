#define _GNU_SOURCE

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <semaphore.h>

#define COUNT  1000 //
#define SEM_OR_PRIO 1 // 1 to use semaphore, 0 to use priority scheduling

typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[2];
threadParams_t threadParams[2];
pthread_attr_t rt_sched_attr[2];
#if SEM_OR_PRIO
sem_t semaphore;
#else
struct sched_param rt_param[2];
#endif

// Unsafe global
static int gsum=0;

void *incThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        gsum += i; // increment the global sum
        printf("Increment thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }

#if SEM_OR_PRIO
    sem_post(&semaphore); // signal decrement thread to start
#endif
}


void *decThread(void *threadp)
{
#if SEM_OR_PRIO
    sem_wait(&semaphore); // wait for increment thread to finish
#endif

    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        gsum -= i; // decrement the global sum
        printf("Decrement thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }
}




int main (int argc, char *argv[])
{
    int rc;
    int i=0;
#if !SEM_OR_PRIO
    cpu_set_t cpuset;
#endif

    pthread_attr_init(&rt_sched_attr[0]);
    pthread_attr_init(&rt_sched_attr[1]);

#if SEM_OR_PRIO
    sem_init(&semaphore, 0, 0); //shared between threads, initial value 0
#else
    CPU_ZERO(&cpuset);
    CPU_SET(0,&cpuset);

    // set to take schedule attributes from attributes object
    pthread_attr_setinheritsched(&rt_sched_attr[0], PTHREAD_EXPLICIT_SCHED); 
    pthread_attr_setinheritsched(&rt_sched_attr[1], PTHREAD_EXPLICIT_SCHED);
    // set the schedule to FIFO
    pthread_attr_setschedpolicy(&rt_sched_attr[0], SCHED_FIFO); 
    pthread_attr_setschedpolicy(&rt_sched_attr[1], SCHED_FIFO);

    rt_param[0].sched_priority = sched_get_priority_max(SCHED_FIFO); // max priority for increment thread
    rt_param[1].sched_priority = sched_get_priority_min(SCHED_FIFO); // min priority for decrement thread

    // set the param (just priority)
    pthread_attr_setschedparam(&rt_sched_attr[0], &(rt_param[0])); 
    pthread_attr_setschedparam(&rt_sched_attr[1], &(rt_param[1]));

    // lock to one CPU
    pthread_attr_setaffinity_np(&rt_sched_attr[0], sizeof(cpu_set_t), &cpuset); 
    pthread_attr_setaffinity_np(&rt_sched_attr[1], sizeof(cpu_set_t), &cpuset);
#endif

   threadParams[i].threadIdx=i;
   pthread_create(&threads[i],   // pointer to thread descriptor
                  &rt_sched_attr[i],     // use scheduling attributes
                  incThread, // thread function entry point
                  &threadParams[i] // parameters to pass in
                 );
    i++;

   threadParams[i].threadIdx=i;
   pthread_create(&threads[i], &rt_sched_attr[i], decThread, &threadParams[i]); // create decrement threasd
   
   for(i=0; i<2; i++)
     pthread_join(threads[i], NULL); // join the threads

   printf("TEST COMPLETE\n");
}
