/*
 * Scheduler.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Georgios C. Chasparis
 * Description: This is the PaRL-sched library that executes the core of the Scheduling policy for thread-pinning.
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <numa.h>
#include <papi.h>
#include <math.h>
#include <map>

#include <fstream>

#include "ThreadInfo.h"
#include "MethodsEstimate.h"
#include "MethodsPerformanceMonitoring.h"
#include "MethodsActions.h"
#include "MethodsOptimize.h"

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

/**
 * Scheduler
 */
class Scheduler
{
public:

	Scheduler();
	Scheduler(const std::string& in_file, const unsigned int& num_threads);
	Scheduler(const Scheduler& other);
	Scheduler& operator=(const Scheduler& other);
	~Scheduler();


	/**
	 * Initialize Scheduler
	 *
	 * @param num_threads					Number of threads
	 * @param RL_active_reshuffling			Reshuffle RL strategies if active threads have changed
	 * @param RL_performance_reshuffling	Reshuffle RL strategies if performance of active threads has dropped significantly
	 * @param step_size						Step size of RL algorithm
	 * @param LAMBDA						Perturbation factor of RL algorithm
	 * @param sched_period					Scheduling period of the child resources
	 * @param suspend_threads				Suspend threads when switching NUMA nodes
	 * @param printout_strategies
	 * @param printout_actions
	 * @param write_to_files
	 * @param write_to_files_details
	 * @param gamma							Factor related to the definition of the balanced performance indicator
	 * @param OS_mapping					Mapping according to the OS scheduler
	 * @param RL_mapping					Mapping according to the RL scheduler
	 * @param PR_mapping					Mapping according to a predefined map
	 * @param ST_mapping					Perform also allocation of thread memory
	 * @param optimize_main_resource		Optimize the use of NUMA nodes
	 */
	void initialize(
			  const unsigned int& num_threads
			, const bool& RL_active_reshuffling
			, const bool& RL_performance_reshuffling
			, const double& step_size
			, const double& LAMBDA
			, const double& sched_period
			, const bool& suspend_threads
			, const bool & printout_strategies
			, const bool & printout_actions
			, const bool& write_to_files
			, const bool& write_to_files_details
			, const double& gamma
			, const bool& OS_mapping
			, const bool& RL_mapping
			, const bool& PR_mapping
			, const bool& ST_mapping
			, const bool& optimize_main_resource
			);

	/**
	 * Update scheduling decisions
	 * @param resource_ind
	 * @param update_inds
	 */
	void update(const unsigned int& resource_ind, std::vector< bool >& update_inds);

	/**
	 * Run scheduler
	 */
	void run();

	/**
	 * Retrieve performances
	 *
	 * @param resource_ind
	 */
	void retrieve_performances(const unsigned int& resource_ind);

	/**
	 * Preprocessing of performances
	 * @param resource_ind
	 */
	void performance_preprocessing(const unsigned int& resource_ind);

	/**
	 * Applying scheduling policy
	 */
	void apply_scheduling_policy();

	/**
	 * Get information of thread
	 * @return
	 */
	inline thread_info* get_tinfo(void)
	{
		return tinfo_;
	}

	static void* send_wrapper(void* object)
    {
		reinterpret_cast<Scheduler*>(object)->run();
		return 0;
	}

	unsigned int counter_of_threads_;

	/**
	 * Get number of threads
	 * @return
	 */
	inline const unsigned int & get_number_threads(void) const
	{
		return num_threads_;
	}


private:

	bool RL_mapping_;
	bool OS_mapping_;
	bool PR_mapping_;
	bool ST_mapping_;

	unsigned int current_most_popular_node_;
	unsigned int previous_most_popular_node_;
	unsigned int current_popularity_;
	unsigned int previous_popularity_;
	bool reallocate_memory_;

	bool optimize_main_resource_;

