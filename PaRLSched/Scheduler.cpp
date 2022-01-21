/*
 * Scheduler.cpp
 *
 *  Created on: Feb 10, 2016
 *      Author: Georgios Chasparis
 *  Updated on: Aug 29, 2018
 *  	- Aug 29, 2018: Changed the reasoning on switching on new NUMA node.
 *  	  A change is only possible if the running average performance in the new NUMA node is currently larger than the current one.
 */

#include "Scheduler.h"
#include <math.h>
#include <sys/syscall.h>
#include <ctype.h>
#include <ctime>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <cfloat>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <cerrno>
#include <sched.h>
#include <assert.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

#include "ThreadControl.h"
#include "ThreadSuspendControl.h"


#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


/**
 *
 */
Scheduler::~Scheduler(void)
{
	avespeedfile_.close();
	avebalancedspeedfile_.close();
	timefile_.close();
	actionsfile_.close();
	if (write_to_files_details_)
	{
		aveperformancefile1_.close();
		aveperformancefile2_.close();
		aveperformancefile3_.close();
		aveperformancefile4_.close();
		strategiesfile1_.close();
		strategiesfile2_.close();
		strategiesfile3_.close();
		strategiesfile4_.close();
	}
};


/***
 *
 *
 */
Scheduler::Scheduler(void)
{
	num_threads_ 						= 1;
	// num_cpus_ 							= 1;
	step_size_RL_						= 0.2;
	step_size_AL_						= 0.01;
	LAMBDA_RL_							= 0.001;
	LAMBDA_AL_							= 0.02;
	factor_h_							= 0.00;

	utility_normalization_				= 1e+8;

	write_to_files_ 					= 0;
	write_to_files_details_				= 0;
	gamma_ 								= 0.02;

	cur_average_performance_ 			= 0;
	cur_balanced_performance_ 			= 0;
	num_active_threads_					= 0;
	num_active_threads_before_ 			= 0;

	printout_actions_ 					= 0;
	printout_strategies_				= 0;

	run_average_performance_ 			= 0;
	run_average_balanced_performance_ 	= 0;

	time_ 								= 0;
	time_before_ 						= 0;

	numa_switch_threshold_				= 0.8;

	RESOURCES_							= {"NULL"};
	CHILD_RESOURCES_					= {"NULL"};
	RESOURCES_OPT_METHODS_				= {"NULL"};
	CHILD_RESOURCES_OPT_METHODS_		= {"NULL"};
	RESOURCES_EST_METHODS_				= {"NULL"};
	CHILD_RESOURCES_EST_METHODS_		= {"NULL"};

	RL_mapping_							= true;
	OS_mapping_							= false;
	PR_mapping_							= false;
	ST_mapping_							= false;

	current_most_popular_node_			= 0;
	previous_most_popular_node_			= 0;
	current_popularity_					= 0;
	previous_popularity_				= 0;
	reallocate_memory_					= false;

	counter_of_threads_					= 0;

	for (unsigned int i = 0; i < num_threads_; i++)
		action_main_old_.push_back(0);


};

Scheduler::Scheduler(const Scheduler& other)
{
	ts_									= other.ts_;
	numa_sched_period_					= other.numa_sched_period_;
	zeta_								= other.zeta_;

	num_threads_ 						= other.num_threads_;
	step_size_RL_ 						= other.step_size_RL_;
	step_size_AL_						= other.step_size_AL_;
	LAMBDA_RL_ 							= other.LAMBDA_RL_;
	LAMBDA_AL_							= other.LAMBDA_AL_;

	factor_h_							= other.factor_h_;

	utility_normalization_				= other.utility_normalization_;

	write_to_files_ 					= other.write_to_files_;
	write_to_files_details_				= other.write_to_files_details_;
	gamma_ 								= other.gamma_;

	numa_switch_threshold_				= other.numa_switch_threshold_;

	cur_average_performance_ 			= other.cur_average_performance_;
	cur_balanced_performance_ 			= other.cur_balanced_performance_;
	num_active_threads_					= other.num_active_threads_;
	num_active_threads_before_ 			= other.num_active_threads_before_;

	printout_actions_ 					= other.printout_actions_;
	printout_strategies_				= other.printout_strategies_;

	run_average_performance_ 			= other.run_average_performance_;
	run_average_balanced_performance_ 	= other.run_average_balanced_performance_;

	time_ 								= other.time_;
	time_before_ 						= other.time_before_;

	RESOURCES_							= other.RESOURCES_;
	CHILD_RESOURCES_					= other.CHILD_RESOURCES_;
	RESOURCES_OPT_METHODS_				= other.RESOURCES_OPT_METHODS_;
	CHILD_RESOURCES_OPT_METHODS_		= other.CHILD_RESOURCES_OPT_METHODS_;
	RESOURCES_EST_METHODS_				= other.RESOURCES_EST_METHODS_;
	CHILD_RESOURCES_EST_METHODS_		= other.CHILD_RESOURCES_EST_METHODS_;

	MAX_NUMBER_MAIN_RESOURCES_ 			= other.MAX_NUMBER_MAIN_RESOURCES_;
	MAX_NUMBER_CHILD_RESOURCES_ 		= other.MAX_NUMBER_CHILD_RESOURCES_;

	RL_mapping_							= other.RL_mapping_;
	OS_mapping_							= other.OS_mapping_;
	PR_mapping_							= other.PR_mapping_;
	ST_mapping_							= other.ST_mapping_;

	current_most_popular_node_			= other.current_most_popular_node_;
	previous_most_popular_node_			= other.previous_most_popular_node_;
	current_popularity_					= other.current_popularity_;
	previous_popularity_				= other.previous_popularity_;
	reallocate_memory_					= other.reallocate_memory_;

	action_main_old_				 	= other.action_main_old_;

	counter_of_threads_					= other.counter_of_threads_;

	tinfo_ = (struct thread_info*) calloc(other.num_threads_+1, sizeof(struct thread_info &));
	*tinfo_ = *other.tinfo_;
	if (tinfo_ == NULL)
		handle_error("calloc");
}

Scheduler& Scheduler::operator=(const Scheduler& other)
{

	ts_									= other.ts_;
	numa_sched_period_					= other.numa_sched_period_;
	zeta_								= other.zeta_;

	num_threads_ 						= other.num_threads_;
	step_size_RL_ 						= other.step_size_RL_;
	step_size_AL_						= other.step_size_AL_;
	LAMBDA_RL_ 							= other.LAMBDA_RL_;
	LAMBDA_AL_							= other.LAMBDA_AL_;

	factor_h_							= other.factor_h_;

	utility_normalization_				= other.utility_normalization_;


	numa_switch_threshold_				= other.numa_switch_threshold_;

	write_to_files_ 					= other.write_to_files_;
	write_to_files_details_				= other.write_to_files_details_;
	gamma_ 								= other.gamma_;

	cur_average_performance_ 			= other.cur_average_performance_;
	cur_balanced_performance_ 			= other.cur_balanced_performance_;
	num_active_threads_					= other.num_active_threads_;
	num_active_threads_before_ 			= other.num_active_threads_before_;

	printout_actions_ 					= other.printout_actions_;
	printout_strategies_				= other.printout_strategies_;

	run_average_performance_ 			= other.run_average_performance_;
	run_average_balanced_performance_ 	= other.run_average_balanced_performance_;

	time_ 								= other.time_;
	time_before_ 						= other.time_before_;

	RESOURCES_							= other.RESOURCES_;
	CHILD_RESOURCES_					= other.CHILD_RESOURCES_;
	RESOURCES_OPT_METHODS_				= other.RESOURCES_OPT_METHODS_;
	CHILD_RESOURCES_OPT_METHODS_		= other.CHILD_RESOURCES_OPT_METHODS_;
	RESOURCES_EST_METHODS_				= other.RESOURCES_EST_METHODS_;
	CHILD_RESOURCES_EST_METHODS_		= other.CHILD_RESOURCES_EST_METHODS_;

	MAX_NUMBER_MAIN_RESOURCES_ 			= other.MAX_NUMBER_MAIN_RESOURCES_;
	MAX_NUMBER_CHILD_RESOURCES_ 		= other.MAX_NUMBER_CHILD_RESOURCES_;

	RL_mapping_							= other.RL_mapping_;
	OS_mapping_							= other.OS_mapping_;
	PR_mapping_							= other.PR_mapping_;
	ST_mapping_							= other.ST_mapping_;

	current_most_popular_node_			= other.current_most_popular_node_;
	previous_most_popular_node_			= other.previous_most_popular_node_;
	current_popularity_					= other.current_popularity_;
	previous_popularity_				= other.previous_popularity_;
	reallocate_memory_					= other.reallocate_memory_;

	action_main_old_				 	= other.action_main_old_;

	counter_of_threads_					= other.counter_of_threads_;

	tinfo_ = (struct thread_info*) calloc(other.num_threads_+1, sizeof(struct thread_info &));
	*tinfo_ = *other.tinfo_;
	if (tinfo_ == NULL)
		handle_error("calloc");

	return *this;
}


/***
 * Constructor of Scheduler class
 * @param config_file : path to the configuration file
 * @param thread_count : number of threads
 */
