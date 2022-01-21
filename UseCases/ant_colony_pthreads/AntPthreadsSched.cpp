/* -----------------------------------------------------------------------------
 * Copyright (C) 2012-2014 Software Competence Center Hagenberg GmbH (SCCH)
 * <georgios.chasparis@scch.at>, <office@scch.at>
 * -----------------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
 * This code is subject to dual-licensing. Please contact office@scch.at
 * if you are interested in obtaining a differently licensed version.
*/

//============================================================================
// Name        : SchedulingExample.cpp
// Author      : Georgios Chasparis
// Version     : 2.2.0
// Date		   : 27.09.2017
// Description :
//============================================================================

#include <iostream>
#include <vector>
#include <ctime>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <error.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
#include <vector>
#include <assert.h>
#include <errno.h>
#include <algorithm>
#include <numeric>
#include <cmath>

#include <math.h>
#include <sched.h>
#include <numa.h>
// #include <hwloc.h>

#define handle_error_en(en, msg) \
		do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


#include "ThreadInfo.h"
#include "ThreadControl.h"
#include "Scheduler.h"

#include "ThreadRoutine.h"

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

/*
 * Input Parameters
 */
/*Application Parameters*/

#define ANT_COLONY_OPTIMIZATION 			1
#define COMBS_EXAMPLE						0

/* Parameters of Combinations Example */
#define THREAD_COUNT    					20
#define THREAD_GOAL							100

/*Scheduler Parameters*/
#define STEP_SIZE							0.005		// the standard one is 0.005
#define LAMBDA								0.1			// the standard one is 0.2
#define SCHED_PERIOD		          		0.2
#define OS_MAPPING				       		1
#define RL_MAPPING							0
#define PR_MAPPING							0		// Predefined mapping used under some applications (e.g., matrix multiplication)
#define ST_MAPPING							0		// Allocating Stack together with thread when changing NUMA node (It is activated only if RL_mapping = 1)
#define OPTIMIZE_MAIN_RESOURCE				1		// Optimize over the main resource
#define SUSPEND_THREADS						0		// if 1, threads are suspended before re-allocated
#define GAMMA								0.01	// a parameter related to minimizing VARIANCE
#define RL_ACTIVE_RESHUFFLING				0		// Reshuffling when a thread becomes inactive
#define RL_PERFORMANCE_RESHUFFLING			0		// We reshuffle the strategies for

/*Output Parameters*/
#define PRINTOUT_STRATEGIES					0
#define PRINTOUT_ACTIONS					1

/*Testing Parameters (may only be used when THREAD_COUNT=4*/
#define WRITE_TO_FILES						1		// enables printing outcome to files
#define WRITE_TO_FILES_DETAILS				0		// enables printing outcome to files (including strategy updates)

/*Declaration of Input Parameters*/
bool ant_colony_optimization				= ANT_COLONY_OPTIMIZATION;
bool combs_example							= COMBS_EXAMPLE;
unsigned int thread_count 					= THREAD_COUNT;					/* Number of threads */
double step_size 							= STEP_SIZE;					/* Step-size of Reinforcement Learning Algorithm */
double lambda 								= LAMBDA;
bool printout_strategies					= PRINTOUT_STRATEGIES;
bool printout_actions						= PRINTOUT_ACTIONS;
bool write_to_files 						= WRITE_TO_FILES;
bool write_to_files_details					= WRITE_TO_FILES_DETAILS;
unsigned int thread_goal 					= THREAD_GOAL;
double sched_period 						= SCHED_PERIOD;	// in nanoseconds
bool OS_mapping 							= OS_MAPPING;
bool RL_mapping								= RL_MAPPING;
bool PR_mapping								= PR_MAPPING;
bool ST_mapping								= ST_MAPPING;
bool optimize_main_resource					= OPTIMIZE_MAIN_RESOURCE;
double gamma_par							= GAMMA;
bool suspend_threads						= SUSPEND_THREADS;
bool RL_active_reshuffling					= RL_ACTIVE_RESHUFFLING;
bool RL_performance_reshuffling				= RL_PERFORMANCE_RESHUFFLING;

// #define OVER_FMT "handler(%d ) Overflow at %p! bit=0x%llx \n"
// #define ERROR_RETURN(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__); exit(retval); }

#define handle_error_en(en, msg) \
		do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

double get_current_time()
{
  static int start = 0, startu = 0;
  struct timeval tval;
  double result;


  if (gettimeofday(&tval, NULL) == -1)
    result = -1.0;
  else if(!start) {
    start = tval.tv_sec;
    startu = tval.tv_usec;
    result = 0.0;
  }
  else
    result = (double) (tval.tv_sec - start) + 1.0e-6*(tval.tv_usec - startu);

  return result;
}


