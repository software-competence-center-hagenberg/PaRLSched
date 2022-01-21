// Copyright (c) 2007 Intel Corp.

// Black-Scholes
// Analytical method for calculating European Options
//
// 
// Reference Source: Options, Futures, and Other Derivatives, 3rd Edition, Prentice 
// Hall, John C. Hull,

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


// Multi-threaded pthreads header
#include <pthread.h>
#include <time.h>

#ifdef SCHEDULER
#include "Scheduler.h"
#include "ThreadInfo.h"
#include "ThreadControl.h"
// #include "SchedulerParams.h"
thread_info *tinfo;
#endif

#define MAX_THREADS 128

pthread_t threadsTable[MAX_THREADS];
int threadsTableAllocated[MAX_THREADS];
pthread_mutexattr_t normalMutexAttr;
int numThreads = MAX_THREADS;

//Precision to use for calculations
#define fptype float

#define NUM_RUNS 1000

typedef struct OptionData_ {
        fptype s;          // spot price
        fptype strike;     // strike price
        fptype r;          // risk-free interest rate
        fptype divq;       // dividend rate
        fptype v;          // volatility
        fptype t;          // time to maturity or option expiration in years 
                           //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)  
        char OptionType;   // Option type.  "P"=PUT, "C"=CALL
        fptype divs;       // dividend vals (not used in this test)
        fptype DGrefval;   // DerivaGem Reference Value
} OptionData;

OptionData *data;
fptype *prices;
int numOptions;

int    * otype;
fptype * sptprice;
fptype * strike;
fptype * rate;
fptype * volatility;
fptype * otime;
int numError = 0;
int nThreads;

// Cumulative Normal Distribution Function
// See Hull, Section 11.8, P.243-244
#define inv_sqrt_2xPI 0.39894228040143270286

double getTimeSec() {
  struct timespec ts;
  int timeStat;
  
  timeStat = clock_gettime(CLOCK_MONOTONIC, &ts);

  return ts.tv_sec + ((double)ts.tv_nsec) / 1000000000.0;
}

fptype CNDF ( fptype InputX ) 
{
    int sign;

    fptype OutputX;
    fptype xInput;
    fptype xNPrimeofX;
    fptype expValues;
    fptype xK2;
    fptype xK2_2, xK2_3;
    fptype xK2_4, xK2_5;
    fptype xLocal, xLocal_1;
    fptype xLocal_2, xLocal_3;

    // Check for negative value of InputX
    if (InputX < 0.0) {
        InputX = -InputX;
        sign = 1;
    } else 
        sign = 0;

    xInput = InputX;
 
    // Compute NPrimeX term common to both four & six decimal accuracy calcs
    expValues = exp(-0.5f * InputX * InputX);
    xNPrimeofX = expValues;
    xNPrimeofX = xNPrimeofX * inv_sqrt_2xPI;

    xK2 = 0.2316419 * xInput;
    xK2 = 1.0 + xK2;
    xK2 = 1.0 / xK2;
    xK2_2 = xK2 * xK2;
    xK2_3 = xK2_2 * xK2;
    xK2_4 = xK2_3 * xK2;
    xK2_5 = xK2_4 * xK2;
    
    xLocal_1 = xK2 * 0.319381530;
    xLocal_2 = xK2_2 * (-0.356563782);
    xLocal_3 = xK2_3 * 1.781477937;
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_4 * (-1.821255978);
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_5 * 1.330274429;
    xLocal_2 = xLocal_2 + xLocal_3;

    xLocal_1 = xLocal_2 + xLocal_1;
    xLocal   = xLocal_1 * xNPrimeofX;
    xLocal   = 1.0 - xLocal;

    OutputX  = xLocal;
    
    if (sign) {
        OutputX = 1.0 - OutputX;
    }
    
    return OutputX;
} 