Scheduler::Scheduler(const std::string& config_file, const unsigned int& thread_count)
{
	std::cout << " Running main Scheduler !" << std::endl;
	counter_of_threads_ = 0;

	/*
	 * ADJUSTABLE PARAMETERS
	 *
	 */

	std::ifstream in_file;
	std::string line;

	std::cout << " opening file " << config_file << std::endl;
	in_file.open(config_file);

	if (in_file.fail()){
		std::cout << "opening the file failed \n";
	}

	// retrieving number of main resources (NUMA nodes)
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	bool rl_mapping = stoi(line);
	if (rl_mapping) {
	  std::cout << "Setting RL mapping with config file \n";
	  RL_mapping_ = true;
	  OS_mapping_ = false;
	} else {
	  std::cout << "Setting OS mapping\n";
	  RL_mapping_ = false;
	  OS_mapping_ = true;
	}
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	int max_number_main_resources = stoi(line);
	std::cout << " maximum number of main resources (NUMA nodes) = " << line << std::endl;
	// retrieving number of child resources for each NUMA node
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	int max_number_child_resources_numa0 = stoi(line);
	std::cout << " maximum number of child resources (CPU cores) of NUMA0 = " << line << std::endl;
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	int max_number_child_resources_numa1 = stoi(line);
	std::cout << " maximum number of child resources (CPU cores) of NUMA1 = " << line << std::endl;
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	double sched_period = std::stof(line);
	ts_ = set_scheduling_period(sched_period);
	std::cout << " scheduling period = " << sched_period << std::endl;
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	optimize_main_resource_ = std::stoi(line);
	std::cout << " optimizing main resource = " << optimize_main_resource_ << std::endl;
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	numa_sched_period_ = std::stof(line);
	std::cout << " scheduling period for main resource " << numa_sched_period_ << std::endl;
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	zeta_ = std::stof(line);
	std::cout << " zeta = " << zeta_ << std::endl;
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	numa_switch_threshold_ = std::stof(line);
	std::cout << " numa_switch_threshold " << numa_switch_threshold_ << std::endl;
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	step_size_RL_ = std::stof(line);
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	step_size_AL_ = std::stof(line);
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	utility_normalization_ = std::stof(line);
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	LAMBDA_RL_ = std::stof(line);
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	LAMBDA_AL_ = std::stof(line);
	getline(in_file, line);
	if (line[0]=='#')
		getline(in_file, line);
	gamma_ = std::stof(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	RL_active_reshuffling_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	RL_performance_reshuffling_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	printout_strategies_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	printout_actions_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	write_to_files_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	write_to_files_details_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	PR_mapping_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	ST_mapping_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	neighborhood_size_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	suspend_threads_ = std::stoi(line);
	getline(in_file, line);
	if (line[0]='#')
		getline(in_file, line);
	factor_h_ = std::stof(line);

	in_file.close();

	// Maximum allowable number of resources
	// *** Use only the first part of the brackets.
	MAX_NUMBER_MAIN_RESOURCES_ = { max_number_main_resources , 1 };		/// The first part defines the number of NUMA nodes that should be used for pinning
	MAX_NUMBER_CHILD_RESOURCES_ = { { max_number_child_resources_numa0, max_number_child_resources_numa1 } , { 0 , 0 } };		/// The first pair defines the number of CPU nodes that should be used for pinning from each NUMA node


	/**
	 *
	 * NON-ADJUSTABLE PARAMETERS
	 * (Please, do not change the following)
	 *
	 */

	/*
	 * Setting up the number of threads, action_main_old_ which records the previous action with respect to the main resource.
	 */
	num_threads_ = thread_count;

	unsigned int initial_mapping	= 0;

	/*
	 * Let us assume that we receive a vector of resources that need to be allocated (optimized) at any
	 * given time. For example, one such resource is PROCESSING_BANDWIDTH, or MEMORY, etc.
	 * This information should be automatically retrieved from the config file of the scheduler.
	 */
	RESOURCES_ = {"NUMA_PROCESSING","NUMA_MEMORY"};
	RESOURCES_EST_METHODS_ = {"AL", "RL"};
	RESOURCES_OPT_METHODS_ = {"AL", "RL"};
	CHILD_RESOURCES_ = {"CPU_PROCESSING","NULL"};
	CHILD_RESOURCES_EST_METHODS_ = {"RL", "RL"};
	CHILD_RESOURCES_OPT_METHODS_ = {"RL", "RL"};

	/*
	 * At this point, we need to also define the corresponding criteria (i.e., utility functions)
	 * based on which the optimization is performed. Such a criterion should be performed for each one
	 * of the resources to be optimized. When optimizing over Processing Bandwidth,
	 *  such a criterion may
	 * correspond to the processing speed, however alternative crit
	 * eria may be defined.
	 */
	for (unsigned int i = 0; i < num_threads_; i++)
		action_main_old_.push_back(0);

	/*
	 * NUMA API: Tests
	 */
	if(numa_available() < 0)
	{
		printf("Your system does not support NUMA API\n");
		exit(1);
	}
	printf("NUMA Nodes available = %d\n",(int)numa_max_node()+1);

	/*
	 * Initializing Variables related to the Architecture
	 */
	max_num_numa_nodes_ = (unsigned int)numa_max_node()+1;
	max_num_cpus_ = (unsigned int)numa_num_configured_cpus();
	set_cpu_nodes_per_numa_node();

	/*
	 * Initialize the PAPI library
	 */
	int retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT)
	{
		fprintf(stderr, "PAPI library initialization error!\n");
	}

	/*
	 * Allocating memory for thread information
	 */
	tinfo_ = (struct thread_info*) calloc(num_threads_+1, sizeof(struct thread_info &));
	if (tinfo_ == NULL)
		handle_error("calloc");

	/*
	 * Estimate Function
	 * For each resource that needs to be allocated, we need to define an Estimate over the
	 * estimated performance of a given source in the future. Special treatement is required when
	 * there also exists a CHILD_RESOURCE.
	 * We need to create structures of estimates for each one of the threads separately.
	 * Each thread is responsible for holding/updating this information.
	 */
	initialize_estimates();
	initialize_performancemonitoring();
	initialize_actions(initial_mapping);
	overall_Performance_.initialize(MAX_NUMBER_MAIN_RESOURCES_.size(), MAX_NUMBER_MAIN_RESOURCES_);

	/*
	 * Initializing Performance Aggregators / Statistics
	 */
	for (unsigned int t=0; t < num_threads_; t++)
	{
		vec_run_average_performances_.push_back(0);
		vec_performances_.push_back(0);
		vec_balanced_performances_.push_back(0);
		vec_min_performances_.push_back(DBL_MAX);
		vec_alg_performances_.push_back(0);

		std::vector < double > performance_per_cpu_tmp;
		/*for (unsigned int c=0; c<num_cpus; c++)
		{
			performance_per_cpu_tmp.push_back(0);
		}
		map_run_average_performances_cpu_.push_back(performance_per_cpu_tmp);
		map_run_average_balanced_performances_cpu_.push_back(performance_per_cpu_tmp);*/
	}

	run_average_performance_ = 0;
	run_average_balanced_performance_ = 0;
	cur_average_performance_ = 0;
	cur_balanced_performance_ = 0;

	/*
	 * Setting up vector of active threads
	 * @description: When the scheduler starts, we assume that all threads are active.
	 */
	for (unsigned int t = 0; t < num_threads_; t++)
	{
		vec_active_threads_.push_back(true);
	}
	active_threads_ = true;
	num_active_threads_ = num_threads_;
	num_active_threads_before_ = num_threads_;

	/*
	 * Setting CPU affinity of master thread
	 */
	cpu_set_t my_set;        /* Define your cpu_set bit mask. */
	CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
	CPU_SET(0, &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	/*
	 * Initializing Strategies for Threads
	 */
	double sum_strategies(0);

	/*
	 * Initializing Vector of Performances, Performances Indices and Actions (per thread)
	 */
	for (unsigned int t = 0; t < num_threads_; t++)
	{
		vec_performances_.push_back(0);
		vec_performances_update_inds_.push_back(false);
		// actions_.push_back(0);
	}

	struct timeval tim;
	gettimeofday(&tim, NULL);
	time_before_ = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	time_ = 0;

	// Opening files for recording performances
	std::string finalpath = "./_scheduler_outputs";
	if (mkdir(finalpath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
	{
	    if( errno == EEXIST ) {
	       // alredy exists
	    } else {
	       // something else
	        std::cout << "cannot create sessionnamefolder error:" << strerror(errno) << std::endl;
	        throw std::runtime_error( strerror(errno) );
	    }
	}
	timefile_.open("./_scheduler_outputs/time.txt",std::ios::out);
	avespeedfile_.open("./_scheduler_outputs/avespeed.txt",std::ios::out);
	avebalancedspeedfile_.open("./_scheduler_outputs/avebalancedspeedfile.txt",std::ios::out);
	actionsfile_.open("./_scheduler_outputs/actionsfile.txt",std::ios::out);

	if (write_to_files_details_)
	{
		aveperformancefile1_.open("./_scheduler_outputs/aveperformance1.txt",std::ios::out);
		aveperformancefile2_.open("./_scheduler_outputs/aveperformance2.txt",std::ios::out);
		aveperformancefile3_.open("./_scheduler_outputs/aveperformance3.txt",std::ios::out);
		aveperformancefile4_.open("./_scheduler_outputs/aveperformance4.txt",std::ios::out);
		strategiesfile1_.open("./_scheduler_outputs/strategies1.txt",std::ios::out);
		strategiesfile2_.open("./_scheduler_outputs/strategies2.txt",std::ios::out);
		strategiesfile3_.open("./_scheduler_outputs/strategies3.txt",std::ios::out);
		strategiesfile4_.open("./_scheduler_outputs/strategies4.txt",std::ios::out);
	}
}


/**
 *
 * @param num_threads
 * @param rl_mapping
 */
//Scheduler::Scheduler(const unsigned int& num_threads, bool rl_mapping)
//{
//
//	std::cout << " Running main Scheduler !" << std::endl;
//	counter_of_threads_ = 0;
//
//
//	/*
//	 * ADJUSTABLE PARAMETERS
//	 *
//	 */
//
//
//	// Maximum allowable number of resources
//	// *** Use only the first part of the brackets.
//	MAX_NUMBER_MAIN_RESOURCES_ = { 2 , 1 };		// The first part defines the number of NUMA nodes that should be used for pinning
//	MAX_NUMBER_CHILD_RESOURCES_ = { { 6, 0 } , { 0 , 0 } };		// The first pair defines the number of CPU nodes that should be used for pinning from each NUMA node
//
//	// Setting-up the Scheduler's policy (Learning-based or OS)
//	// rl_mapping = true;
//	if (rl_mapping) {
//	  std::cout << "Setting RL mapping\n";
//	  RL_mapping_ = true;
//	  OS_mapping_ = false;
//	} else {
//	  std::cout << "Setting OS mapping\n";
//	  RL_mapping_ = false;
//	  OS_mapping_ = true;
//	}
//	//RL_mapping_ = true;
//	//if (RL_mapping_)
//	//	OS_mapping_ = false;
//	//else
//	//	OS_mapping_	= true;
//
//	// Setting up the Scheduling Period
//	const double sched_period = 0.025;					// Scheduling period (originally 0.075)
//	ts_ = set_scheduling_period(sched_period);
//
//	// Parameters with respect to the memory/numa switching
//	optimize_main_resource_ 		= false;				// It currenctly refers to the NUMA node allocation. If 'false' then switching between NUMA nodes is not allowed.
//	numa_sched_period_  = 40; //40						// This is set as a multiplicative factor of the sched_period (e.g., decisions over NUMA switching are taken every 10*sched_period)
//	zeta_ = 0.5;										// if 0.5 over 1/2 of the threads have to be running on the new NUMA node destination before binding the (stack) memory of a thread to it.
//	numa_switch_threshold_ = 0.9;
//
//	/**
//	 *
//	 * NON-ADJUSTABLE PARAMETERS
//	 * (Please, do not change the following)
//	 *
//	 */
//
//	/*
//	 * Setting up the number of threads, action_main_old_ which records the previous action with respect to the main resource.
//	 */
//	num_threads_ = num_threads;
//
//	/**
//	 * Parameters w.r.t. the RL_mapping algorithm
//	 */
//	step_size_RL_ 					= 0.05;	   		// Step size of RL
//	step_size_AL_					= 0.05;    		// This step-size is used for computing the running-average performance of the AL iterations
//	utility_normalization_			= 1e+7;
//
//	LAMBDA_RL_ 						= 0.02;   		//originally 0.002
//	LAMBDA_AL_						= 0.02;
//    gamma_ 							= 0.00;
//	RL_active_reshuffling_			= true;	 		// Reshuffling when a thread becomes inactive
//	RL_performance_reshuffling_ 	= false; 		// We reshuffle the strategies when performance drops
//
//	printout_strategies_ 			= false;
//	printout_actions_ 				= true;
//	write_to_files_ 				= true;
//	write_to_files_details_ 		= false;
//
//	PR_mapping_ 					= false;
//	ST_mapping_ 					= false;
//
//	neighborhood_size_  			= 2;
//	unsigned int initial_mapping	= 0;
//
//	/*
//	 * Suspend threads
//	 */
//	suspend_threads_ 				= false;
//
//	/*
//	 * Let us assume that we receive a vector of resources that need to be allocated (optimized) at any
//	 * given time. For example, one such resource is PROCESSING_BANDWIDTH, or MEMORY, etc.
//	 * This information should be automatically retrieved from the config file of the scheduler.
//	 */
//	RESOURCES_ = {"NUMA_PROCESSING","NUMA_MEMORY"};
//	RESOURCES_EST_METHODS_ = {"AL", "RL"};
//	RESOURCES_OPT_METHODS_ = {"AL", "RL"};
//	CHILD_RESOURCES_ = {"CPU_PROCESSING","NULL"};
//	CHILD_RESOURCES_EST_METHODS_ = {"RL", "RL"};
//	CHILD_RESOURCES_OPT_METHODS_ = {"RL", "RL"};
//
//	/*
//	 * At this point, we need to also define the corresponding criteria (i.e., utility functions)
//	 * based on which the optimization is performed. Such a criterion should be performed for each one
//	 * of the resources to be optimized. When optimizing over Processing Bandwidth,
//	 *  such a criterion may
//	 * correspond to the processing speed, however alternative crit
//	 * eria may be defined.
//	 */
//	for (unsigned int i = 0; i < num_threads_; i++)
//		action_main_old_.push_back(0);
//
//	/*
//	 * NUMA API: Tests
//	 */
//	if(numa_available() < 0)
//	{
//		printf("Your system does not support NUMA API\n");
//		exit(1);
//	}
//	printf("NUMA Nodes available = %d\n",(int)numa_max_node()+1);
//
//	/*
//	 * Initializing Variables related to the Architecture
//	 */
//	max_num_numa_nodes_ = (unsigned int)numa_max_node()+1;
//	max_num_cpus_ = (unsigned int)numa_num_configured_cpus();
//	set_cpu_nodes_per_numa_node();
//
//	/*
//	 * Initialize the PAPI library
//	 */
//	int retval = PAPI_library_init(PAPI_VER_CURRENT);
//	if (retval != PAPI_VER_CURRENT)
//	{
//		fprintf(stderr, "PAPI library initialization error!\n");
//	}
//
//	/*
//	 * Allocating memory for thread information
//	 */
//	tinfo_ = (struct thread_info*) calloc(num_threads_+1, sizeof(struct thread_info &));
//	if (tinfo_ == NULL)
//		handle_error("calloc");
//
//	/*
//	 * Estimate Function
//	 * For each resource that needs to be allocated, we need to define an Estimate over the
//	 * estimated performance of a given source in the future. Special treatement is required when
//	 * there also exists a CHILD_RESOURCE.
//	 * We need to create structures of estimates for each one of the threads separately.
//	 * Each thread is responsible for holding/updating this information.
//	 */
//	initialize_estimates();
//	initialize_performancemonitoring();
//	initialize_actions(initial_mapping);
////	std::cout << "max number of main resources " << MAX_NUMBER_MAIN_RESOURCES_.size() << std::endl;
//	overall_Performance_.initialize(MAX_NUMBER_MAIN_RESOURCES_.size(), MAX_NUMBER_MAIN_RESOURCES_);
//
//	/*
//	 * Initializing Performance Aggregators / Statistics
//	 */
//	for (unsigned int t=0; t < num_threads_; t++)
//	{
//		vec_run_average_performances_.push_back(0);
//		vec_performances_.push_back(0);
//		vec_balanced_performances_.push_back(0);
//		vec_min_performances_.push_back(DBL_MAX);
//		vec_alg_performances_.push_back(0);
//
//		std::vector < double > performance_per_cpu_tmp;
//		/*for (unsigned int c=0; c<num_cpus; c++)
//		{
//			performance_per_cpu_tmp.push_back(0);
//		}
//		map_run_average_performances_cpu_.push_back(performance_per_cpu_tmp);
//		map_run_average_balanced_performances_cpu_.push_back(performance_per_cpu_tmp);*/
//	}
//
//	run_average_performance_ = 0;
//	run_average_balanced_performance_ = 0;
//	cur_average_performance_ = 0;
//	cur_balanced_performance_ = 0;
//
//	/*
//	 * Setting up vector of active threads
//	 * @description: When the scheduler starts, we assume that all threads are active.
//	 */
//	for (unsigned int t = 0; t < num_threads_; t++)
//	{
//		vec_active_threads_.push_back(true);
//	}
//	active_threads_ = true;
//	num_active_threads_ = num_threads_;
//	num_active_threads_before_ = num_threads_;
//
//	/*
//	 * Setting CPU affinity of master thread
//	 */
//	cpu_set_t my_set;        /* Define your cpu_set bit mask. */
//	CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
//	CPU_SET(0, &my_set);
//	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
//
//	/*
//	 * Initializing Strategies for Threads
//	 */
//	double sum_strategies(0);
//
//	/*
//	 * Initializing Vector of Performances, Performances Indices and Actions (per thread)
//	 */
//	for (unsigned int t = 0; t < num_threads_; t++)
//	{
//		vec_performances_.push_back(0);
//		vec_performances_update_inds_.push_back(false);
//		// actions_.push_back(0);
//	}
//
//	struct timeval tim;
//	gettimeofday(&tim, NULL);
//	time_before_ = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
//	time_ = 0;
//
//	// Opening files for recording performances
//	timefile_.open("time.txt",std::ios::out);
//	avespeedfile_.open("avespeed.txt",std::ios::out);
//	avebalancedspeedfile_.open("avebalancedspeedfile.txt",std::ios::out);
//	actionsfile_.open("actionsfile.txt",std::ios::out);
//
//	if (write_to_files_details_)
//	{
//		aveperformancefile1_.open("aveperformance1.txt",std::ios::out);
//		aveperformancefile2_.open("aveperformance2.txt",std::ios::out);
//		aveperformancefile3_.open("aveperformance3.txt",std::ios::out);
//		aveperformancefile4_.open("aveperformance4.txt",std::ios::out);
//		strategiesfile1_.open("strategies1.txt",std::ios::out);
//		strategiesfile2_.open("strategies2.txt",std::ios::out);
//		strategiesfile3_.open("strategies3.txt",std::ios::out);
//		strategiesfile4_.open("strategies4.txt",std::ios::out);
//	}
//
//};


/***
 *
 */
void Scheduler::run()
{
	sched_iteration_ = 0;

	
    // Running main control loop
	while (active_threads_)
	{

		nanosleep(&ts_, NULL);
		std::cout << " sched iteration " << sched_iteration_ << std::endl;

		active_threads_ = false;

		/*
		 * Performance Counting and Scheduling Update
		 */
		for (unsigned int r=0; r<1; r++)
			retrieve_performances(r);

		/*
		 * Performance Pre-processing
		 * This is performed for each one of the resources to be optimized
		 */
		for (unsigned int r = 0; r < 1; r++)
			performance_preprocessing(r);

		if ( active_threads_ && RL_mapping_ )
		{
			/*
			 * update
			 * @description: the update function of the scheduler should be performed for each one of the main RESOURCES
			 * @note: temporarily, we currently update only the processing of the resources.
			 * We only update the scheduling policies if the RL_mapping has been selected.
			 */
			// Updating of the main resource
			for (unsigned int r = 0; r < 1; r++)
			{
				update( r, vec_performances_update_inds_);
				// for each main resource, we also update the child resource
			}
		}

		/*
		 * Writing to files
		 * */
		write_to_files();

		/*
		 * Applying Scheduling Policy
		 */
		std::map<unsigned int, std::vector< Struct_Actions > >::iterator it_map_actions = map_Actions_per_Thread_.begin();
		apply_scheduling_policy();
		sched_iteration_++;
	}
}



/***
 * Scheduler::update
 * The main purpose of the update function of the scheduler is to perform two main functionalities
 * a) ESTIMATE (compute estimates of the performance of each one of the threads (given the resource over which optimization is perfomed)
 * b) OPTIMIZE (optimize allocations based on prior estimates or independently of such estimates)
 * It is implicitly assumed here, that alternative methodologies may be developed.
 * @param resource_ind
 * @param update_inds
 */
void Scheduler::update(const unsigned int& resource_ind, std::vector<bool>& update_inds)
{

	/*
	 * ESTIMATION
	 * Note that it makes sense to update the estimates of a thread only for the main source that is is used by the thread.
	 */
	estimate(resource_ind);

	/*
	 * OPTIMIZATION
	 */
	optimize(resource_ind);

	/*
	 * We update the number of active threads
	 * */
	num_active_threads_before_ = num_active_threads_;


}


/*
 * estimate
 */
void Scheduler::estimate(const unsigned int& resource_ind)
{

	if (printout_strategies_)
		std::cout << "~~~~~Strategies\n ";

	/*
	 * Estimates, Actions and Performances over the Main Resources (including child resources)
	 */
	std::map<unsigned int, std::vector< Struct_Actions > >::iterator it_map_actions = map_Actions_per_Thread_.begin();
	std::map<unsigned int, std::vector< Struct_Estimate > >::iterator it_map_estimates;
	std::map<unsigned int, std::vector< Struct_PerformanceMonitoring > >::iterator it_map_performances;

	bool active_threads_change(false);
	bool found_active_threads(false);

	// temporarily, we will first reproduce all the strategy updates below in the form of estimates
	// we perform the following loop over threads
	for (it_map_actions; it_map_actions != map_Actions_per_Thread_.end(); ++it_map_actions)
	{

		// we get the estimates and performances for each one of the threads
		it_map_estimates = map_Estimate_per_Thread_.find(it_map_actions->first);
		it_map_performances = map_PerformanceMonitoring_per_Thread_.find(it_map_actions->first);

		if (vec_active_threads_[it_map_actions->first] == false)
			continue;		// we only update the strategies when this thread is active!
		else
			found_active_threads = true;

		// for this thread, we get the structure of Estimates over the child resources
		unsigned int action_main = it_map_actions->second[resource_ind].action_per_main_source_;
		bool action_main_changed(false);
		if (sched_iteration_ == 1)
			action_main_old_[it_map_actions->first] = action_main;
		if (action_main != action_main_old_[it_map_actions->first]){
			std::cout << " action main " << action_main << std::endl;
			std::cout << " action main old " << action_main_old_[it_map_actions->first] << std::endl;
			action_main_changed = true;
		}

		unsigned int action_child = it_map_actions->second[resource_ind].action_per_child_source_;

		double cur_balanced_performance = it_map_performances->second[resource_ind].balanced_performance_;
		double cur_run_average_balanced_performance = it_map_performances->second[resource_ind].run_average_balanced_performance_;

		std::vector<double>& main_estimates = it_map_estimates->second[resource_ind].vec_estimates_;
		std::vector<double>& main_cummulative_estimates = it_map_estimates->second[resource_ind].vec_cummulative_estimates_;
		std::vector<double>& child_estimates = it_map_estimates->second[resource_ind].vec_child_estimates_[action_main].vec_estimates_;
		std::vector<double>& child_cummulative_estimates = it_map_estimates->second[resource_ind].vec_child_estimates_[action_main].vec_cummulative_estimates_;

		if (num_active_threads_before_ > num_active_threads_)
			active_threads_change = true;
		else
			active_threads_change = false;

		if (sched_iteration_ % numa_sched_period_ == 0 && optimize_main_resource_)
		{

			// Updating Estimates over the Main Resource
			if (RESOURCES_EST_METHODS_[resource_ind].compare("RL")==0)
			{

				methods_estimate_.RL_update(
						it_map_estimates->second[resource_ind].maximum_performance_
						, main_estimates
						, main_cummulative_estimates
						, cur_balanced_performance
						, cur_run_average_balanced_performance
						, action_main
						, false				// we do not use the 'action_main_changed' in updating the estimates of the main resource
						, step_size_RL_
						, RL_active_reshuffling_
						, RL_performance_reshuffling_
						, factor_h_
						, active_threads_change
						, it_map_actions->first
					);

			}
			else if (RESOURCES_EST_METHODS_[resource_ind].compare("AL")==0){

				methods_estimate_.AL_update(
						  cur_balanced_performance
						, it_map_performances->second[resource_ind].run_average_balanced_performance_
						, it_map_performances->second[resource_ind].run_average_balanced_performance_before_
						, step_size_AL_
						, active_threads_change
						, it_map_actions->first
						, it_map_estimates->second[resource_ind].low_benchmark_
						, it_map_estimates->second[resource_ind].high_benchmark_
						, it_map_estimates->second[resource_ind].random_switch_
						, it_map_estimates->second[resource_ind].action_change_
					);

			}
		}

		// Updating Estimates over the Child Resources
		if (CHILD_RESOURCES_EST_METHODS_[resource_ind].compare("RL")==0){

			methods_estimate_.RL_update(
					it_map_estimates->second[resource_ind].vec_child_estimates_[action_main].maximum_performance_
					, child_estimates
					, child_cummulative_estimates
					, cur_balanced_performance
					, cur_run_average_balanced_performance
					, action_child
					, action_main_changed
					, step_size_RL_
					, RL_active_reshuffling_
					, RL_performance_reshuffling_
					, factor_h_
					, active_threads_change
					, it_map_actions->first);

		}
		else if (CHILD_RESOURCES_EST_METHODS_[resource_ind].compare("AL")==0)
		{
			// this is normally not used for child resources
			methods_estimate_.AL_update(
				  cur_balanced_performance
				, cur_run_average_balanced_performance
				, cur_run_average_balanced_performance
				, step_size_AL_
				, active_threads_change
				, it_map_actions->first
				, it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].low_benchmark_
				, it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].high_benchmark_
				, it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].random_switch_
				, it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].action_change_
			);
		}

		if (printout_strategies_)
		{
			std::cout << "  - thread " << it_map_actions->first << " -- \n";
			std::cout << "  - NUMA strategies \n";
			for (unsigned int n = 0; n < main_estimates.size(); n++){
				std::cout << "      numa node " << n << " = " << main_estimates[n] << std::endl;
			}
			std::cout << "  - CPU strategies for selected NUMA node : " << action_main << std::endl;
			for (unsigned int s = 0; s < child_estimates.size(); s++){
				std::cout << "      cpu " << s << " = " << child_estimates[s] << std::endl;
			}
		}
		action_main_old_[it_map_actions->first] = action_main;
	}

	if (!found_active_threads){
		std::cout << " All threads are inactive!!!!!!!!!!\n";
		active_threads_ = false;
	}
}


