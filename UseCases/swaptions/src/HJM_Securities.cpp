//HJM_Securities.cpp
//Routines to compute various security prices using HJM framework (via Simulation).
//Authors: Mark Broadie, Jatin Dewanwala
//Collaborator: Mikhail Smelyanskiy, Jike Chong, Intel
//Modified by Christian Bienia for the PARSEC Benchmark Suite

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <time.h>

#include "nr_routines.h"
#include "HJM.h"
#include "HJM_Securities.h"
#include "HJM_type.h"

#include "ThreadInfo.h"
#include "ThreadControl.h"
#include "Scheduler.h"

#include <pthread.h>
#define MAX_THREAD 1024

int NUM_TRIALS = DEFAULT_NUM_TRIALS;
int nThreads = 1;
int nSwaptions = 1;
int iN = 11; 
//FTYPE dYears = 5.5;
int iFactors = 3; 
parm *swaptions;

thread_info *tinfo;

long seed = 1979; //arbitrary (but constant) default value (birth year of Christian Bienia)
long swaption_seed;

// =================================================
FTYPE *dSumSimSwaptionPrice_global_ptr;
FTYPE *dSumSquareSimSwaptionPrice_global_ptr;
int chunksize;

void * worker(void *arg){
  int tid = *((int *)arg);
  FTYPE pdSwaptionPrice[2];

  ThreadControl thread_control;
  thread_info * info = &(tinfo[tid]);
  if(!thread_control.thd_init_counters (info->thread_id, (void *)info))
    printf("Error: Problem initializing counters for thread %d",info->thread_num);

  /*
   * Get the pid of the process
   */
  info->tid = syscall(SYS_gettid);
  // std::cout << " 	Thread " << info->tid << " runs on CPU " << sched_getcpu() << std::endl;

  int beg, end, chunksize;
  if (tid < (nSwaptions % nThreads)) {
    chunksize = nSwaptions/nThreads + 1;
    beg = tid * chunksize;
    end = (tid+1)*chunksize;
  } else {
    chunksize = nSwaptions/nThreads;
    int offsetThread = nSwaptions % nThreads;
    int offset = offsetThread * (chunksize + 1);
    beg = offset + (tid - offsetThread) * chunksize;
    end = offset + (tid - offsetThread + 1) * chunksize;
  }

  if(tid == nThreads -1 )
    end = nSwaptions;

  for(int i=beg; i < end; i++) {
     int iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice,  swaptions[i].dStrike, 
                                       swaptions[i].dCompounding, swaptions[i].dMaturity, 
                                       swaptions[i].dTenor, swaptions[i].dPaymentInterval,
                                       swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, 
                                       swaptions[i].pdYield, swaptions[i].ppdFactors,
                                       swaption_seed+i, NUM_TRIALS, BLOCK_SIZE, 0);
     assert(iSuccess == 1);
     swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
     swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
   }

  info->status = 1;
  info->termination_time = info->time_before - info->time_init;
  return (void *)0;
}


//print a little help message explaining how to use this program
void print_usage(char *name) {
  fprintf(stderr,"Usage: %s OPTION [OPTIONS]...\n", name);
  fprintf(stderr,"Options:\n");
  fprintf(stderr,"\t-ns [number of swaptions (should be > number of threads]\n");
  fprintf(stderr,"\t-sm [number of simulations]\n");
  fprintf(stderr,"\t-nt [number of threads]\n");
  fprintf(stderr,"\t-sd [random number seed]\n");
  fprintf(stderr,"\t-sc [scheduler config file]\n");
}

//Please note: Whenever we type-cast to (int), we add 0.5 to ensure that the value is rounded to the correct number. 
//For instance, if X/Y = 0.999 then (int) (X/Y) will equal 0 and not 1 (as (int) rounds down).
//Adding 0.5 ensures that this does not happen. Therefore we use (int) (X/Y + 0.5); instead of (int) (X/Y);