fptype BlkSchlsEqEuroNoDiv( fptype sptprice,
                            fptype strike, fptype rate, fptype volatility,
                            fptype time, int otype, float timet )
{
    fptype OptionPrice;

    // local private working variables for the calculation
    fptype xStockPrice;
    fptype xStrikePrice;
    fptype xRiskFreeRate;
    fptype xVolatility;
    fptype xTime;
    fptype xSqrtTime;

    fptype logValues;
    fptype xLogTerm;
    fptype xD1; 
    fptype xD2;
    fptype xPowerTerm;
    fptype xDen;
    fptype d1;
    fptype d2;
    fptype FutureValueX;
    fptype NofXd1;
    fptype NofXd2;
    fptype NegNofXd1;
    fptype NegNofXd2;    
    
    xStockPrice = sptprice;
    xStrikePrice = strike;
    xRiskFreeRate = rate;
    xVolatility = volatility;

    xTime = time;
    xSqrtTime = sqrt(xTime);

    logValues = log( sptprice / strike );
        
    xLogTerm = logValues;
        
    
    xPowerTerm = xVolatility * xVolatility;
    xPowerTerm = xPowerTerm * 0.5;
        
    xD1 = xRiskFreeRate + xPowerTerm;
    xD1 = xD1 * xTime;
    xD1 = xD1 + xLogTerm;

    xDen = xVolatility * xSqrtTime;
    xD1 = xD1 / xDen;
    xD2 = xD1 -  xDen;

    d1 = xD1;
    d2 = xD2;
    
    NofXd1 = CNDF( d1 );
    NofXd2 = CNDF( d2 );

    FutureValueX = strike * ( exp( -(rate)*(time) ) );        
    if (otype == 0) {            
        OptionPrice = (sptprice * NofXd1) - (FutureValueX * NofXd2);
    } else { 
        NegNofXd1 = (1.0 - NofXd1);
        NegNofXd2 = (1.0 - NofXd2);
        OptionPrice = (FutureValueX * NegNofXd2) - (sptprice * NegNofXd1);
    }
    
    return OptionPrice;
}

void *bs_thread(void *tinfo_ptr)
{
    int i, j;
    fptype price;
    fptype priceDelta;
#ifdef SCHEDULER
    int tid = ((thread_info *)tinfo_ptr)->thread_num;
#else
    int tid = *((int *)tinfo_ptr);
#endif
    int start = tid * (numOptions / nThreads);
    int end = start + (numOptions / nThreads);

#ifdef SCHEDULER

    ThreadControl thread_control;
    thread_info * info = (thread_info *) tinfo_ptr;
    if(!thread_control.thd_init_counters (info->thread_id, (void *)info))
      printf("Error: Problem initializing counters for thread %d",info->thread_num);
    info->tid = syscall(SYS_gettid);
#endif
    
    for (j=0; j<NUM_RUNS; j++) {
      for (i=start; i<end; i++) {
	/* Calling main function to calculate option value based on 
	 * Black & Scholes's equation.
	 */
	price = BlkSchlsEqEuroNoDiv( sptprice[i], strike[i],
				     rate[i], volatility[i], otime[i], 
				     otype[i], 0);
	prices[i] = price;
	
      }
    }
  
#ifdef SCHEDULER  
  info->status = 1;
  info->termination_time = info->time_before - info->time_init;
#endif
  
    return (void *)NULL;
}