/*
 * optimize
 *
 * @description: This function updates the allocations of the threads w.r.t. the optimized resources
 * For example, in the case of the RL algorithm, the estimates in the estimate function are directly
 * used to generate allocations. Alternative strategies may be employed. For example, one strategy could
 * be to play instead the action with the largest performance so far, and so on.
 */
void Scheduler::optimize(const unsigned int& resource_ind)
{

	if (printout_actions_)
		std::cout << "~~~~~Actions selected -- \n";

	// the main goal here is to define a new action profile over the selected resources.
	std::map<unsigned int, std::vector< Struct_Actions > >::iterator it_map_actions = map_Actions_per_Thread_.begin();
	std::map<unsigned int, std::vector< Struct_Estimate > >::iterator it_map_estimates;
	std::map<unsigned int, std::vector< Struct_PerformanceMonitoring > >::iterator it_map_performances;

	for (it_map_actions; it_map_actions!=map_Actions_per_Thread_.end(); ++it_map_actions)
	{
		// for each one of the (running) threads
		it_map_estimates = map_Estimate_per_Thread_.find(it_map_actions->first);
		it_map_performances = map_PerformanceMonitoring_per_Thread_.find(it_map_actions->first);

		// we only update the actions for the active threads
		if (vec_active_threads_[it_map_actions->first] == false)
			continue;

		/*
		 * Optimizing main resources
		 */
		if (sched_iteration_ % numa_sched_period_ == 0 && optimize_main_resource_ ) //|| it_map_estimates->second[resource_ind].random_switch_)
		{
			std::cout << " !!!!!!!!!!!! Update main resource !!!!!!!!!!!!!!!!! \n";
			std::cout << " sched iteration " << sched_iteration_ << std::endl;
			std::cout << " number of choices: " << it_map_actions->second[resource_ind].num_actions_main_resource_ << std::endl;
			if (RESOURCES_OPT_METHODS_[resource_ind].compare("RL") == 0)
				methods_optimize_.RL_optimize
					(
						  it_map_estimates->second[resource_ind].vec_cummulative_estimates_
						, it_map_actions->second[resource_ind].num_actions_main_resource_
						, it_map_actions->second[resource_ind].action_per_main_source_
						, LAMBDA_RL_
						, it_map_performances->second[resource_ind].run_average_balanced_performance_
					);
			else if (RESOURCES_OPT_METHODS_[resource_ind].compare("AL") == 0)
			{

				std::cout << " thread " << it_map_actions->first << " action " << it_map_actions->second[resource_ind].action_per_main_source_ << std::endl;

				methods_optimize_.AL_optimize
					(
						it_map_estimates->second[resource_ind].random_switch_
						, it_map_estimates->second[resource_ind].action_change_
						, overall_Performance_.run_average_balanced_performance_[resource_ind]
						, overall_Performance_.run_ave_balanced_performances_per_main_resource_per_action_[resource_ind]
						, it_map_estimates->second[resource_ind].low_benchmark_
						, it_map_estimates->second[resource_ind].high_benchmark_
						, it_map_actions->second[resource_ind].action_per_main_source_
						, it_map_actions->second[resource_ind].num_actions_main_resource_
						, LAMBDA_AL_
						, it_map_actions->first
						, numa_switch_threshold_
					);

				it_map_performances->second[resource_ind].run_average_balanced_performance_before_ = it_map_performances->second[resource_ind].run_average_balanced_performance_;
			}
		}

		/*
		 * Optimizing Child Resources
		 */
		if (CHILD_RESOURCES_OPT_METHODS_[resource_ind].compare("RL") == 0)
			methods_optimize_.RL_optimize
			(
			   it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].vec_cummulative_estimates_
			   , it_map_actions->second[resource_ind].vec_num_child_actions_per_main_resource_[it_map_actions->second[resource_ind].action_per_main_source_]
			   , it_map_actions->second[resource_ind].action_per_child_source_
			   , LAMBDA_RL_
			   , it_map_performances->second[resource_ind].run_average_balanced_performance_
		   );
		else if (CHILD_RESOURCES_OPT_METHODS_[resource_ind].compare("AL") == 0)
		{
			methods_optimize_.AL_optimize
			(
				it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].random_switch_
				, it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].action_change_
				, it_map_performances->second[resource_ind].run_average_balanced_performance_
				, overall_Performance_.run_ave_balanced_performances_per_main_resource_per_action_[resource_ind]
				, it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].low_benchmark_
				, it_map_estimates->second[resource_ind].vec_child_estimates_[it_map_actions->second[resource_ind].action_per_main_source_].high_benchmark_
				, it_map_actions->second[resource_ind].action_per_child_source_
				, it_map_actions->second[resource_ind].vec_num_child_actions_per_main_resource_[it_map_actions->second[resource_ind].action_per_main_source_]
				, LAMBDA_AL_
				, it_map_actions->first
				, numa_switch_threshold_
			);
		}
	}
}


