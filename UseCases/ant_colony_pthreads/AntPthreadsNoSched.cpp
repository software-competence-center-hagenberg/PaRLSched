#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <limits.h>
#include <math.h>
#include <omp.h>
//#include <numa.h>

#include <pthread.h>

struct Params {
  int lo_ant_ind;
  int hi_ant_ind;
  unsigned int *weight;
  unsigned int *deadline;
  unsigned int *process_time;
  double *tau;
};

#define const_beta 2
#define const_xi 0.1
#define const_rho 0.1
#define const_q0 0.9
#define const_max_int 2147483647
#define const_e 0.000001

unsigned int num_ants;
unsigned int num_jobs;
unsigned int **weight;
unsigned int **deadline;
unsigned int **process_time;
unsigned int **all_results;
double **tau;
unsigned int *cost;
unsigned int best_t = UINT_MAX;
unsigned int *best_result;
double t1;
unsigned int num_workers, chunk_size=1;

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

using namespace std;
//using namespace tbb;

void init(char *fname, unsigned int num_ants) {
  
  unsigned int num_tasks;
  //unsigned int numa_num_nodes = numa_num_configured_nodes();
  ifstream infile(fname);
  if (infile.is_open()) {
    infile >> num_jobs >> num_tasks;

    weight = (unsigned int **) malloc (sizeof(unsigned int *)*1);
    deadline = (unsigned int **) malloc (sizeof(unsigned int*)*1);
    process_time = (unsigned int **) malloc (sizeof(unsigned int*)*1);
    tau = (double **) malloc (sizeof(double *)*1);
    weight[0] = new unsigned int[num_jobs];
    deadline[0] = new unsigned int[num_jobs];
    process_time[0] = new unsigned int[num_jobs];
    
    for(unsigned int j=0; j<num_jobs; j++) 
      infile >> process_time[0][j];
    for(unsigned int j=0; j<num_jobs; j++)
      infile >> weight[0][j];
    for(unsigned int j=0; j<num_jobs; j++) 
      infile >> deadline[0][j];
    
    all_results = new unsigned int *[num_ants];
    for (unsigned int j=0; j<num_ants; j++) 
      all_results[j] = new unsigned int[num_jobs];
    tau[0] = new double [num_jobs*num_jobs];
    for (unsigned int j=0; j<num_jobs; j++)
      for (unsigned int i=0; i<num_jobs; i++)
	tau[0][i*num_jobs+j] = 1.0;
    best_result = new unsigned int[num_jobs];
  }
}

unsigned int solve ( unsigned int ant_id,
		     unsigned int *weight,
		     unsigned int *deadline,
		     unsigned int *process_time,
		     double *tau) {
  unsigned int cost;
  double *pij = new double[num_jobs];
  double *eta = new double[num_jobs];
  unsigned int i,j,k;
  unsigned int scheduled_time = 0;
  unsigned int remain_time = 0;
  unsigned int time = 0;
  double sumpij = 0, q, *tauk, maxp, x;
  unsigned int *tabus = new unsigned int[num_jobs];
  unsigned int *results = all_results[ant_id];
  
  memset(tabus, 1, num_jobs * sizeof(unsigned int));

  for (i = 0; i < num_jobs; i++) 
    remain_time += process_time[i];

  for (k = 0; k < num_jobs-1; k++){
    //t1 = get_current_time();
    tauk = &tau[k*num_jobs];
    for (i=0; i<num_jobs; i++) {
      if (scheduled_time + process_time[i] > deadline[i]) 
	eta[i] = 1.0 / (scheduled_time + process_time[i] );
      else 
	eta[i] = 1.0 / (deadline[i] );
    }
    for (i = 0; i < num_jobs; i++) {
      if (tabus[i] != 0) {
	pij[i] = pow(eta[i], const_beta) * tauk[i];
	sumpij += pij[i];
      } else pij[i] = 0;
    }
    
    q = ((double)rand())/RAND_MAX;
    if (q < const_q0){
      j = 0;
      maxp = pij[0];
      for (i = 1; i < num_jobs; i++)
	if (pij[i] > maxp){
	  j = i;
	  maxp= pij[i];
	}
    }
    else{
      q = ((double)rand())/RAND_MAX;
      q *= sumpij;
      double temp = 0.0;
      j = 0;
      while(temp - const_e < q && j < num_jobs ){
	temp += pij[j];
	j++;
      }
      j--;
      while ( tabus[j] == 0) j--;
    }
    results[k] = j;
    tabus[j] = 0;
    scheduled_time += process_time[j];
    remain_time -= process_time[j];
    
  }
  //	find the last job
  j = 0;
  for (i = 0; i < num_jobs; i++){
    if (tabus[i] != 0) j = i;
  }
  k = num_jobs-1;
  results[k] = j;

  for (i = 0; i < num_jobs; i++){
    j = results[i];
    time += process_time[j];
    if (time > deadline[j]) {
      cost += (time - deadline[j]) * weight[j];
    }
  }

  free( tabus);
  free( pij);
  free( eta);

  return(cost);
  
}