/*
 * Global Parameters w.r.t. the Ant Colony Optimization
 */
#define const_beta 2
#define const_xi 0.1
#define const_rho 0.1
#define const_q0 0.9
#define const_max_int 2147483647
#define const_e 0.000001

unsigned int num_ants;
unsigned int num_jobs;
unsigned int *weight;
unsigned int *deadline;
unsigned int *process_time;
unsigned int **all_results;
double *tau;
unsigned int *cost;
unsigned int best_t = UINT_MAX;
unsigned int *best_result;

/*
 * Functions used within the Ant Colony Optimization
 */
void init(char *fname, unsigned int num_ants)
{
  unsigned int num_tasks;
  ifstream infile(fname);
  if (infile.is_open())
  {
	    std::cout << " Successfully opening the input file !!!\n";
	    infile >> num_jobs >> num_tasks;

		weight = new unsigned int[num_jobs];
		deadline = new unsigned int[num_jobs];
		process_time = new unsigned int[num_jobs];

		for(int j=0; j<num_jobs; j++)
		  infile >> process_time[j];
		for(int j=0; j<num_jobs; j++)
		  infile >> weight[j];
		for(int j=0; j<num_jobs; j++)
		  infile >> deadline[j];

		all_results = new unsigned int *[num_ants];

		for (int j=0; j<num_ants; j++)
		  all_results[j] = new unsigned int[num_jobs];

		tau = new double [num_jobs*num_jobs];
		for (int j=0; j<num_jobs; j++)
		{
		  for (int i=0; i<num_jobs; i++)
		  {
			 tau[i*num_jobs+j] = 1.0;
		  }
		}
		best_result = new unsigned int[num_jobs];
  }
  else
  {
	  std::cerr << " Error reading the input file !!! \n";
	  exit(1);
  }
}

static void mdd( double *res, unsigned int scheduled_time )
{
  unsigned int i;
  for (i=0; i<num_jobs; i++) {
    if (scheduled_time + process_time[i] > deadline[i])
      res[i] = 1.0 / (scheduled_time + process_time[i] );
    else
      res[i] = 1.0 / (deadline[i] );
  }
}

static void findSolution( unsigned int *results,
                          double *pij,
                          double *eta,
                          double *tau)
{
  unsigned int i,j,k;
  unsigned int scheduled_time = 0;
  unsigned int remain_time = 0;
  double sumpij = 0;
  unsigned int *tabus = new unsigned int[num_jobs];
  double q;
  double *tauk;

  double maxp;
  double x;

  memset(tabus, 1, num_jobs * sizeof(unsigned int));

  for (i = 0; i < num_jobs; i++){
	  remain_time += process_time[i];
  }

  for (k = 0; k < num_jobs-1; k++)
  {
		tauk = &tau[k*num_jobs];
		mdd(eta, scheduled_time);
		for (i = 0; i < num_jobs; i++) {
			if (tabus[i] != 0) {
				pij[i] = pow(eta[i], const_beta) * tauk[i];
				sumpij += pij[i];
			}
			else
				pij[i] = 0;
		}

		q = ((double)rand())/RAND_MAX;
		if (q < const_q0)
		{
			j = 0;
			maxp = pij[0];
			for (i = 1; i < num_jobs; i++)
			{
				if (pij[i] > maxp)
				{
					j = i;
					maxp= pij[i];
				}
			}
		}
		else
		{
			q = ((double)rand())/RAND_MAX;
			q *= sumpij;
			double temp = 0.0;
			j = 0;
			while(temp - const_e < q && j < num_jobs )
			{
				temp += pij[j];
				j++;
			}
			j--;
			while ( tabus[j] == 0) j--;
		}
		if (j>num_jobs)
		  fprintf (stderr, "kuckuricku %d\n", j);

		results[k] = j;
		tabus[j] = 0;
		scheduled_time += process_time[j];
		remain_time -= process_time[j];

  }

  // find the last job
  j = 0;
  for (i = 0; i < num_jobs; i++){
    if (tabus[i] != 0) j = i;
  }
  k = num_jobs-1;
  results[k] = j;
  free( tabus);

}

unsigned int fitness( unsigned int *result )
{
  unsigned int cost = 0;
  unsigned int i,j;
  unsigned int time = 0;
  for (i = 0; i < num_jobs; i++)
  {
		j = result[i];
		time += process_time[j];
		if (time > deadline[j])
		{
		  cost += (time - deadline[j]) * weight[j];
		}
  }
  return(cost);
}