	/**
	 * Assign processing NUMA node
	 *
	 * @param i
	 * @param new_numa_node
	 * @param previous_numa_node
	 * @param new_cpu_node
	 * @param previous_cpu_node
	 */
	void assign_processing_node(
			const unsigned int& i
			,const unsigned int& new_numa_node
			, const unsigned int& previous_numa_node
			, const std::vector< unsigned int >& new_cpu_node
			, const unsigned int& previous_cpu_node );

	/**
	 * Assign memory NUMA node (test function, it is not used)
	 *
	 * Here, we wish to migrate the pages of the process, to the NUMA node which is closer to the majority of the running threads.
	 * For example, the scheduler may keep track of the NUMA node that each thread is running and depending on whether the majority of
	 * them is running decides to move the pages of the process, or thread.
	 *
	 * @param thread_num
	 * @param old_numa_node
	 * @param new_numa_node
	 */
	void assign_memory_node_test(
			const unsigned int& thread_num
			, const unsigned int& old_numa_node
			, const unsigned int& new_numa_node
			);

	/**
	 * Assign memory NUMA node (test function, it is not used)
	 *
	 * @param num_threads_per_resource
	 */
	void assign_memory_node_test2(
			std::vector<unsigned int>& num_threads_per_resource
			);

	/**
	 * Perform estimation (or formulate beliefs) over potentially beneficial allocations
	 *
	 * @param resource_ind
	 */
	void estimate(const unsigned int& resource_ind);

	/**
	 * Optimize with respect to resources
	 *
	 * @param resource_ind
	 */
	void optimize(const unsigned int& resource_ind);

	/**
	 * Initialize Estimates
	 */
	void initialize_estimates();


	/**
	 * Initialize Performance Monitoring
	 *
	 * This function admits a similar structure to the initialize_estimates() function and initializes all the necessary
	 * variables for keeping track of the performance of the threads with respect to the resources to be optimized.
	 */
	void initialize_performancemonitoring();

	/**
	 *
	 * Initialize Actions
	 *
	 * This function admits a similar structure to the initialize_estimates() function and initializes all the necessary
	 * variables for keeping track of the actions of the threads with respect to the optimized resources
	 *
	 * @param policy
	 */
	void initialize_actions(const unsigned int& policy);

	/**
	 * Write actions selected to files
	 */
	void write_to_files();

	/*
	 * Variables related to the resources optimized
	 */
	std::vector< std::string > RESOURCES_;
	std::vector< std::string > CHILD_RESOURCES_;
	std::vector< std::string > RESOURCES_OPT_METHODS_;
	std::vector< std::string > CHILD_RESOURCES_OPT_METHODS_;
	std::vector< std::string > RESOURCES_EST_METHODS_;
	std::vector< std::string > CHILD_RESOURCES_EST_METHODS_;
	std::vector< unsigned int > MAX_NUMBER_MAIN_RESOURCES_;
	std::vector< std::vector<unsigned int>> MAX_NUMBER_CHILD_RESOURCES_;

	/*
	 * Variables related to the architecture
	 */
	bool support_numa_;
	unsigned int num_threads_;
	bool suspend_threads_;

	/*
	 * Variables related to the RL algorithm for CPU pinning
	 */
	double time_;
	double time_before_;
	unsigned int sched_iteration_ = 0;
	double step_size_RL_;
	double step_size_AL_;
	double LAMBDA_RL_;
	double LAMBDA_AL_;
	double gamma_;
	bool RL_active_reshuffling_;
	bool RL_performance_reshuffling_;
	unsigned int neighborhood_size_;
	double factor_h_;


	/*
	 * Neighborhoods
	 */
	std::map< unsigned int, std::set<unsigned int> > map_neighbors_;