void Scheduler::initialize_estimates()
{
	for (unsigned int i = 0; i < num_threads_; i++)
	{
		std::vector< Struct_Estimate > vec_Estimates_per_Thread;
		for (unsigned int r = 0; r < RESOURCES_.size(); r++)
		{
			// we create an estimate per resource
			Struct_Estimate estimate;
			if (RESOURCES_[r].compare("NUMA_PROCESSING")==0)
			{
				// in case the resource to be allocated corresponds to the NUMA_PROCESSING, then we need to also check if there is
				// a corresponding child resource, that is "CPU_PROCESSING"
				// we first need to retrieve the maximum number of NUMA nodes available

				if (CHILD_RESOURCES_[r].compare("CPU_PROCESSING")==0)
					// for each one of the numa nodes, we need to get the maximum number of CPU nodes
					estimate.initialize_w_child ( RESOURCES_[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r],
							CHILD_RESOURCES_[r], cpu_nodes_per_numa_node_, MAX_NUMBER_CHILD_RESOURCES_[r], i );
				else
					estimate.initialize(RESOURCES_[r],max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r], 0, true);
			}
			else if ( RESOURCES_[r].compare("NUMA_MEMORY")==0 )
				estimate.initialize( RESOURCES_[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r], 0, true);
			vec_Estimates_per_Thread.push_back ( estimate );
		}
		map_Estimate_per_Thread_.insert(std::pair< unsigned int, std::vector< Struct_Estimate > > ( i, vec_Estimates_per_Thread));
	}
}