/*
 * Solving
 */
unsigned int solve ( unsigned int ant_id )
{
  unsigned int cost;
  double *pij = new double[num_jobs];
  double *eta = new double[num_jobs];

  findSolution( all_results[ant_id],
                pij,
                eta,
                tau);

  cost = fitness(all_results[ant_id]);

  free( pij);
  free( eta);

  return(cost);

}


/*
 * Updating
 */
void update( unsigned int best_t, unsigned int *best_result ) {
  unsigned int i,j;
  double ddd = const_rho / best_t * num_jobs;
  double dmin = 1.0 / 2.0 / num_jobs;

  for (i = 0; i < num_jobs; i++)
  {
    for (j = 0; j < num_jobs; j++)
    {
    	tau[i*num_jobs+j] *= 1-const_rho;

    	if (tau[i*num_jobs+j] < dmin)
    		tau[i*num_jobs+j] = dmin;
    }
  }

  for (i = 0; i < num_jobs; i++) {
	  tau[i*num_jobs+best_result[i]] +=  ddd;
  }

}

unsigned int pick_best(unsigned int **best_result)
{
  unsigned int i,j;
  unsigned int best_t = UINT_MAX;

  for (i=0; i<num_ants; i++)
  {
      if(cost[i] < best_t)
      {
    	  best_t = cost[i];
    	  *best_result = all_results[i];
      }
  }

  return (best_t);
}

static void * thread_execute (void* arg)
{
	/*
	 * Initializing performance counters
	 */
	ThreadControl thread_control;
    thread_info * info = (static_cast<thread_info*>(arg));

    if(!thread_control.thd_init_counters (info->thread_id, arg))
    	printf("Error: Problem initializing counters for thread %d",info->thread_num);

    /*
	 * Get the pid of the process
	 */
    info->tid = syscall(SYS_gettid);
    info->pid = syscall(SYS_getpid);

    // Ant-Colony Optimization
	if (ant_colony_optimization)
	{
		for (unsigned int i = info->thread_ref; i < info->thread_ref + info->thread_goal; i++){
			cost[i] = solve (i);
		}
	}

	// Combinations Example:
	if (combs_example)
	{
		ThreadRoutine thread_routine;
		thread_routine.work(info);
	}

	info->status = 1;
	info->termination_time = info->time_before - info->time_init;
	return (void *)0;
}


void thread_create(thread_info* tinfo, const unsigned int & num_iter)
{

    pthread_attr_t detach;
    int status;
    int i;

    /*
     * The detach state attribute determines whether a thread created
     * using the thread attributes object 'attr' will be created in a joinable or a
     * detach state.
     *
     */
    status = pthread_attr_init(&detach);
    if (status != 0)
        // err_abort (status, "Init attributes object");
    	std::cout << "Init attributes object" << std::endl;

	status = pthread_attr_setdetachstate(&detach, PTHREAD_CREATE_DETACHED);
	if (status != 0)
		std::cerr << "Set create-detached" << std::endl;

	if (ant_colony_optimization)
	{

		std::cout << " entering ant_colony_optimization\n";

		/*
		 * User-specified part of the code.
		 * The user should appropriately specify the arguments necessary for thread execution and the definition of the function 'thread_execute'
		 * which performs the numerical operations executed by the application.
		 */
		//for (unsigned int j = 0; j < num_iter; j++) {

			// inside this loop, we parallelize the solution ( # threads = # ants )
			// in other words, the 'execute' function of the thread corresponds to the solve function of each ant.

			for (unsigned int thread_ind = 0; thread_ind < thread_count; thread_ind++)
			{

				if (ant_colony_optimization)
				{
					/* Setting some generic parameters for each thread */
					tinfo[thread_ind].thread_num 	= thread_ind;
					if ((thread_count>=2) && (thread_ind < thread_count-1))
					{
						tinfo[thread_ind].thread_goal 	= std::ceil((double)num_ants/(double)thread_count);
						tinfo[thread_ind].thread_ref 	= (double)thread_ind * std::ceil((double)num_ants/(double)thread_count);
					}
					else if (thread_count < 2)
					{
						tinfo[thread_ind].thread_goal = num_ants;
						tinfo[thread_ind].thread_ref = 0;
					}
					else if (thread_ind == thread_count-1)
					{
						tinfo[thread_ind].thread_goal = (double)num_ants - (double)(thread_count-1) * std::ceil( ((double)num_ants)/ ((double)thread_count ));
						tinfo[thread_ind].thread_ref = (double)(thread_count-1) * std::ceil( (double)num_ants / ((double)thread_count ));
					}

					std::cout << " thread " << thread_ind << " has reference " << tinfo[thread_ind].thread_ref << " and goal " << tinfo[thread_ind].thread_goal << std::endl;
				}

				if (combs_example)
					tinfo[thread_ind].thread_goal = THREAD_GOAL;

				tinfo[thread_ind].status = 0;

				/*
				 * For each thread to be created, we assign a version of ThreadRoutine.
				 * This can be different for each thread, however here is an idendical function.
				 */
				std::cout << " creating thread " << thread_ind << std::endl;
				// ThreadRoutine thread_routine();
				status = pthread_create(&tinfo[thread_ind].thread_id, &detach, &thread_execute, &tinfo[thread_ind]);
				if (status != 0)
					handle_error("Creating thread failed!");

				std::cout << " thread_id of thread " << thread_ind << " is " << tinfo[thread_ind].return_thread_id() << std::endl;

			}
		//}
	}
	else if (combs_example)
	{
		for (i = 0; i< THREAD_COUNT; i++)
		{

			/* Setting some generic parameters for each thread */
			tinfo[i].thread_num 	= i+1;
			tinfo[i].thread_goal 	= THREAD_GOAL;
			tinfo[i].status 		= 0;

			/*
			 * For each thread to be created, we assign a version of ThreadRoutine.
			 * This can be different for each thread, however here is an identical function.
			 */
			status = pthread_create(&tinfo[i].thread_id, &detach, &thread_execute, &tinfo[i]);
			if (status != 0)
				handle_error("Creating thread failed!");

			std::cout << " thread_id of thread " << i << " is " << tinfo[i].return_thread_id() << std::endl;
		}
	}

}