int main (int argc, char **argv)
{
  FILE *file;
  int i;
  int loopnum;
  fptype * buffer;
  int * buffer2;
  int rv;
  
  if (argc < 4 || argc > 5 ) {
    printf("Usage:\n\t%s <nthreads> <inputFile> <outputFile> <SchedulerConfig> \n", argv[0]);
    exit(1);
  }
  nThreads = atoi(argv[1]);
  char *inputFile = argv[2];
  char *outputFile = argv[3];
  char *schedConfFile = argv[4];
  //char *schedConfFile;
  // if (argc == 5)
  //  scheduler_config = atoi(argv[4]);
  
  //Read input data from file
  file = fopen(inputFile, "r");
  if(file == NULL) {
    printf("ERROR: Unable to open file %s.\n", inputFile);
    exit(1);
  }
  rv = fscanf(file, "%i", &numOptions);
  if(rv != 1) {
    printf("ERROR: Unable to read from file %s.\n", inputFile);
    fclose(file);
    exit(1);
  }
  if(nThreads > numOptions) {
    printf("WARNING: Not enough work, reducing number of threads to match number of options.\n");
    nThreads = numOptions;
  }
  
#ifdef SCHEDULER
  printf("initializing scheduler.\n");
  Scheduler scheduler(schedConfFile, nThreads);
  // Scheduler scheduler(nThreads, os_sched);

#endif
    
  // alloc spaces for the option data
  data = (OptionData*)malloc(numOptions*sizeof(OptionData));
  prices = (fptype*)malloc(numOptions*sizeof(fptype));
  for ( loopnum = 0; loopnum < numOptions; ++ loopnum ) {
    rv = fscanf(file, "%f %f %f %f %f %f %c %f %f", &data[loopnum].s, &data[loopnum].strike, &data[loopnum].r, &data[loopnum].divq, &data[loopnum].v, &data[loopnum].t, &data[loopnum].OptionType, &data[loopnum].divs, &data[loopnum].DGrefval);
    if(rv != 9) {
      printf("ERROR: Unable to read from file %s.\n", inputFile);
      fclose(file);
      exit(1);
    }
  }
  rv = fclose(file);
  if(rv != 0) {
    printf("ERROR: Unable to close file %s.\n", inputFile);
    exit(1);
  }

  double start_time = getTimeSec();

  pthread_mutexattr_init( &normalMutexAttr);
  //    pthread_mutexattr_settype( &normalMutexAttr, PTHREAD_MUTEX_NORMAL);
  numThreads = nThreads;
  for ( i = 0; i < MAX_THREADS; i++) {
    threadsTableAllocated[i] = 0;
  }
  
  printf("Num of Options: %d\n", numOptions);
  printf("Num of Runs: %d\n", NUM_RUNS);
  
#define PAD 256
#define LINESIZE 64
  
  buffer = (fptype *) malloc(5 * numOptions * sizeof(fptype) + PAD);
  sptprice = (fptype *) (((unsigned long long)buffer + PAD) & ~(LINESIZE - 1));
  strike = sptprice + numOptions;
  rate = strike + numOptions;
  volatility = rate + numOptions;
  otime = volatility + numOptions;
  
  buffer2 = (int *) malloc(numOptions * sizeof(fptype) + PAD);
  otype = (int *) (((unsigned long long)buffer2 + PAD) & ~(LINESIZE - 1));
  
  for (i=0; i<numOptions; i++) {
    otype[i]      = (data[i].OptionType == 'P') ? 1 : 0;
    sptprice[i]   = data[i].s;
    strike[i]     = data[i].strike;
    rate[i]       = data[i].r;
    volatility[i] = data[i].v;    
    otime[i]      = data[i].t;
  }
  
  printf("Size of data: %d\n", numOptions * (sizeof(OptionData) + sizeof(int)));
  
  int *tids;
  tids = (int *) malloc (nThreads * sizeof(int));
  
#ifdef SCHEDULER
  tinfo = scheduler.get_tinfo();
#endif
  for(i=0; i<nThreads; i++) {
    tids[i]=i;
#ifdef SCHEDULER
    printf("creating details of thread %d.\n", i);


    tinfo[i].thread_num = i;
    tinfo[i].status = 0;
#endif
//    int i;
//    for ( i = 0; i < MAX_THREADS; i++)
//      if ( threadsTableAllocated[i] == 0)
//    	  break;



#ifdef SCHEDULER
    printf("initializing pthread\n");
    pthread_create(&tinfo[i].thread_id,NULL,(void *(*)(void *))bs_thread,(void *)(&tinfo[i]));
    threadsTable[i] = tinfo[i].thread_id;
#else
    pthread_create(&threadsTable[i],NULL,(void *(*)(void *))bs_thread,(void *)&tids[i]);
#endif
    threadsTableAllocated[i] = 1;
  }
  
#ifdef SCHEDULER  
  scheduler.run();
#else      
  void *ret;
  for ( i = 0; i < MAX_THREADS;i++) {
    if ( threadsTableAllocated[i] == 0)    break;
    pthread_join( threadsTable[i], &ret);
  }
#endif

  double end_time = getTimeSec();
  printf ("Completion time is %f\n", end_time - start_time);

  free(tids);
    
  //Write prices to output file
  file = fopen(outputFile, "w");
  if(file == NULL) {
    printf("ERROR: Unable to open file %s.\n", outputFile);
    exit(1);
  }
  rv = fprintf(file, "%i\n", numOptions);
  if(rv < 0) {
    printf("ERROR: Unable to write to file %s.\n", outputFile);
    fclose(file);
    exit(1);
  }
  for(i=0; i<numOptions; i++) {
    rv = fprintf(file, "%.18f\n", prices[i]);
    if(rv < 0) {
      printf("ERROR: Unable to write to file %s.\n", outputFile);
      fclose(file);
      exit(1);
    }
  }
  rv = fclose(file);
  if(rv != 0) {
    printf("ERROR: Unable to close file %s.\n", outputFile);
    exit(1);
  }
  
  free(data);
  free(prices);
  
  return 0;
}