void Scheduler::initialize_performancemonitoring()
{
	for (unsigned int i = 0; i < num_threads_; i++){
		std::vector < Struct_PerformanceMonitoring > vec_Performances_per_Thread;
		for (unsigned int r = 0; r < RESOURCES_.size(); r++)
		{
			// we create a PerformanceMonitoring structure per resource optimized
			Struct_PerformanceMonitoring performancemonitoring;
			if (RESOURCES_[r].compare("NUMA_PROCESSING") == 0)
			{
				if (CHILD_RESOURCES_[r].compare("CPU_PROCESSING") == 0)
					performancemonitoring.initialize_w_child( RESOURCES_[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r],
							CHILD_RESOURCES_[r], cpu_nodes_per_numa_node_, MAX_NUMBER_CHILD_RESOURCES_[r]  );
				else
					performancemonitoring.initialize(RESOURCES_[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r]);
			}
			else if (RESOURCES_[r].compare("NUMA_MEMORY") == 0)
				performancemonitoring.initialize(RESOURCES_[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r]);
			vec_Performances_per_Thread.push_back( performancemonitoring );
		}
		map_PerformanceMonitoring_per_Thread_.insert(std::pair< unsigned int, std::vector< Struct_PerformanceMonitoring > >( i , vec_Performances_per_Thread));
	}
}


void Scheduler::initialize_actions(const unsigned int& policy)
{
	const std::vector< std::string > resources = RESOURCES_;
	const std::vector< std::string > child_resources = CHILD_RESOURCES_;

	for (unsigned int i = 0; i < num_threads_; i++)
	{
		std::vector< Struct_Actions > vec_Actions_per_Thread;
		for (unsigned int r = 0; r < resources.size(); r++)
		{
			Struct_Actions actions;
			if (resources[r].compare("NUMA_PROCESSING") == 0){
				if (child_resources[r].compare("CPU_PROCESSING") == 0)
					actions.initialize_w_child ( resources[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r], child_resources[r],
							cpu_nodes_per_numa_node_, MAX_NUMBER_CHILD_RESOURCES_[r], i, action_main_old_, policy, neighborhood_size_ );
				else
					actions.initialize(RESOURCES_[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r], action_main_old_, i);
			}
			else if (RESOURCES_[r].compare("NUMA_MEMORY") == 0)
				actions.initialize(resources[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r], action_main_old_, i);
			vec_Actions_per_Thread.push_back ( actions );
		}

		map_Actions_per_Thread_.insert(std::pair< unsigned int, std::vector< Struct_Actions > > (i , vec_Actions_per_Thread));

	}
}


/*
 * retrieve_performances()
 * @description: this function retrieves the  performances for each one of the threads and for each one of the main resources
 */
void Scheduler::retrieve_performances(const unsigned int& resource_ind)
{

	/*
	 * For now, let us assume that we one performance counter for each one of the main resources.
	 * In other words, for now the resource_ind does not have any impact.
	 *
	 * However, we need to find an interface for thd_record_counters function, to accept the main resource as an input.
	 */
	if (printout_strategies_)
		std::cout << "~~~~~Performances\n";

	bool found_active_thread = false;
	ThreadControl thread_control;
	std::map<unsigned int, std::vector<Struct_PerformanceMonitoring>>::iterator it_map_performances = map_PerformanceMonitoring_per_Thread_.begin();
	for ( it_map_performances; it_map_performances!=map_PerformanceMonitoring_per_Thread_.end(); ++it_map_performances )
	{
		// for each one of the threads
		unsigned int thread_counter = it_map_performances->first;
		if (!thread_control.thd_record_counters(tinfo_[thread_counter].thread_id,&tinfo_[thread_counter]))
			printf("Error: Problem recording counters for thread %d", (int)tinfo_[thread_counter].thread_id);

		//double performance_tmp = (tinfo_[thread_counter].performance - tinfo_[thread_counter].loadstore_ins_) + 5*tinfo_[thread_counter].loadstore_ins_;

		it_map_performances->second[resource_ind].performance_ = tinfo_[thread_counter].performance / utility_normalization_;

		//it_map_performances->second[resource_ind].performance_ = performance_tmp / utility_normalization_;

		//it_map_performances->second[resource_ind].performance_ = (tinfo_[thread_counter].performance - tinfo_[thread_counter].tlb_dm_) / utility_normalization_;

		it_map_performances->second[resource_ind].performance_update_ind_ = tinfo_[thread_counter].performance_update_ind;
		//if (tinfo_[thread_counter].status == 0 && it_map_performances->second[resource_ind].run_average_performance_ > 0.001){
		if (sched_iteration_ >= 100){
			if (tinfo_[thread_counter].status == false && it_map_performances->second[resource_ind].run_average_performance_ > 1e-5)
			//if (it_map_performances->second[resource_ind].run_average_performance_ > 1e-12)
			{
				// if the status is 'incomplete' and the performance is non-zero, then we consider the thread 'active'
				vec_active_threads_[thread_counter] = true;	// the thread has not completed its task.
				found_active_thread = true;
			}
			else if ((tinfo_[thread_counter].status == false && it_map_performances->second[resource_ind].run_average_performance_ <= 1e-5))
				vec_active_threads_[thread_counter] = true;
			else
				vec_active_threads_[thread_counter] = true;
		}
		else
		{
			vec_active_threads_[thread_counter] = true;
			found_active_thread = true;
			active_threads_ = true;
		}
//		std::cout << " performance / running average performance of thread " << thread_counter << " is ("
//				<< it_map_performances->second[resource_ind].performance_ << " / " << it_map_performances->second[resource_ind].run_average_performance_ << " ) " << std::endl;
	}

	if (!found_active_thread)
	{
		active_threads_ = false;
		std::cout << " all threads are inactive \n";

	}
	else
		active_threads_ = true;
}