int main (int argc, char *argv[])
{

	/*
	 * Initializing parameters of Ant-Colony Optimization
	 */
	unsigned int num_iter;
	char *fname;
	unsigned int i, j;
	unsigned int num_workers, do_chunking=0, chunk_size=1;
	unsigned int num_cpus_1numa, num_cpus_2numa;
	bool RL_mapping(false);
	std::string Scheduler_conf;

	if ( ( argc<7 || argc > 7) && ant_colony_optimization) {

		cerr << "Usage: ant_farm <nr_workers> <nr_iterations> <nr_ants> <input_filename> <do_chunking> <Scheduler_conf>" << endl;
		exit(1);

	}
	else if ( argc < 2 || argc > 2 && combs_example){
		cerr << "Usage: combs_example <nr_workers> " << endl;
	}
	else if (argc == 7 && ant_colony_optimization)
	{
		num_workers = atoi(argv[1]);		/* Number of workers */
		num_iter = atoi(argv[2]);			/* Number of optimization iterations */
		num_ants = atoi(argv[3]);			/* Number of ants */

		fname = argv[4];
		do_chunking = atoi(argv[5]);
		Scheduler_conf = argv[6];

		init (fname, num_ants);
		cost = new unsigned int[num_ants];

		if (do_chunking)
			chunk_size = num_ants / num_workers;

		// In case, we run the Ant Colony Optimization, we set the THREAD_COUNT = num of workers.
		if (ant_colony_optimization)
			thread_count = num_workers;

	}
	else if (argc == 2 && combs_example)
	{
		num_workers = atoi(argv[1]);
	}

	double t1 = get_current_time();

    /*
     * Initialize Scheduler
     *
     * @inputs:
     * 		- Types of Resources to be scheduled (e.g., CPU bandwidth, Memory, etc.)
     * 		- In NUMA architectures, we should be able to select node for a running thread.
     */
	
    //Scheduler scheduler(thread_count, RL_mapping);
	std::cout << "starting scheduler with config file \n";
	Scheduler scheduler(Scheduler_conf, thread_count);

    /*
     * Defining Threads / Parallelization Pattern
     *
     * @description: This part might be different for each application and it is up to the user to define properly these threads.
     * The main important input for the PaRLSched library is to get access to the id's of the threads defined.
     */
    thread_create( scheduler.get_tinfo(), num_iter );

    /*
     * Run Scheduler
     *
     * @description: This function call initiates the scheduler, which involves the pre-defined resource allocation
     *
     */
   	scheduler.run();

    double t2 = get_current_time();
    std::cout << "Total execution time: " << t2-t1 << "\n";

    pthread_exit ( NULL );        /* Let threads finish */

}




