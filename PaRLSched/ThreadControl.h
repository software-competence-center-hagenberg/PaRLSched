/*
 * ThreadControl.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Georgios Chasparis
 *      Description: This library collects functions with respect to the control of the operation of a thread, the collection of performance counters, etc.
 */

#ifndef THREADCONTROL_H_
#define THREADCONTROL_H_

#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <papi.h>

#include "ThreadInfo.h"

/**
 * ThreadControl
 *
 * It includes all the necessary functions for recording the thread performance counters
 */
class ThreadControl
{
public:
	ThreadControl();
	~ThreadControl();

	/**
	 * The function thd_initialize_counters initializes the PAPI counters
	 *
	 * @param thread_id		an integer ID of this thread
	 * @param arg			the info related to this thread
	 * @return
	 */
	bool thd_init_counters (pthread_t thread_id, void* arg);

	/**
	 * Initialize counters
	 *
	 * @param thread_id
	 * @param info
	 * @return
	 */
	bool thd_init_counters (pthread_t thread_id, thread_info& info);

	/**
	 * Stop counters
	 *
	 * @param thread_id
	 * @param info
	 * @return
	 */
	bool thd_stop_counters (const int & thread_id, thread_info& info);

	/**
	 * It records the selected counters for this thread
	 *
	 * @param thread			the ID # of this thread
	 * @param arg				the info of this thread
	 * @return
	 */
	bool thd_record_counters (pthread_t thread, void* arg);

	/**
	 * Thread record counters
	 *
	 * @param info
	 * @return
	 */
	bool thd_record_counters (thread_info& info);


private:

//	pthread_t fun_thread_id(void);
};


#endif  /* THREADCONTROL_H_ */