void Scheduler::performance_preprocessing(const unsigned int& resource_ind)
{
	/*
	 * Performance Pre-Processing
	 * Description: The purpose of this function is to perform necessary pre-processing of the available performances
	 * (e.g., computing the average, the running average, etc.)
	 *
	 */

	double sum_performances = 0;
	double ave_performance = 0;
	double sum_balanced_performances = 0;
	double ave_balanced_performance = 0;

	/* Parameters of overall performance */
	std::vector<unsigned int> num_threads_per_main_resource;
	for (unsigned int a = 0; a < MAX_NUMBER_MAIN_RESOURCES_[resource_ind]; a++)
	{
		num_threads_per_main_resource.push_back(0);
	}
	std::vector<double> sum_balanced_performances_per_main_resource;
	std::vector<double> ave_balanced_performances_per_main_resource;
	for (unsigned int a = 0; a < MAX_NUMBER_MAIN_RESOURCES_[resource_ind]; a++)
	{
		sum_balanced_performances_per_main_resource.push_back(0);
		ave_balanced_performances_per_main_resource.push_back(0);
	}

	struct timeval tim;
	gettimeofday(&tim, NULL);

	double current_time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	time_ = time_ + ( current_time - time_before_);
	time_before_ = current_time;

	num_active_threads_ = 0;

	/*
	 * Computing the Sum of Performances
	 */
	std::map< unsigned int, std::vector< Struct_PerformanceMonitoring > >::iterator it_performance = map_PerformanceMonitoring_per_Thread_.begin();
	std::map< unsigned int, std::vector< Struct_Actions > >::iterator map_Actions_it;

	for (it_performance; it_performance != map_PerformanceMonitoring_per_Thread_.end(); ++it_performance)
	{
		// for each one of the threads
		map_Actions_it = map_Actions_per_Thread_.find(it_performance->first);		// we retrieve its action

//		std::cout << " running average performance " << it_performance->second[resource_ind].run_average_performance_ << std::endl;

		if (vec_active_threads_[it_performance->first] == true)
		{
			num_active_threads_ += 1;
			// I update the performances, although for some might be the same.
			// The idea here is that if the thread is active we take into account its performance
			// for strategy update.
			// vec_performances_[t] = vec_performances_[t]/1e+8;
			sum_performances += it_performance->second[resource_ind].performance_;
			// updating the running average performance
			it_performance->second[resource_ind].update_run_average_performance(it_performance->second[resource_ind].performance_, step_size_AL_, sched_iteration_);
			// updating the running average performance per main resource
			it_performance->second[resource_ind].update_run_average_performance_per_main_resource(it_performance->second[resource_ind].performance_,
					map_Actions_it->second[resource_ind].action_per_main_source_, step_size_AL_ , sched_iteration_);
			// updating the running average performance per child resource
			it_performance->second[resource_ind].update_run_average_performance_per_child_resource(it_performance->second[resource_ind].performance_,map_Actions_it->second[resource_ind].action_per_main_source_,
					map_Actions_it->second[resource_ind].action_per_child_source_, step_size_AL_, sched_iteration_);

//			std::cout << " this is the running average performance of thread " << it_performance->first << " on CPU " << map_Actions_it->second[resource_ind].action_per_child_source_ <<
//					" = " << it_performance->second[resource_ind].vec_run_average_performances_per_child_resource_[map_Actions_it->second[resource_ind].action_per_main_source_][map_Actions_it->second[resource_ind].action_per_main_source_] <<
//					std::endl;
		}
	}



	// average performance
	if (num_active_threads_ > 0)
		ave_performance = sum_performances / (double)num_active_threads_;
	else
		ave_performance = sum_performances;

	// balanced performance
	it_performance = map_PerformanceMonitoring_per_Thread_.begin();
	for (it_performance; it_performance != map_PerformanceMonitoring_per_Thread_.end(); ++it_performance)
	{
		// for each one of the running threads
		map_Actions_it = map_Actions_per_Thread_.find(it_performance->first);

		if (vec_active_threads_[it_performance->first] == true)
		{
			// I update the performances, although for some might be the same.
			// The idea here is that if the thread is active we take into account its performance
			// for strategy update.
			// vec_performances_[t] = vec_performances_[t]/1e+8;
			it_performance->second[resource_ind].balanced_performance_ = it_performance->second[resource_ind].performance_ - gamma_ * pow(it_performance->second[resource_ind].performance_ - ave_performance, 2);
			sum_balanced_performances += it_performance->second[resource_ind].performance_ - gamma_ * pow(it_performance->second[resource_ind].performance_ - ave_performance, 2);
			// updating the running average balanced performance
			it_performance->second[resource_ind].update_run_average_balanced_performance(it_performance->second[resource_ind].balanced_performance_, step_size_AL_ , sched_iteration_);

			num_threads_per_main_resource[map_Actions_it->second[resource_ind].action_per_main_source_]++;
			sum_balanced_performances_per_main_resource[map_Actions_it->second[resource_ind].action_per_main_source_] += it_performance->second[resource_ind].balanced_performance_;

		}
	}

	if (num_active_threads_ > 0)
		ave_balanced_performance = sum_balanced_performances/(double)num_active_threads_;
	else
		ave_balanced_performance = sum_balanced_performances;


	/* Updating ave balanced performance per main resources actions
	 *
	 *
	 */
	for (unsigned int a = 0; a < MAX_NUMBER_MAIN_RESOURCES_[resource_ind]; a++)
	{
		if (num_threads_per_main_resource[a] > 0){
			ave_balanced_performances_per_main_resource[a] = sum_balanced_performances_per_main_resource[a] / num_threads_per_main_resource[a];

			if (ave_balanced_performances_per_main_resource[a] < 0)
				ave_balanced_performances_per_main_resource[a] = 0;

			overall_Performance_.update_run_average_balanced_performances_per_main_resource_per_action(resource_ind, a, ave_balanced_performances_per_main_resource[a], sched_iteration_ );
		}
	}

	/*
	 * Updating the Overall Performance
	 */
	overall_Performance_.update_sum_performances_per_main_resource(resource_ind, sum_performances);
	overall_Performance_.update_ave_performance_per_main_resource(resource_ind, ave_performance);

	overall_Performance_.update_sum_balanced_performances_per_main_resource(resource_ind, sum_balanced_performances);
	overall_Performance_.update_ave_balanced_performance_per_main_resource(resource_ind, ave_balanced_performance);

	overall_Performance_.update_run_average_performance_per_main_resource(resource_ind, ave_performance, sched_iteration_);
	overall_Performance_.update_run_average_balanced_performance_per_main_resource(resource_ind, ave_balanced_performance, sched_iteration_);

	overall_Performance_.update_num_threads_using_main_resource_per_action(resource_ind, num_threads_per_main_resource);
	overall_Performance_.update_sum_balanced_performances_per_main_resource_per_action(resource_ind, sum_balanced_performances_per_main_resource);
	overall_Performance_.update_ave_balanced_performances_per_main_resource_per_action(resource_ind, ave_balanced_performances_per_main_resource);

	/*
	 * Updating Overall Performances to each one of the threads
	 */
	it_performance = map_PerformanceMonitoring_per_Thread_.begin();

	for (it_performance; it_performance != map_PerformanceMonitoring_per_Thread_.end(); ++it_performance)
	{
		map_Actions_it = map_Actions_per_Thread_.find(it_performance->first);

		if (vec_active_threads_[it_performance->first] == true)
		{
			num_active_threads_ += 1;
			it_performance->second[resource_ind].overall_performance_ = overall_Performance_.ave_performance_per_main_resource_[resource_ind];
			it_performance->second[resource_ind].overall_balanced_performance_ = overall_Performance_.ave_balanced_performance_per_main_resource_[resource_ind];
		}
	}

	std::cout << "~~~~~Average Performances \n";
	std::cout << "  current average performance : " << overall_Performance_.ave_performance_per_main_resource_[resource_ind] << ", run average perf. " << overall_Performance_.run_average_performance_[resource_ind] << std::endl;
	std::cout << "  current balanced performance : " << overall_Performance_.ave_balanced_performance_per_main_resource_[resource_ind] << ", run average balanced perf. " << overall_Performance_.run_average_balanced_performance_[resource_ind] << std::endl;

	if (RESOURCES_[resource_ind].compare("NUMA_PROCESSING") == 0){
		run_average_performance_ = overall_Performance_.run_average_performance_[resource_ind];
		run_average_balanced_performance_ = overall_Performance_.run_average_balanced_performance_[resource_ind];
	}
}


