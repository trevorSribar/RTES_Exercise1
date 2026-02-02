// Date: 2/1/2026
// Author: Trevor Sribar, Julian Werder
// Class ECEN 5623 Real-Time Embedded Systems: Exercise 1 Part 4 Section C
// https://www.i-programmer.info/programming/cc/13002-applying-c-deadline-scheduling.html?start=1
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#include <unistd.h>
#include <sys/syscall.h>

#define DEADLINE 0 // 0 for FIFO and 1 for Deadline
const struct timespec TWO_SECONDS = {2, 0};
static struct timespec the_clock = {0, 0};

#if DEADLINE
struct sched_attr 
{
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) 
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

void * threadA(void *p) 
{
    struct sched_attr attr = 
    {
        .size = sizeof (attr),
        .sched_policy = SCHED_DEADLINE,
        .sched_runtime = 10 * 1000 * 1000,
        .sched_period = 2 * 1000 * 1000 * 1000,
        .sched_deadline = 11 * 1000 * 1000
    };

    sched_setattr(0, &attr, 0);

    for (;;) 
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &the_clock);
        printf("Time is %ld:%ld\n", the_clock.tv_sec, the_clock.tv_nsec);
        fflush(0);
        sched_yield();
    };
}
#else
void * threadA(void *p) 
{
    int ret;
    for (;;) 
    {
        clock_gettime(CLOCK_MONOTONIC, &the_clock);
        printf("Time is %ld:%09ld\n", the_clock.tv_sec, the_clock.tv_nsec);
        fflush(0);
        ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &TWO_SECONDS, NULL);
        if(ret != 0){
            printf("ERROR: %d\n",ret);
        }
        
    };
}
#endif

int main(int argc, char** argv) 
{
    pthread_t pthreadA;
#if DEADLINE
    pthread_create(&pthreadA, NULL, threadA, NULL);
#else
    pthread_attr_t thread_attr;
    struct sched_param thread_param;
    pthread_attr_init(&thread_attr);
    pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED); 
    pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO); 
    thread_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&thread_attr, &thread_param);
    pthread_create(&pthreadA, &thread_attr, threadA, NULL);
#endif
    pthread_exit(0);
    return (EXIT_SUCCESS);
}
