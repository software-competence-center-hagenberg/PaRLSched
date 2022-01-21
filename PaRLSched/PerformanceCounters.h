/*
 * PerformanceCounters.h
 *
 *  		Created on	: Feb 11, 2016
 *      		Author	: chasparis
 *      	Description	: This library collects functions with respect to the collection of performance measurements
 */

#ifndef PERFORMANCECOUNTERS_H_
#define PERFORMANCECOUNTERS_H_

#include <papi.h>
#include <ctime>
#include <sys/time.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

/**
 * This structure captures different performance indicators of a thread
 */
struct Performance
{
	int num_performance_counters;
	double initial_tot_ins;
	double initial_tot_cyc;
	double initial_lst_ins;
	double initial_L1misses;
	double initial_tlb_dm;
	double time;
	void reset(void)
	{
		num_performance_counters = 2;
		initial_tot_ins = 0;
		initial_tot_cyc = 0;
		initial_lst_ins = 0;
		initial_L1misses = 0;
		initial_tlb_dm = 0;
		struct timeval tim;
		gettimeofday(&tim, NULL);
		time = tim.tv_sec+(tim.tv_usec/1000000.0);
	}
};

/**
 * Class PerformanceCounters
 */
class PerformanceCounters
{
public:
	PerformanceCounters();
	~PerformanceCounters();

	int initialize(void);
	void get_performance(Performance & performance);

private:

	struct timeval tim;
	int EventSet;
	long long int * values;

};



#endif /* PERFORMANCECOUNTERS_H_ */