	/*
	 * Scheduling period
	 */
	struct timespec ts_;
	struct timespec set_scheduling_period(const double& sched_period)
	{
		struct timespec ts;
		ts.tv_sec = floor(sched_period);
		if ( sched_period - (double) ts.tv_sec > 0)
			ts.tv_nsec = ( sched_period - (double)(ts.tv_sec) ) * 1e+9;
		else
			ts.tv_nsec = 0;
		return ts;
	};
	int numa_sched_period_;
	double numa_switch_threshold_;
	double zeta_;			// percentage of threads required before binding memory
	double utility_normalization_;

	/*
	 * Active Threads
	 */
	bool active_threads_;
	std::vector<bool> vec_active_threads_;

	/*
	 * Variables related to printout of strategies
	 */
	bool printout_strategies_;
	bool printout_actions_;
	bool write_to_files_;
	bool write_to_files_details_;

	/*
	 * (Performance) Estimates / Monitoring
	 * @description: We wish to create/generate estimates over the expected performance of certain resources
	 * (which may include child resources)
	 * At the same time, we wish to also observe the performance of the threads w.r.t. each one of the available resources.
	 * In the following maps, the first element refers to the 'thread_id'.
	 */
	std::map< unsigned int, std::vector< Struct_Estimate > > map_Estimate_per_Thread_;
	std::map< unsigned int, std::vector< Struct_PerformanceMonitoring> > map_PerformanceMonitoring_per_Thread_;
	std::map< unsigned int, std::vector< Struct_Actions > > map_Actions_per_Thread_;

	Struct_OverallPerformance overall_Performance_;
	Struct_MethodsOptimize methods_optimize_;
	Struct_MethodsEstimate methods_estimate_;

	/*
	 *	All information related to a thread
	 */
	thread_info * tinfo_;

	/*
	 * Variables related to the Performances of threads
	 */
	std::vector< double > vec_performances_;
	std::vector< double > vec_balanced_performances_;
	std::vector<bool> vec_performances_update_inds_;
	double cur_average_performance_;			// current average performance
	double cur_balanced_performance_;			// current balanced performance
	double run_average_performance_;			// running average performance
	double run_average_balanced_performance_;	// running average balanced performance
	std::vector< double > vec_run_average_performances_;
	std::vector< std::vector < double > > map_run_average_performances_cpu_;
	std::vector< std::vector < double > > map_run_average_balanced_performances_cpu_;
	std::vector< double > vec_min_performances_;
	std::vector< double > vec_alg_performances_;


	/*
	 * Variables related to the Active Threads
	 */
	unsigned int num_active_threads_;
	unsigned int num_active_threads_before_;

	/*
	 * Variables related to the Architecture
	 */
	unsigned int max_num_numa_nodes_;
	unsigned int max_num_cpus_;
	std::vector < std::vector< unsigned int > > cpu_nodes_per_numa_node_;

	void set_cpu_nodes_per_numa_node()
	{
		cpu_nodes_per_numa_node_.clear();
		for ( unsigned int nn=0; nn < max_num_numa_nodes_; nn++ )
		{
			std::vector < unsigned int > empty_vector;
			cpu_nodes_per_numa_node_.push_back(empty_vector);
		}
		for ( unsigned int cpu=0; cpu < max_num_cpus_; cpu++ )
		{
			cpu_nodes_per_numa_node_[numa_node_of_cpu(cpu)].push_back(cpu);
		}
	};


	/*
	 * Variables related to Streaming of Outputs/Policies
	 */
	std::fstream actionsfile_,
				avespeedfile_,
				avebalancedspeedfile_,
				timefile_,
				aveperformancefile1_,
				aveperformancefile2_,
				aveperformancefile3_,
				aveperformancefile4_,
				strategiesfile1_,
				strategiesfile2_,
				strategiesfile3_,
				strategiesfile4_;



	void display_stack_related_attributes(pthread_attr_t *attr, char *prefix);
	void* PreFaultStack();


	/*
	 * Temporary variables w.r.t. actions
	 *
	 */
	std::vector<unsigned int> action_main_old_;


};


#endif /* SCHEDULER_H_ */
