/*
 * ThreadControl.cpp
 *
 *  Created on: May 2, 2016
 *      Author: Georgios Chasparis
 */

#include "ThreadControl.h"


ThreadControl::ThreadControl()
{
};

ThreadControl::~ThreadControl()
{
};

pthread_mutex_t mut_init_counters = PTHREAD_MUTEX_INITIALIZER;

thread_info * object;

pthread_t wrapper(void)
{
	object->return_thread_id();
}

bool ThreadControl::thd_init_counters (pthread_t target_thread, void* arg)
{

	thread_info * info = (static_cast<thread_info*>(arg));

	std::cout << "Initializing Performance Counters for thread " << info->tid << std::endl;

	/*
	 * In this part, I was experimenting with replacing the pthread_self() command, that retrieves the ID of the thread
	 * but it may only run within the thread itself.
	 * Now the ID is retrieved through the info of each thread, and by assigning the corresponding ID to the global variable 'wrapper'
	 * This initialization works properly, which means that this function can be called outside the thread_execute function (i.e., it can be
	 * called from the scheduler). But, I haven't tried that yet.
	 */

	pthread_mutex_lock(&mut_init_counters);

	if (PAPI_thread_init(pthread_self) != PAPI_OK)
		 printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	info->EVENT_SET = PAPI_NULL;

	/* Create the Event Set */
	if (PAPI_create_eventset(&info->EVENT_SET) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(info->EVENT_SET, PAPI_TOT_INS) != PAPI_OK)	// PAPI_TOT_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Cycles to our EventSet */
	if (PAPI_add_event(info->EVENT_SET, PAPI_LST_INS) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Cycles stalled waiting for memory accesses */
//	if (PAPI_add_event(info->EVENT_SET, PAPI_TOT_CYC) != PAPI_OK) //PAPI_LST_INS
//		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* L1-Data Misses*/
	if (PAPI_add_event(info->EVENT_SET, PAPI_L1_DCM) != PAPI_OK) //PAPI_LST_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Data-Translation lookaside buffer misses*/
	if (PAPI_add_event(info->EVENT_SET, PAPI_TLB_DM) != PAPI_OK) //PAPI_LST_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

//
//	if (PAPI_add_event(info->EVENT_SET, PAPI_L1_DCM) != PAPI_OK) // L1 Data Misses
//		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
//
//	if (PAPI_add_event(info->EVENT_SET, PAPI_LD_INS) != PAPI_OK) // L1 Data Misses
//		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
//
//	if (PAPI_add_event(info->EVENT_SET, PAPI_SR_INS) != PAPI_OK) // L1 Data Misses
//		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);


	/* Start counting */
	if (PAPI_start(info->EVENT_SET) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	info->performance = 0;
	info->performance_before = 0;

	/*
	 * Initializing time
	 */
	struct timeval tim;
	gettimeofday(&tim, NULL);

	info->time_init = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info->time_before = info->time_init;

	pthread_mutex_unlock(&mut_init_counters);

	return true;
}


bool ThreadControl::thd_init_counters (pthread_t thread_id, thread_info& info)
{

	/*
	 * Experimenting with replacing the pthread_self() command, that retrieves the ID of the thread
	 * but it may only run within the thread itself.
	 * Now the ID is retrieved through the info of each thread, and by assigning the corresponding ID to the global variable 'wrapper'
	 * This initialization works properly, which means that this function can be called outside the thread_execute function (i.e., it can be
	 * called from the scheduler). However, it has not been tested properly.
	 */
	pthread_mutex_lock(&mut_init_counters);
	info.thread_id = thread_id;

//	if (PAPI_thread_init((unsigned long (*) (void)) (thread_num)) != PAPI_OK)
	if (PAPI_thread_init(pthread_self) != PAPI_OK)
		 printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	info.EVENT_SET = PAPI_NULL;

	/* Create the Event Set */
	if (PAPI_create_eventset(&info.EVENT_SET) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(info.EVENT_SET, PAPI_TOT_INS) != PAPI_OK)	// PAPI_TOT_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Cycles to our EventSet */
	if (PAPI_add_event(info.EVENT_SET, PAPI_LST_INS) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

//	/* Cycles stalled waiting for memory accesses */
//	if (PAPI_add_event(info.EVENT_SET, PAPI_TOT_CYC) != PAPI_OK) //PAPI_LST_INS
//		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* L1-Data Misses*/
	if (PAPI_add_event(info.EVENT_SET, PAPI_L1_DCM) != PAPI_OK) //PAPI_LST_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Data-Translation lookaside buffer misses*/
	if (PAPI_add_event(info.EVENT_SET, PAPI_TLB_DM) != PAPI_OK) //PAPI_LST_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);


	/* Start counting */
	if (PAPI_start(info.EVENT_SET) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}
	else
		std::cout << " PAPI started for thread_id = " << info.thread_id << std::endl;

	info.performance = 0;
	info.performance_before = 0;

	info.loadstore_ins_ = 0;
	info.loadstore_ins_before_ = 0;

	info.L1misses_ = 0;
	info.L1misses_before_ = 0;

	info.tlb_dm_ = 0;
	info.tlb_dm_before_ = 0;

	/*
	 * Initializing time
	 */
	struct timeval tim;
	gettimeofday(&tim, NULL);

	info.time_init = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info.time_before = info.time_init;

	pthread_mutex_unlock(&mut_init_counters);

	return true;
}

bool ThreadControl::thd_stop_counters (const int & thread_id, thread_info& info)
{

	std::cout << "Terminating Performance Counters " << std::endl;

	/*
	 * In this part, I was experimenting with replacing the pthread_self() command, that retrieves the ID of the thread
	 * but it may only run within the thread itself.
	 * Now the ID is retrieved through the info of each thread, and by assigning the corresponding ID to the global variable 'wrapper'
	 * This initialization works properly, which means that this function can be called outside the thread_execute function (i.e., it can be
	 * called from the scheduler). But, I haven't tried that yet.
	 */
	pthread_mutex_lock(&mut_init_counters);

// 	object = new thread_info(*info);
	long long int values[3];
	if (PAPI_stop(info.EVENT_SET, values) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
		exit(1);
	}

	pthread_mutex_unlock(&mut_init_counters);

	return true;
}



bool ThreadControl::thd_record_counters (pthread_t thread, void* arg)
{

	thread_info * info = (static_cast<thread_info*>(arg));

	if (info->status == true)
		return true;

	/* Read Performances */
	long long int values[3];
	if (PAPI_read(info->EVENT_SET, values) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	/*
	 * Retrieving the current time
	 */
	struct timeval tim;
	double current_time;
	gettimeofday(&tim, NULL);
	current_time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info->time = current_time;

	/*
	 * Updating the performance of this thread...
	 * We define performance as the total instructions completed
	 */

//	info->performance = ((double)values[0] - info->performance_before)/(info->time - info->time_before);
//	info->performance_before = (double)values[0];
//	info->performance_update_ind = true;

//	info->performance = ((double)values[0] - 1 * (double)values[1] - info->performance_before)/(info->time - info->time_before);
//	info->performance_before = (double)values[0] - 1 * (double)values[1];
//	info->performance_update_ind = true;

	info->total_completed_ins_ = ((double)values[0] - info->total_completed_ins_before_)/(info->time - info->time_before);
	info->total_completed_ins_before_ = (double)values[0];

	info->total_cyc_ = ((double)values[1]-info->total_cyc_before_)/(info->time - info->time_before);
	info->total_cyc_before_ = (double)values[1];

	info->loadstore_ins_ = ((double)values[2] - info->loadstore_ins_before_)/(info->time - info->time_before);
	info->loadstore_ins_before_ = (double)values[2];

	info->L1misses_ = ((double)values[3] - info->L1misses_before_)/(info->time - info->time_before);
	info->L1misses_before_ = (double)values[3];

	info->tlb_dm_ = ((double)values[4] - info->tlb_dm_before_)/(info->time - info->time_before);
	info->tlb_dm_before_ = (double)values[4];

//	info->performance = info->total_completed_ins_ - 10*info->L1misses_;
//	info->performance_before = info->total_completed_ins_before_ - 10*info->L1misses_before_;

	info->performance = info->total_completed_ins_;
	info->performance_before = info->total_completed_ins_before_;
	info->performance_update_ind = true;


	/* Printing out counters */
//	std::cout << " Thread : " << info->thread_num << " TOTAL instructions / sec = " << info->total_completed_ins_ << std::endl;
//	std::cout << "          " << info->thread_num << " TOTAL cycles / sec = " << info->total_cyc_  << std::endl;
//	std::cout << "          " << info->thread_num << " Load/Store instructions / sec = " << info->loadstore_ins_  << std::endl;
//	std::cout << "          " << info->thread_num << " L1 cache misses / sec = " << info->L1misses_  << std::endl;
//	std::cout << "          " << info->thread_num << " total Ins / Load-Store Ins = " << info->performance / info->loadstore_ins_  << std::endl;
//	std::cout << "          " << info->thread_num << " LoadStore Instructions / L1 misses / sec = " << info->loadstore_ins_ / info->L1misses_ << std::endl;
//	std::cout << "          " << info->thread_num << " Data Translations / LoadStore Instructions / sec = " << info->tlb_dm_ / info->loadstore_ins_ << std::endl;

	/*
	 * Updating the elapsed time
	 */
	info->termination_time += info->time - info->time_before;

	/*
	 * Updating the last recording time
	 */
	info->time_before = current_time;

	return true;
}

bool ThreadControl::thd_record_counters (thread_info& info)
{

	// std::cout << "Retrieving Performance Counters" << std::endl;
	// thread_info * info = (static_cast<thread_info*>(arg));

	if (info.status == true)
		return true;

	/* Read Performances */
	long long int values[3];
	if (PAPI_read(info.EVENT_SET, values) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	/*
	 * Retrieving the current time
	 */
	struct timeval tim;
	double current_time;
	gettimeofday(&tim, NULL);
	current_time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info.time = current_time;

	/*
	 * Updating the performance of this thread...
	 */

	info.performance = ((double)values[0] - info.performance_before)/(info.time - info.time_before);
	info.performance_before = (double)values[0];
	info.performance_update_ind = true;

	/*
	 * Updating the elapsed time
	 */
	info.termination_time += info.time - info.time_before;

	/*
	 * Updating the last recording time
	 */
	info.time_before = current_time;

	return true;
}