int main(int argc, char *argv[])
{
	int iSuccess = 0;
	int i,j;
	char * SchedConfigFile;

	FTYPE **factors=NULL;

        if(argc == 1)
        {
          print_usage(argv[0]);
          exit(1);
        }

        for (int j=1; j<argc; j++) {
        	if (!strcmp("-sm", argv[j])) {NUM_TRIALS = atoi(argv[++j]);}
        	else if (!strcmp("-nt", argv[j])) {nThreads = atoi(argv[++j]);}
        	else if (!strcmp("-ns", argv[j])) {nSwaptions = atoi(argv[++j]);}
        	else if (!strcmp("-sd", argv[j])) {seed = atoi(argv[++j]);}
        	else if (!strcmp("-sc", argv[j])) {SchedConfigFile = argv[++j];}
          else {
            fprintf(stderr,"Error: Unknown option: %s\n", argv[j]);
            print_usage(argv[0]);
            exit(1);
          }
        }

        if(nSwaptions < nThreads) {
          fprintf(stderr,"Error: Fewer swaptions than threads.\n");
          print_usage(argv[0]);
          exit(1);
        }

        printf("Number of Simulations: %d,  Number of threads: %d Number of swaptions: %d\n", NUM_TRIALS, nThreads, nSwaptions);
        swaption_seed = (long)(2147483647L * RanUnif(&seed));

	pthread_t      *threads;
	pthread_attr_t  pthread_custom_attr;

	if ((nThreads < 1) || (nThreads > MAX_THREAD))
	{
		fprintf(stderr,"Number of threads must be between 1 and %d.\n", MAX_THREAD);
		exit(1);
	}
	threads = (pthread_t *) malloc(nThreads * sizeof(pthread_t));
	pthread_attr_init(&pthread_custom_attr);

	if ((nThreads < 1) || (nThreads > MAX_THREAD))
	{
		fprintf(stderr,"Number of threads must be between 1 and %d.\n", MAX_THREAD);
		exit(1);
	}

        // initialize input dataset
	factors = dmatrix(0, iFactors-1, 0, iN-2);
	//the three rows store vol data for the three factors
	factors[0][0]= .01;
	factors[0][1]= .01;
	factors[0][2]= .01;
	factors[0][3]= .01;
	factors[0][4]= .01;
	factors[0][5]= .01;
	factors[0][6]= .01;
	factors[0][7]= .01;
	factors[0][8]= .01;
	factors[0][9]= .01;

	factors[1][0]= .009048;
	factors[1][1]= .008187;
	factors[1][2]= .007408;
	factors[1][3]= .006703;
	factors[1][4]= .006065;
	factors[1][5]= .005488;
	factors[1][6]= .004966;
	factors[1][7]= .004493;
	factors[1][8]= .004066;
	factors[1][9]= .003679;

	factors[2][0]= .001000;
	factors[2][1]= .000750;
	factors[2][2]= .000500;
	factors[2][3]= .000250;
	factors[2][4]= .000000;
	factors[2][5]= -.000250;
	factors[2][6]= -.000500;
	factors[2][7]= -.000750;
	factors[2][8]= -.001000;
	factors[2][9]= -.001250;
	
        // setting up multiple swaptions
        swaptions = (parm *)malloc(sizeof(parm)*nSwaptions);

        int k;
        for (i = 0; i < nSwaptions; i++) {
          swaptions[i].Id = i;
          swaptions[i].iN = iN;
          swaptions[i].iFactors = iFactors;
          swaptions[i].dYears = 5.0 + ((int)(60*RanUnif(&seed)))*0.25; //5 to 20 years in 3 month intervals

          swaptions[i].dStrike = 0.1 + ((int)(49*RanUnif(&seed)))*0.1; //strikes ranging from 0.1 to 5.0 in steps of 0.1
          swaptions[i].dCompounding = 0;
          swaptions[i].dMaturity = 1.0;
          swaptions[i].dTenor = 2.0;
          swaptions[i].dPaymentInterval = 1.0;

          swaptions[i].pdYield = dvector(0,iN-1);;
          swaptions[i].pdYield[0] = .1;
          for(j=1;j<=swaptions[i].iN-1;++j)
            swaptions[i].pdYield[j] = swaptions[i].pdYield[j-1]+.005;

          swaptions[i].ppdFactors = dmatrix(0, swaptions[i].iFactors-1, 0, swaptions[i].iN-2);
          for(k=0;k<=swaptions[i].iFactors-1;++k)
                 for(j=0;j<=swaptions[i].iN-2;++j)
                        swaptions[i].ppdFactors[k][j] = factors[k][j];
        }


	Scheduler scheduler(SchedConfigFile, nThreads);
	tinfo = scheduler.get_tinfo();
	// **********Calling the Swaption Pricing Routine*****************
	int threadIDs[nThreads];
        for (i = 0; i < nThreads; i++) {
          threadIDs[i] = i;
	  tinfo[i].thread_num = i;
	  tinfo[i].status = 0;
          pthread_create(&threads[i], &pthread_custom_attr, worker, &threadIDs[i]);
	  tinfo[i].thread_id = threads[i];
        }

	scheduler.run();

        for (i = 0; i < nThreads; i++) {
          pthread_join(threads[i], NULL);
        }

	free(threads);

        //for (i = 0; i < nSwaptions; i++) {
	//fprintf(stderr,"Swaption %d: [SwaptionPrice: %.10lf StdError: %.10lf] \n", 
	//         i, swaptions[i].dSimSwaptionMeanPrice, swaptions[i].dSimSwaptionStdError);
	//
        //}

        for (i = 0; i < nSwaptions; i++) {
          free_dvector(swaptions[i].pdYield, 0, swaptions[i].iN-1);
	  free_dmatrix(swaptions[i].ppdFactors, 0, swaptions[i].iFactors-1, 0, swaptions[i].iN-2);
        }


        free(swaptions);

	return iSuccess;
}
