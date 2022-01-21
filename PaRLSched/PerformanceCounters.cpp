/*
 * PerformanceCounters.cpp
 *
 *  Created on: Feb 11, 2016
 *      Author: chasparis
 */

#include "PerformanceCounters.h"
#include <fstream>
#include <vector>
#include <iostream>

/**
 * Constructor of PerformanceCounters
 */
PerformanceCounters::PerformanceCounters(void)
{
	values = new long long int ( {0} );
	EventSet = PAPI_NULL;
}

/**
 * Destructor of PerformanceCounters
 */
PerformanceCounters::~PerformanceCounters(void)
{
	delete values;
}

/**
 * Initializing PerformanceCounters
 * @return
 */
int PerformanceCounters::initialize(void)
{

	int retval;

	/* Initialize the PAPI library */
	retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT) {
		fprintf(stderr, "PAPI library initialization error!\n");
		return(1);
	}

	/* Create the Event Set */
	if (PAPI_create_eventset(&EventSet) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet (instructions completed) */
	if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK)	// PAPI_TOT_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Cycles to our EventSet */
	if (PAPI_add_event(EventSet, PAPI_TOT_CYC) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(EventSet, PAPI_LST_INS) != PAPI_OK) //PAPI_LST_INS Load/Store Instructions Completed
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Start counting */
	if (PAPI_start(EventSet) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	return(0);

}

/**
 * Getting performance from PerformanceCounters
 *
 * @param performance
 */
void PerformanceCounters::get_performance(Performance& performance)
{

	/* Read Performances */
	if (PAPI_read(EventSet, values) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	std::cout << " test " << performance.initial_tot_ins << ", " << values[0] << " ( difference: " << (double)values[0] - performance.initial_tot_ins << " )" << std::endl;
	std::cout << " test mem " << performance.initial_lst_ins << ", " << values[2] << " ( difference: " << (double)values[2] - performance.initial_lst_ins << " )" << std::endl;

	performance.initial_tot_ins = (double)values[0] - performance.initial_tot_ins;
	performance.initial_tot_cyc = (double)values[1] - performance.initial_tot_cyc;
	performance.initial_lst_ins = (double)values[2] - performance.initial_lst_ins;
	performance.initial_L1misses = (double)values[3] - performance.initial_L1misses;
	performance.initial_tlb_dm = (double)values[4]-performance.initial_tlb_dm;

	gettimeofday(&tim, NULL);
	performance.time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0) - performance.time;

	std::cout << " tot ins + " << performance.initial_lst_ins << std::endl;
	std::cout << " tot cyc + " << performance.initial_tot_cyc << std::endl;
	std::cout << " lst_ins + " << performance.initial_tot_ins << std::endl;
	std::cout << " L1 misses + " << performance.initial_L1misses << std::endl;
	std::cout << " tlb dm + " << performance.initial_tlb_dm << std::endl;
	std::cout << " time      " << performance.time << std::endl;

}
