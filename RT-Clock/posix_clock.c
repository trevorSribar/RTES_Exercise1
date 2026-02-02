/****************************************************************************/
/* Function: nanosleep and POSIX 1003.1b RT clock demonstration             */
/*                                                                          */
/* Sam Siewert - 02/05/2011                                                 */
/* Modified Trevor & Julian - 01/31/2026                                    */
/*                                                                          */
/****************************************************************************/

// include files
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

// defined constants
#define NSEC_PER_SEC (1000000000)
#define NSEC_PER_MSEC (1000000)
#define NSEC_PER_USEC (1000)
#define ERROR (-1)
#define OK (0)
#define TEST_SECONDS (0)
#define TEST_NANOSECONDS (NSEC_PER_MSEC * 10)

// premeptive function declaration
void end_delay_test(void);

// structs
static struct timespec sleep_time = {0, 0};
static struct timespec sleep_requested = {0, 0};
static struct timespec remaining_time = {0, 0};

// file globals
static unsigned int sleep_count = 0;

pthread_t main_thread;
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min;
struct sched_param main_param;


void print_scheduler(void) //prints the schedule type of the current process
{
   int schedType; //define an integer to hold the scheduling type

   schedType = sched_getscheduler(getpid());//gets the scheduling type for the current process ID

   switch(schedType) // switch statement to print the scheduling type
   {
     case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n");
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}


double d_ftime(struct timespec *fstart, struct timespec *fstop) //returns the difference in time between two timespec structs (double)
{
  double dfstart = ((double)(fstart->tv_sec) + ((double)(fstart->tv_nsec) / 1000000000.0)); //start time (seconds + ns)
  double dfstop = ((double)(fstop->tv_sec) + ((double)(fstop->tv_nsec) / 1000000000.0)); //stop time in (seconds + ns)

  return(dfstop - dfstart); 
}


int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t) //calcs difference in 2 timespec structs & stores the result in delta_t, erorr if stop ealier than start
{
  int dt_sec=stop->tv_sec - start->tv_sec; // difference in seconds
  int dt_nsec=stop->tv_nsec - start->tv_nsec; // difference in nanoseconds

  //printf("\ndt calcuation\n");

  // case 1 - less than a second of change
  if(dt_sec == 0)
  {
	  //printf("dt less than 1 second\n");

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC) //if we have +diff & less than 1 sec
	  {
	          //printf("nanosec greater at stop than start\n");
		  delta_t->tv_sec = 0;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC) // if we have +diff & more than 1 sec
	  {
	          //printf("nanosec overflow\n");
		  delta_t->tv_sec = 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means stop is earlier than start
	  {
	         printf("stop is earlier than start\n");
		 return(ERROR);  
	  }
  }

  // case 2 - more than a second of change, check for roll-over
  else if(dt_sec > 0)
  {
	  //printf("dt more than 1 second\n");

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC) //if we have +diff & less than 1 sec
	  {
	          //printf("nanosec greater at stop than start\n");
		  delta_t->tv_sec = dt_sec;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC) // if we have +diff & more than 1 sec
	  {
	          //printf("nanosec overflow\n");
		  delta_t->tv_sec = delta_t->tv_sec + 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means roll over
	  {
	          //printf("nanosec roll over\n");
		  delta_t->tv_sec = dt_sec-1;
		  delta_t->tv_nsec = NSEC_PER_SEC + dt_nsec;
	  }
  }

  return(OK);
}

// time structs for RT clock
static struct timespec rtclk_dt = {0, 0};
static struct timespec rtclk_start_time = {0, 0};
static struct timespec rtclk_stop_time = {0, 0};
static struct timespec delay_error = {0, 0};

// Clock type
//#define MY_CLOCK CLOCK_REALTIME // user can change timestamps (bad)
//#define MY_CLOCK CLOCK_MONOTONIC // user can change frequency (bad)
#define MY_CLOCK CLOCK_MONOTONIC_RAW // like MONOTONIC but takes raw hardware time (good)
//#define MY_CLOCK CLOCK_REALTIME_COARSE // 
//#define MY_CLOCK CLOCK_MONOTONIC_COARSE // like MONOTONIC but lower resolution & faster=

// # of interations
#define TEST_ITERATIONS (100)