void Scheduler::apply_scheduling_policy()
{

	/*
	 * This function applies the scheduling action derived from optimize()
	 *
	 * a) Assigning main resource (e.g., NUMA node for processing)
	 * b) Assigning child resource (e.g., CPU node for processing)
	 */
	std::vector<unsigned int> num_threads_per_resource;
	for (unsigned int main_r=0;main_r<RESOURCES_.size(); main_r++)
	{
		if ((RESOURCES_[main_r].compare("NUMA_PROCESSING") == 0))
		{
			for (unsigned int source=0; source < max_num_numa_nodes_; source++)
			{
				num_threads_per_resource.push_back(0);
			}
		}
	}


	bool found_most_popular_node(false);
	reallocate_memory_ = false;

	/*
	 * We first find which one of the nodes is the most popular
	 */
	if (RL_mapping_)
	{

		unsigned int new_main_action;
		unsigned int new_numa_node(0);
		unsigned int new_child_action;
		std::vector< unsigned int > new_cpu_node;
		unsigned int previous_numa_node;
		unsigned int previous_cpu_node;

		for (unsigned int i = 0; i < num_threads_; i++)
		{
			for (unsigned int r = 0; r < RESOURCES_.size(); r++)
			{
								// for each one of the main resources
				std::map<unsigned int, std::vector< Struct_Actions> >::iterator it_action = map_Actions_per_Thread_.find(i);

				if ((RESOURCES_[r].compare("NUMA_PROCESSING") == 0) && (CHILD_RESOURCES_[r].compare("CPU_PROCESSING") == 0))
				{

					// we perform all necessary actions for assigning the new NUMA node
					// the action that needs to be implemented is:
					new_main_action = it_action->second[r].action_per_main_source_;
					new_numa_node = new_main_action;
					new_child_action = it_action->second[r].action_per_child_source_;
					new_cpu_node = {it_action->second[r].vec_child_sources_[new_main_action][new_child_action]};

					/*
					 * Computing the most popular NUMA node (used for memory allocation)
					 */
					if (ST_mapping_)
					{
						num_threads_per_resource[new_numa_node]++;

						if (num_threads_per_resource[new_numa_node] >= ceil( zeta_ * num_threads_ ) && !found_most_popular_node)	// > ceil(2.7 * num_threads_ / 3)
						{
							// if for this thread, the new_numa_node is larger than the half of the threads,
							// then we will need to reallocate memory for this thread, or consider it.
							found_most_popular_node = true;
							current_most_popular_node_ = new_numa_node;
							current_popularity_ = num_threads_per_resource[new_numa_node];
							if (current_most_popular_node_ != previous_most_popular_node_){
								reallocate_memory_ = true;
							}

						}
						else if (num_threads_per_resource[new_numa_node] >= ceil( zeta_ * num_threads_ ) && found_most_popular_node)		// > ceil( 2.7 * num_threads_ / 3 )
						{
							current_popularity_ = num_threads_per_resource[new_numa_node];
						}
						else{
							if ( i == num_threads_-1 && !found_most_popular_node)	// if currently processing the last thread, and still has not found a popular node, then we set it to undetermined...
								current_most_popular_node_ = -1;					// in this case the most_popular_node is undetermined

							reallocate_memory_ = false;
						}

						if (current_popularity_ > previous_popularity_ && current_most_popular_node_ == previous_most_popular_node_)
							reallocate_memory_ = true;

					}

					// updating the memory index of the thread
					tinfo_[i].memory_index = new_numa_node;

					// update the previous most popular node
					if ( ST_mapping_ && i == num_threads_ -1 ){
						previous_most_popular_node_ = current_most_popular_node_;
						previous_popularity_ = current_popularity_;
					}
				}
			}
		}
	}


	/*
	 * Assigning processing node
	 */
	// temporary variable, it is used for testing purposes (randomly picks the numa node of a group of agents)
	int random_numa_node = rand() % 2;

	for (unsigned int i = 0; i < num_threads_; i++)
	{

		if ( tinfo_[i].status == 0 ){
			/*
			 * Suspending thread before setting its affinity
			 */
			if (suspend_threads_){
				printf ("Suspending thread %d.\n", i);
				if (thd_suspend (tinfo_[i].thread_id) != 0){
					printf ("%s:%d\t ERROR: Suspend thread failed!\n", __FILE__, __LINE__);
				}
			}

			/*
			 * Setting its CPU affinity
			 */
			unsigned int new_main_action;
			unsigned int new_numa_node(0);
			unsigned int new_child_action;
			std::vector< unsigned int > new_cpu_node;
			unsigned int previous_numa_node;
			unsigned int previous_cpu_node;

//			std::cout << " RL_mapping is " << RL_mapping_ << std::endl;

			if (RL_mapping_)
			{

				for (unsigned int r = 0; r < RESOURCES_.size(); r++){
					// for each one of the main resources
					std::map<unsigned int, std::vector< Struct_Actions> >::iterator it_action = map_Actions_per_Thread_.find(i);

					if ((RESOURCES_[r].compare("NUMA_PROCESSING") == 0) && (CHILD_RESOURCES_[r].compare("CPU_PROCESSING") == 0))
					{

						// we perform all necessary actions for assigning the new NUMA node
						// the action that needs to be implemented is:
						new_main_action = it_action->second[r].action_per_main_source_;
						new_numa_node = new_main_action;
						new_child_action = it_action->second[r].action_per_child_source_;

						std::map<std::tuple<unsigned int, unsigned int, unsigned int>, std::vector<unsigned int>>::iterator neighbors =
							it_action->second[r].map_neighbors_.find(std::tuple<unsigned int,unsigned int,unsigned int>(it_action->first, new_main_action, it_action->second[r].vec_child_sources_[new_main_action][new_child_action]));
						if (neighbors != it_action->second[r].map_neighbors_.end())
							new_cpu_node = neighbors->second; 	// we assign the neighborhood as an action
						else{
							std::cout << "   new child action " << new_child_action << std::endl;
							new_cpu_node = {it_action->second[r].vec_child_sources_[new_main_action][new_child_action]};
						}
						// new_cpu_node = {it_action->second[r].vec_child_sources_[new_main_action][new_child_action]};

//						if (ST_mapping_)
//						{
//							num_threads_per_resource[new_numa_node]++;
//
//	//						std::cout << " number of threads for new numa node " << num_threads_per_resource[new_numa_node] << " and found_most_popular_node_ind = " << found_most_popular_node << std::endl;
//
//							if (num_threads_per_resource[new_numa_node] >= ceil(2 * num_threads_ / 3) && !found_most_popular_node)
//							{
//								// if for this thread, the new_numa_node is larger than the half of the threads,
//								// then we will need to reallocate memory for this thread, or consider it.
//								found_most_popular_node = true;
//								current_most_popular_node_ = new_numa_node;
//								current_popularity_ = num_threads_per_resource[new_numa_node];
//	//							std::cout << " current most popular node " << current_most_popular_node_ << " and previous one " << previous_most_popular_node_ << std::endl;
//								if (current_most_popular_node_ != previous_most_popular_node_){
//									reallocate_memory_ = true;
//								}
//								else
//									reallocate_memory_ = false;
//
//							}
//							else if (num_threads_per_resource[new_numa_node] >= ceil(2 * num_threads_ / 3) && found_most_popular_node)
//							{
//								current_popularity_ = num_threads_per_resource[new_numa_node];
//							}
//							else{
//								if ( i == num_threads_-1 && !found_most_popular_node)	// if currently processing the last thread, and still has not found a popular node, then we set it to undetermined...
//									current_most_popular_node_ = -1;					// in this case the most_popular_node is undetermined
//
//								reallocate_memory_ = false;
//							}
//
//							if (current_popularity_ > previous_popularity_ && current_most_popular_node_ == previous_most_popular_node_)
//								reallocate_memory_ = true;
//
//						}

						previous_numa_node = it_action->second[r].previous_action_per_main_source_;
						previous_cpu_node = it_action->second[r].previous_action_per_child_source_;

						/*
						 * Assigning processing node, which also includes memory allocations
						 */
						assign_processing_node(i, new_numa_node, previous_numa_node, new_cpu_node, previous_cpu_node);

						// updating the memory index of the thread
						tinfo_[i].memory_index = new_numa_node;

						// updating the old action
						it_action->second[r].previous_action_per_main_source_ = new_numa_node;
						it_action->second[r].previous_action_per_child_source_ = new_cpu_node[0];

//						// update the previous most popular node
//						if ( ST_mapping_ && i == num_threads_ -1 ){
//							previous_most_popular_node_ = current_most_popular_node_;
//							previous_popularity_ = current_popularity_;
//							reallocate_memory_ = false;
//						}

					}
				}
			}
			else if (OS_mapping_)
			{

				for (unsigned int r = 0; r < RESOURCES_.size(); r++){
					// for each one of the main resources
					std::map<unsigned int, std::vector< Struct_Actions> >::iterator it_action = map_Actions_per_Thread_.find(i);
					if ((RESOURCES_[r].compare("NUMA_PROCESSING") == 0) && (CHILD_RESOURCES_[r].compare("CPU_PROCESSING") == 0))
					{
						// we perform all necessary actions for assigning the new NUMA node
						// the action that needs to be implemented is:
						std::vector< unsigned int > cpu_nodes;
						for (unsigned int main_source = 0; main_source < it_action->second[r].vec_child_sources_.size(); main_source++){
							for (unsigned int child_source = 0; child_source < it_action->second[r].vec_child_sources_[main_source].size(); child_source ++)
								cpu_nodes.push_back(it_action->second[r].vec_child_sources_[main_source][child_source]);
						}
						assign_processing_node(i, new_numa_node, previous_numa_node, cpu_nodes, previous_cpu_node);
					}
				}
			}
			else if (PR_mapping_)
			{
				new_cpu_node = {(i+1)%10 + random_numa_node * 10};
				previous_cpu_node = 0;
				new_numa_node = random_numa_node;
				previous_numa_node = 0;
				assign_processing_node(i, new_numa_node, previous_numa_node, new_cpu_node, previous_cpu_node);
			}


			/*
			 * Continuing running thread after setting its affinity
			 */
			if ( suspend_threads_ ){
				printf ("Continuing thread %d.\n", i);
				if (thd_continue (tinfo_[i].thread_id) != 0){
					printf ("%s:%d\t ERROR: Continuing thread failed!\n", __FILE__, __LINE__);
				}
			}

		}
		else{
			vec_active_threads_[i] = false;
			std::cout << " Status of thread DD " << i << ": FINISHED!" << " ( time = " << tinfo_[i].termination_time << " )" << std::endl;
		}


	} // end of applying scheduling policy


	// assign memory
//	if (RL_mapping_ && ST_mapping_)
//	{
//		previous_most_popular_node_ = current_most_popular_node_;
//		std::vector<unsigned int>::iterator most_popular_node_population = std::max_element(num_threads_per_resource.begin(),num_threads_per_resource.end());
//		unsigned int most_popular_node = std::distance(num_threads_per_resource.begin(),most_popular_node_population);
////		for (unsigned int i = 0; i < num_threads_; i++)
////		{
////			if (!vec_active_threads_[i])
////				continue;
////			std::map<unsigned int, std::vector< Struct_Actions> >::iterator it_action = map_Actions_per_Thread_.find(i);
////			unsigned int new_main_action = it_action->second[0].action_per_main_source_;
////			unsigned int new_numa_node = new_main_action;
////			unsigned int previous_numa_node = it_action->second[0].previous_action_per_main_source_;
////			if (most_popular_node != new_numa_node && most_popular_node != current_most_popular_node_)
////				assign_memory_node(i, previous_numa_node, new_numa_node);
////		}
//		current_most_popular_node_ = most_popular_node;
//		if (current_most_popular_node_ != previous_most_popular_node_)
//			reallocate_memory_ = true;
//		else
//			reallocate_memory_ = false;
//
//	}
}


/*
 * Scheduler::assign_processing_node
 */
