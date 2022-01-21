/*
 * ThreadInfo.h
 *
 *  Created on: Apr 29, 2016
 *      Author: Georgios Chasparis
 *      Description: This library collects variables related to the creation & operation of a thread.
 */

#ifndef SRC_THREADINFO_H_
#define SRC_THREADINFO_H_

#include "MethodsActions.h"

struct thread_info
{    /* Used as argument to thread_start() */
	thread_info(){};
   pthread_t 			thread_id;         				/* ID returned by pthread_create() */
   unsigned long int	papi_thread_id;					/* PAPI thread ID */
   int       			thread_num;        				/* Application-defined thread # */
   char     			*argv_string;       			/* From command-line argument */
   int long 			total_instructions_completed;	/* Total instructions completed */
   //std::vector<Struct_Actions>		s_actions_;						/* Structure Defining Actions of the Threads  */

   bool					status;							/* Status boolean */
   double				termination_time;
   unsigned int			thread_goal;
   unsigned int			thread_ref;						/* references related to the objective */

   double				performance;
   double 				performance_before;				/* previous performance */
   bool					performance_update_ind;			/* indicator that the performance has been updated */


   double				total_completed_ins_;
   double				total_completed_ins_before_;
   double				total_completed_ins_update_ind;

   double				total_cyc_;
   double				total_cyc_before_;
   double				total_cyc_update_ind;

   double				loadstore_ins_;
   double				loadstore_ins_before_;
   double				loadstore_ins_update_index_;

   double				L1misses_;
   double				L1misses_before_;
   double				L1misses_update_index_;

   double				tlb_dm_;
   double				tlb_dm_before_;
   double				tlb_dm_update_index_;

   double				time_init;
   double				time_before;					/* this is the time of the last performance measurement */
   double 				time;
   int					EVENT_SET;						/* This is the event set of the performance counters */
   pid_t				tid;
   pid_t				pid;
   pthread_t		return_thread_id(void)
   {
	   return thread_id;
   }
   unsigned int 		memory_index;					// an index that defines which part of the memory is used

};


#endif /* SRC_THREADINFO_H_ */