void *delay_test(void *threadID) //checks the delay error of nanosleep TEST_ITERATIONS times
{
  //function variables initilization
  int idx, rc; 
  unsigned int max_sleep_calls=3;
  int flags = 0;
  struct timespec rtclk_resolution;

  sleep_count = 0; // reset sleep count

  if(clock_getres(MY_CLOCK, &rtclk_resolution) == ERROR) // checks if clock resolution is an error
  {
      perror("clock_getres");
      exit(ERROR); // originally erronious -1 for error
  }
  else // otherwise prints clock resolution
  { // originially this had an erronious 1000 for ns per us
      printf("\n\nPOSIX Clock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs, %ld nanosecs\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/NSEC_PER_USEC), rtclk_resolution.tv_nsec);
  }

  for(idx=0; idx < TEST_ITERATIONS; idx++) // loops for the number of TEST_ITERATIONS
  {
      printf("test %d\n", idx); //prints the current iteration number

      /* run test for defined seconds */
      sleep_time.tv_sec=TEST_SECONDS;
      sleep_time.tv_nsec=TEST_NANOSECONDS;
      sleep_requested.tv_sec=sleep_time.tv_sec;
      sleep_requested.tv_nsec=sleep_time.tv_nsec;

      /* start time stamp */ 
      clock_gettime(MY_CLOCK, &rtclk_start_time);

      /* request sleep time and repeat if time remains */
      do 
      {
          if(rc=nanosleep(&sleep_time, &remaining_time) == 0) break; // call nanosleep & if the sleep is not interrupted, break
         
          //otherwise update sleep_time to remaining_time
          sleep_time.tv_sec = remaining_time.tv_sec; 
          sleep_time.tv_nsec = remaining_time.tv_nsec;
          sleep_count++;
      } 
      while (((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0)) && (sleep_count < max_sleep_calls)); // repeats if we need to run for longer & we are less than max_sleep_calls

      clock_gettime(MY_CLOCK, &rtclk_stop_time); // stop time stamp

      delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt); // calculate elapsed time
      delta_t(&rtclk_dt, &sleep_requested, &delay_error); // calculate delay error

      end_delay_test(); //prints the results (clock DT & delay error)
  }

}

void end_delay_test(void) //prints clock DT & delay error
{
    double real_dt;
#if 0
  printf("MY_CLOCK start seconds = %ld, nanoseconds = %ld\n", 
         rtclk_start_time.tv_sec, rtclk_start_time.tv_nsec);
  
  printf("MY_CLOCK clock stop seconds = %ld, nanoseconds = %ld\n", 
         rtclk_stop_time.tv_sec, rtclk_stop_time.tv_nsec);
#endif

  real_dt=d_ftime(&rtclk_start_time, &rtclk_stop_time);
  printf("MY_CLOCK clock DT seconds = %ld, msec=%ld, usec=%ld, nsec=%ld, sec=%6.9lf\n", 
         rtclk_dt.tv_sec, rtclk_dt.tv_nsec/NSEC_PER_MSEC, rtclk_dt.tv_nsec/NSEC_PER_USEC, rtclk_dt.tv_nsec, real_dt);
  // removed eronuous numbers with macros
#if 0
  printf("Requested sleep seconds = %ld, nanoseconds = %ld\n", 
         sleep_requested.tv_sec, sleep_requested.tv_nsec);

  printf("\n");
  printf("Sleep loop count = %ld\n", sleep_count);
#endif
  printf("MY_CLOCK delay error = %ld, nanoseconds = %ld\n", 
         delay_error.tv_sec, delay_error.tv_nsec);
}

// is used in ifdef to run RT thread in main()
#define RUN_RT_THREAD

void main(void) //runs delay_test either as a thread or as a raw function call
{
   int rc, scope;

   printf("Before adjustments to scheduling policy:\n");
   print_scheduler();

#ifdef RUN_RT_THREAD //if we have RUN_RT_THREAD defined create and run a thread for delay_test
   pthread_attr_init(&main_sched_attr);// initializes the thread to default values, stored in the passed variable
   pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);// sets the new thread to use the schedule explicitly defined in the attribute object
   pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);// Sets the priority to FIFO

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);// returns the max priority for FIFO
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);// returns the min priority for FIFO

   main_param.sched_priority = rt_max_prio;// sets main to max priority
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);// updates the scheduler with max priority for this thread (main)


   if (rc) // returns 0 on success, so on an error it will throw the errors & print below.
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror("sched_setschduler"); exit(-1);
   }

   printf("After adjustments to scheduling policy:\n");
   print_scheduler();

   main_param.sched_priority = rt_max_prio;
   pthread_attr_setschedparam(&main_sched_attr, &main_param); // Sets the schedule parameters to rt_max_prio

   rc = pthread_create(&main_thread, &main_sched_attr, delay_test, (void *)0); // creates a thread with the parameters defined above that executes delay_test

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror("pthread_create");
       exit(-1);
   }

   pthread_join(main_thread, NULL); // once the thread finnishes running, it will come back to main which kills the thread

   if(pthread_attr_destroy(&main_sched_attr) != 0) // destroys the thread attributes (cleans memory) 
     perror("attr destroy"); // throws an error about destroying the attributes if it fails to
#else
   delay_test((void *)0); //if we don't have RUN_RT_THREAD defined run delay_test
#endif

   printf("TEST COMPLETE\n");
}