void Scheduler::assign_processing_node(const unsigned int& thread,
		const unsigned int& numa_node,
		const unsigned int & previous_numa_node,
		const std::vector< unsigned int >& cpu_node,
		const unsigned int & previous_cpu_node)
{

	cpu_set_t mask;
	CPU_ZERO (&mask);

	struct bitmask * numa_old_nodes;
	struct bitmask * numa_new_nodes;

	numa_old_nodes = numa_bitmask_alloc(max_num_numa_nodes_);
	numa_new_nodes = numa_bitmask_alloc(max_num_numa_nodes_);
	numa_bitmask_setbit( numa_old_nodes, previous_numa_node);
	numa_bitmask_setbit( numa_new_nodes, numa_node);


	for (unsigned int i = 0; i < cpu_node.size(); i++)
		CPU_SET( cpu_node[i] , &mask);

	/*
	 * bind process to processor
	 * */
	if (pthread_setaffinity_np(tinfo_[thread].thread_id, sizeof(mask), &mask) <0)
	{
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	if (RL_mapping_)
	{

		// We move the stack to the new node:
		if (ST_mapping_ == 1 && current_most_popular_node_ == numa_node && numa_node != previous_numa_node && reallocate_memory_)
		{

			pthread_attr_t attr;
			void *stackAddr = nullptr;
			size_t stackSize = 0;
			if ((0 != pthread_getattr_np(tinfo_[thread].thread_id, &attr)) || (0 != pthread_attr_getstack(&attr, &stackAddr, &stackSize))) {
				assert(false);
			}

			std::cout << " stack size : " << stackSize << " and stack Address : " << stackAddr << std::endl;
			std::cout << " allocating memory for thread: " << thread << std::endl;


			/*
			 * The following attempts to move (non-shared) memory of the thread allocated in "stack" to the specified node-mask.
			 */
			const long unsigned int nodeMask = numa_node; // numa_node_of_cpu(cpu_node[0]);
			const auto bindRc = mbind(stackAddr, stackSize, MPOL_PREFERRED, &nodeMask, sizeof(nodeMask), MPOL_MF_MOVE );
			//if (bindRc == -1)
			//	exit(1);
			/*
			 * If MPOL_MF_STRICT is passed in flags and mode is not MPOL_DEFAULT,
			   then the call fails with the error EIO if the existing pages in the
			   memory range don't follow the policy.

			   If MPOL_MF_MOVE is specified in flags, then the kernel will attempt
			   to move all the existing pages in the memory range so that they
			   follow the policy.  Pages that are shared with other processes will
			   not be moved.  If MPOL_MF_STRICT is also specified, then the call
			   fails with the error EIO if some pages could not be moved.

			   If MPOL_MF_MOVE_ALL is passed in flags, then the kernel will attempt
			   to move all existing pages in the memory range regardless of whether
			   other processes use the pages.  The calling thread must be privileged
			   (CAP_SYS_NICE) to use this flag.  If MPOL_MF_STRICT is also
			   specified, then the call fails with the error EIO if some pages could
			   not be moved.
			 *
			 *
			 */
			assert(0 == bindRc);

			PreFaultStack();
		}
	}


	/*
	 * Getting the current CPU affinity
	 */
	if ((RL_mapping_ || PR_mapping_) && printout_actions_){
		cpu_set_t mask_tmp;
		int output_get_cpu_affinity = pthread_getaffinity_np(tinfo_[thread].thread_id, sizeof(mask_tmp), &mask_tmp);
		// int output_get_numa_affinity = numa_get_membind()
		std::vector<int> vec_cpus;
		std::string cpus;
		for (unsigned int cpu=0; cpu < max_num_cpus_; cpu++)
		{
			if (CPU_ISSET(cpu,&mask_tmp) != 0)
			{
				vec_cpus.push_back(cpu);
				cpus += std::to_string(cpu)+" ";
			}
		}
		std::cout << " thread " << tinfo_[thread].thread_num << " will run on CPU's { " << cpus << " } / thread (tid) = " << tinfo_[thread].tid << std::endl;

	}

}

void Scheduler::assign_memory_node_test(const unsigned int & thread_num, const unsigned int& old_numa_node, const unsigned int& new_numa_node)
{
	sleep(15);

	struct bitmask * numa_old_nodes;
	struct bitmask * numa_new_nodes;

	numa_old_nodes = numa_bitmask_alloc(max_num_numa_nodes_);
	numa_new_nodes = numa_bitmask_alloc(max_num_numa_nodes_);
	numa_bitmask_setbit( numa_old_nodes, old_numa_node);
	numa_bitmask_setbit( numa_new_nodes, new_numa_node);

	std::cout << " allocating memory ... \n";

	if (thd_suspend (tinfo_[thread_num].thread_id) != 0){
		printf ("%s:%d\t ERROR: Suspend thread failed!\n", __FILE__, __LINE__);
	}

	if (numa_migrate_pages(tinfo_[thread_num].tid, numa_old_nodes, numa_new_nodes) < 0)
	{
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	if (thd_continue (tinfo_[thread_num].thread_id) != 0){
		printf ("%s:%d\t ERROR: Continuing thread failed!\n", __FILE__, __LINE__);
	}

	std::cout << " after allocation of pages " << std::endl;
	sleep(15);

}

void Scheduler::assign_memory_node_test2(std::vector<unsigned int>& num_threads_per_resource)
{

	int pid = tinfo_[0].pid;
	std::cout << " ************** pid is " << tinfo_[0].pid << " and tid is " << tinfo_[0].tid << " and thread_id " << tinfo_[0].thread_num << std::endl;

	char *page_base;
	char *pages;

	void **addr;
	int *status;
	int *nodes;
	int errors;
	int nr_nodes;

	struct bitmask *old_nodes;
	struct bitmask *new_nodes;
	unsigned int pagesize = getpagesize();
	unsigned int page_count = 32;
	nr_nodes = numa_max_node()+1;
	old_nodes = numa_bitmask_alloc(nr_nodes);
	new_nodes = numa_bitmask_alloc(nr_nodes);
	numa_bitmask_setbit(old_nodes, 1);
	numa_bitmask_setbit(new_nodes, 0);

	int i, rc;

	if (nr_nodes < 2) {
		printf("A minimum of 2 nodes is required for this test.\n");
		exit(1);
	}

	page_base = (char*)malloc((pagesize + 1) * page_count);
	addr = (void**)malloc(sizeof(char *) * page_count);
	status = (int *)malloc(sizeof(int *) * page_count);
	nodes = (int *)malloc(sizeof(int *) * page_count);
	if (!page_base || !addr || !status || !nodes) {
		printf("Unable to allocate memory\n");
		exit(1);
	}

	pages = (char *) ((((long)page_base) & ~((long)(pagesize - 1))) + pagesize);

	for (i = 0; i < page_count; i++) {
		if (i != 2)
			/* We leave page 2 unallocated */
			pages[ i * pagesize ] = (char) i;
		addr[i] = pages + i * pagesize;
		nodes[i] = 1;
		status[i] = -123;
	}

	/* Move to starting node */
	std::cout << " Move to starting node \n";
	rc = numa_move_pages(pid, page_count, addr, nodes, status, 0);
	if (rc < 0 && errno != ENOENT) {
		perror("move_pages");
		exit(1);
	}

	sleep(30);

	/* Verify correct startup locations */
	printf("Page location at the beginning of the test\n");
	printf("------------------------------------------\n");

	numa_move_pages(pid, page_count, addr, NULL, status, 0);
	for (i = 0; i < page_count; i++) {
		printf("Page %d vaddr=%p node=%d\n", i, pages + i * pagesize, status[i]);
		if (i != 2 && status[i] != 1) {
			printf("Bad page state before migrate_pages. Page %d status %d\n",i, status[i]);
			exit(1);
		}
	}

	sleep(30);

	/* Move to node zero */
	printf("Move to node zero....\n");
	numa_move_pages(pid, page_count, addr, nodes, status, 0);

	printf("\nMigrating the current processes pages ...\n");
	rc = numa_migrate_pages(pid, old_nodes, new_nodes);

	if (rc < 0) {
		perror("numa_migrate_pages failed");
		errors++;
	}

	sleep(30);

	/* Get page state after migration */
	numa_move_pages(pid, page_count, addr, NULL, status, 0);
	for (i = 0; i < page_count; i++) {
		printf("Page %d vaddr=%lx node=%d\n", i,
			(unsigned long)(pages + i * pagesize), status[i]);
		if (i != 2) {
			if (pages[ i* pagesize ] != (char) i) {
				printf("*** Page contents corrupted.\n");
				errors++;
			} else if (status[i]) {
				printf("*** Page on the wrong node\n");
				errors++;
			}
		}
	}

	if (!errors)
		printf("Test successful.\n");
	else
		printf("%d errors.\n", errors);

}

void* Scheduler::PreFaultStack()
{
const size_t NUM_PAGES_TO_PRE_FAULT = 50;
const size_t size = NUM_PAGES_TO_PRE_FAULT * numa_pagesize();
void *allocaBase = alloca(size);
memset(allocaBase, 0, size);
return allocaBase;
}


void Scheduler::display_stack_related_attributes(pthread_attr_t *attr, char *prefix)
{
   int s;
   size_t stack_size, guard_size;
   void *stack_addr;

   s = pthread_attr_getguardsize(attr, &guard_size);
   if (s != 0)
	   handle_error_en(s, "pthread_attr_getguardsize");
   printf("%sGuard size          = %d bytes\n", prefix, guard_size);

   s = pthread_attr_getstack(attr, &stack_addr, &stack_size);
   if (s != 0)
	   handle_error_en(s, "pthread_attr_getstack");
   printf("%sStack address       = %p", prefix, stack_addr);
   if (stack_size > 0)
	   printf(" (EOS = %p)", (char *) stack_addr + stack_size);
   printf("\n");
   printf("%sStack size          = 0x%x (%d) bytes\n",
		   prefix, stack_size, stack_size);
}

/*
 * Write to files
 */
void Scheduler::write_to_files()
{
	if (write_to_files_)
	{

		avespeedfile_ << run_average_performance_ << "\n";
		avebalancedspeedfile_ << run_average_balanced_performance_ << "\n";
		timefile_ << time_ << "\n";
		for (unsigned int t=0;t<num_threads_;t++)
		{
			std::map<unsigned int, std::vector<Struct_Actions>>::iterator map_actions_it = map_Actions_per_Thread_.find(t);
			actionsfile_ << map_actions_it->second[0].action_per_child_source_ << ";";
			if (t==num_threads_-1){
				actionsfile_ << std::endl;
			}
		}
		if (write_to_files_details_){
			aveperformancefile1_ << vec_run_average_performances_[0] << "\n";
			aveperformancefile2_ << vec_run_average_performances_[1] << "\n";
			aveperformancefile3_ << vec_run_average_performances_[2] << "\n";
			aveperformancefile4_ << vec_run_average_performances_[3] << "\n";
//			strategiesfile1_ << map_strategies_[0][0] << ";" << map_strategies_[0][1] << ";" << map_strategies_[0][2] << ";" << map_strategies_[0][3] << "\n";
//			strategiesfile2_ << map_strategies_[1][0] << ";" << map_strategies_[1][1] << ";" << map_strategies_[1][2] << ";" << map_strategies_[1][3] << "\n";
//			strategiesfile3_ << map_strategies_[2][0] << ";" << map_strategies_[2][1] << ";" << map_strategies_[2][2] << ";" << map_strategies_[2][3] << "\n";
//			strategiesfile4_ << map_strategies_[3][0] << ";" << map_strategies_[3][1] << ";" << map_strategies_[3][2] << ";" << map_strategies_[3][3] << "\n";
		}
	}
}