void *solve_worker(void *input_data) {
  unsigned int *weight = ((Params *)input_data)->weight;
  unsigned int *deadline = ((Params *)input_data)->deadline;
  unsigned int *process_time = ((Params *)input_data)->process_time;
  double *tau = ((Params *)input_data)->tau;
  int lo_ind = ((Params *)input_data)->lo_ant_ind;
  int hi_ind = ((Params *)input_data)->hi_ant_ind;
  unsigned int res;
  
  for (int i=lo_ind; i<hi_ind; i++) {
    res = solve (i, weight, deadline, process_time, tau);
  }

  return ((void *)res);
}



unsigned int fitness( unsigned int *result,
		      unsigned int *process_time,
		      unsigned int *deadline,
		      unsigned int *weight) {
  unsigned int cost = 0;
  unsigned int i,j;
  unsigned int time = 0;
  for (i = 0; i < num_jobs; i++){
    j = result[i];
    time += process_time[j];
    if (time > deadline[j]) {
      cost += (time - deadline[j]) * weight[j];
    }
  }
  return(cost);
}

void update( unsigned int best_t, unsigned int *best_result, double *tau) {
  unsigned int i,j;
  double ddd = const_rho / best_t * num_jobs;
  double dmin = 1.0 / 2.0 / num_jobs;
  
  for (i = 0; i < num_jobs; i++) {
    for (j = 0; j < num_jobs; j++) {
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

  for (i=0; i<num_ants; i++) {
      if(cost[i] < best_t) {
	best_t = cost[i];
	*best_result = all_results[i];
      }
  }
  
  return (best_t);
}

unsigned int
do_work (unsigned int nr_workers,
	 unsigned int num_ants,
	 unsigned int **weight,
	 unsigned int **deadline,
	 unsigned int **process_time,
	 double **tau)
{
  extern unsigned int *cost;
  extern unsigned int chunk_size;
  extern unsigned int best_t;
  extern unsigned int *best_result;

  Params input_data[nr_workers];
  chunk_size = num_ants / nr_workers;
  int extra_pool = num_ants - (nr_workers * chunk_size);
  int count = 0;

  pthread_t threads[nr_workers];
  
  for (int i=0; i<nr_workers; i++) {
    int extra;
    if (extra_pool>0) {
      extra = 1;
      extra_pool--;
    } else {
      extra = 0;
    }
    int chunk = chunk_size + extra;
    input_data[i].lo_ant_ind = count;
    input_data[i].hi_ant_ind = count + chunk;
    input_data[i].weight = weight[0];
    input_data[i].deadline = deadline[0];
    input_data[i].process_time = process_time[0];
    input_data[i].tau = tau[0];
    count += chunk;
  }

  for (int i=0; i<nr_workers; i++)
    pthread_create(&threads[i], NULL, solve_worker, (void *) (&input_data[i]));

  for (int i=0; i<nr_workers; i++) {
    void *ret;
    pthread_join(threads[i], &ret);
    cost[i] = *((unsigned int *)(&ret));
  }

  best_t = pick_best(&best_result);
  update(best_t, best_result, tau[0]);

}

int main(int argc, char **argv) {
  unsigned int num_iter; 
  char *fname;
  unsigned int i, j;
  unsigned int do_chunking=0;

  if (argc<6) { 
    cerr << "Usage: ant_farm <nr_workers> <nr_iterations> <nr_ants> <input_filename> [<do_chunking>]" << endl;
    exit(1);
  }
  

  num_workers = atoi(argv[1]);
  num_iter = atoi(argv[2]);
  num_ants = atoi(argv[3]);
  fname = argv[4];
  do_chunking = atoi(argv[5]);

  pthread_t threads[num_workers];
  
  init (fname, num_ants);
  cost = new unsigned int[num_ants];
  
  if (do_chunking) 
    chunk_size = num_ants / num_workers;
  
  t1 = get_current_time();
  for (j=0; j<num_iter; j++) {
	// cout << " current iteration " << j << endl;
    do_work(num_workers, num_ants, weight, deadline, process_time, tau);
  }
  t1 = get_current_time() - t1;
  cout << "Total runtime is " << t1 << " -- best solution found has cost " << best_t << endl;
}


