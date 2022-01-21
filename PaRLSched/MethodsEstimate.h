/*
 * Scheduler.h
 *
 *  Created on: Nov 29, 2016
 *      Author: Georgios C. Chasparis
 * Description: Provides methods and information structuring necessary for creating
 * 				estimates over performances of allocations.
 */

#ifndef METHODSESTIMATE_H_
#define METHODSESTIMATE_H_

#include <vector>
#include <iostream>
#include <set>
#include <algorithm>
#include "MethodsActions.h"
#include <unistd.h>


/*
 * Struct_Estimate
 *
 * @description: provides the necessary information for keeping track our estimates for the performance of a thread over a specific node/cpu
 * or the performance of a thread's memory over a specific node (or some further refinement if possible)
 *
 * This structure is planned for 'per-thread' use, since it will hold estimates over the proper use of resources by each thread separately.
 *
 * Within this structure, there is an additional struct that tracks the performances of the threads throughout the running time.
 *
 */

/**
 * Struct_Estimate
 *
 * It provides the necessary information for keeping track our estimates for the performance of a thread over a specific node/cpu
 * or the performance of a thread's memory over a specific node (or some further refinement if possible)
 *
 * This structure is planned for 'per-thread' use, since it will hold estimates over the proper use of resources by each thread separately.
 *
 * Within this structure, there is an additional struct that tracks the performances of the threads throughout the running time.
 */
struct Struct_Estimate
{

	std::string resource_;									/* The name of the resource assigned. */
	unsigned int num_sources_;								/* This is the number of the sources available, e.g., number of CPU's available. */
	std::vector< std::vector< unsigned int > > vec_child_sources_;		/* This is the number of the child sources available for each one of the main sources.
																Potentially, there might be different number of child sources for each main source. */
	std::vector<double> vec_estimates_;						/* This vector holds our estimates over the best assignment, e.g., over the CPU assignment */
	std::vector<double> vec_cummulative_estimates_;

	double run_average_performance_;						/* It holds our running average performance */

	std::vector< Struct_Estimate > vec_child_estimates_;	/* child estimates (used under hierarchical resources) */

	double low_benchmark_;
	double high_benchmark_;

	double maximum_performance_;
	bool random_switch_;									/* This variable enforces random switch of an action */
	bool action_change_;


	/**
	 * This function initializes the structure
	 *
	 * @param resource					resource type
	 * @param num_sources				number of sources
	 * @param max_num_sources_user		maximum number of sources per user
	 * @param initial_action			initial action
	 * @param mixed
	 */
	void initialize(
			const std::string& resource
			, const unsigned int& num_sources
			, const unsigned int& max_num_sources_user
			, const unsigned int& initial_action
			, const bool& mixed
			)
	{
		random_switch_ = false;
		action_change_ = false;
		maximum_performance_ = 0;
		resource_ = resource;
		num_sources_ = std::min<unsigned int>(num_sources,max_num_sources_user);
		vec_estimates_.resize(num_sources_);
		vec_cummulative_estimates_.resize(num_sources_);
		if (num_sources > 0)
		{
			for (unsigned int e=0;e<num_sources_;e++)
			{
				if (!mixed){
					if (e == initial_action)
						vec_estimates_[e] = 0.98;
					else{
						if (num_sources_ - 1 > 0)
							vec_estimates_[e] = 0.02 / ((double)num_sources_ - 1);
						else
							vec_estimates_[e] = 0.02;
					}
				}
				else
					vec_estimates_.at(e) = (double)1/((double)num_sources);

				if (e==0)
					vec_cummulative_estimates_[e] = vec_estimates_[e];
				else
					vec_cummulative_estimates_[e] = vec_cummulative_estimates_[e-1] + vec_estimates_[e];
			}
		}
	}


	/**
	 * It is implicitly assumed that the main resource also has a child resource
	 * For example, in the case of PROCESSING_BANDWIDTH, the main sources correspond to the NUMA nodes (if available)
	 * while the child sources correspond to the CPU units available in each NUMA node.
	 *
	 * @param resource							resource type
	 * @param num_sources						number of sources
	 * @param max_num_sources_user				maximum number of sources per user
	 * @param child_resource					child resource type
	 * @param vec_num_child_sources				vector of number of child sources
	 * @param vec_max_num_child_sources_user	vector of maximum number of child sources per user
	 * @param thread_num						thread number
	 */
	void initialize_w_child
		(
			  const std::string& resource
			, const unsigned int& num_sources
			, const unsigned int& max_num_sources_user
			, const std::string& child_resource
			, const std::vector< std::vector< unsigned int > >& vec_num_child_sources
			, const std::vector< unsigned int >& vec_max_num_child_sources_user
			, const unsigned int & thread_num
		)
	{
		resource_ = resource;
		num_sources_ = std::min<unsigned int>(num_sources,max_num_sources_user);
		vec_estimates_.resize(num_sources_);
		vec_cummulative_estimates_.resize(num_sources_);
		vec_child_sources_.clear();
		for (unsigned int r=0;r < num_sources_; r++){
			std::vector<unsigned int> temp_child_resources;
			unsigned int num_child_res = std::min<unsigned int>(vec_num_child_sources[r].size(),vec_max_num_child_sources_user[r]);
			for (unsigned int cr=0; cr < num_child_res; cr++)
				temp_child_resources.push_back(vec_num_child_sources[r][cr]);
			vec_child_sources_.push_back(temp_child_resources);
		}

		/* Computing the proper initial action based on Round-Robin */
		Struct_Actions actions;
		std::vector<unsigned int> initial_action = actions.compute_initial_action(vec_num_child_sources,vec_max_num_child_sources_user,thread_num, num_sources_);
		unsigned int initial_action_main = initial_action[0];
		unsigned int initial_action_child = initial_action[1];

		/* initialize the vector of estimates for the main resource */
		if (num_sources > 0){
			for (unsigned int e=0;e<num_sources_;e++)
			{
				if (num_sources_ - 1 > 0){
					if (e == initial_action_main)
						vec_estimates_[e] = 0.9999;
					else{
						vec_estimates_[e] = 0.0001 / ((double)num_sources_ - 1);
					}
				}
				else
					vec_estimates_[e] = 1;

				if (e==0)
					vec_cummulative_estimates_[e] = vec_estimates_[e];
				else
					vec_cummulative_estimates_[e] = vec_cummulative_estimates_[e-1] + vec_estimates_[e];
			}
		}
		/* initialize the vector of estimates for the child resources */
		vec_child_estimates_.clear();
		if (vec_child_sources_.size() > 0){
			// in case the number of the child resources is nonzero
			for (unsigned int cr = 0; cr < vec_child_sources_.size(); cr++)
			{
				Struct_Estimate s_estimate;
				if (cr == initial_action_main)
					s_estimate.initialize(child_resource, vec_child_sources_[cr].size(), vec_max_num_child_sources_user[cr], initial_action_child, false);
				else
					// in case this is not the main source selected, then we mix the estimates over the child resources.
					s_estimate.initialize(child_resource, vec_child_sources_[cr].size(), vec_max_num_child_sources_user[cr], initial_action_child, true);
				vec_child_estimates_.push_back(s_estimate);
			}
		}
	}


	/**
	 * This function receives a vector of estimates, and assigns it to the existing structure
	 *
	 * @param vec_estimates
	 */
	void update_estimates(const std::vector<double>& vec_estimates)
	{
		for (unsigned int b=0;b<vec_estimates.size();b++)
			vec_estimates_.at(b) = vec_estimates.at(b);
	}

	/**
	 * This function receives a vector of estimates for the child resources, and assigns it to the existing structure
	 *
	 * @param vec_estimates
	 * @param source_num
	 */
	void update_child_estimate(const std::vector<double>& vec_estimates, const unsigned int& source_num)
	{
		for (unsigned int b=0;b<vec_estimates.size();b++)
			vec_child_estimates_.at(source_num).vec_estimates_.at(b) = vec_estimates[b];
	}


	/**
	 *
	 * @param vec_estimates
	 * @param source_num
	 */
	void update_action(const std::vector<double>& vec_estimates, const unsigned int & source_num){};

};



/**
 * This structure provides alternative methodologies for creating estimates over the best selection.
 * Inputs: For now, we assume that it takes as inputs the following:
 * 			- prior estimate data (maybe not created under the same rule)
 * 			- current performance (this may take different values depending on the resource to be allocated)
 */
struct Struct_MethodsEstimate
{


	/**
	 * Provides a framework for creating estimates over the best action, by using the current selection and the current performance
	 * It is assumed that a set of 'finite' number of actions/selections is available (e.g., number of available nodes).
	 *
	 * @param maximum_performance				maximum performance observed so far
	 * @param vec_estimates						vector of estimates
	 * @param vec_cummulative_estimates			vector of cummulative estimates
	 * @param current_performance				current performance
	 * @param current_run_ave_performance		current running average performance
	 * @param current_action					current action
	 * @param action_main_changed				indicator that the action of the main resource has changed
	 * @param step_size_RL						step size of RL algorithm
	 * @param RL_active_reshuffling				strategies are reshuffled when a thread becomes inactive
	 * @param RL_performance_reshuffling		strategies are reshuffled when the performance drops significantly
	 * @param factor_h							factor h of aspiration-based RL algorithm
	 * @param active_threads_change				indicators that the active threads have changed
	 * @param thread							thread number
	 */
	void RL_update(double& maximum_performance,
			std::vector<double>& vec_estimates,
			std::vector<double>& vec_cummulative_estimates,
			const double& current_performance,
			const double& current_run_ave_performance,
			const unsigned int& current_action,
			const bool& action_main_changed,
			const double& step_size_RL,
			const bool& RL_active_reshuffling,
			const bool& RL_performance_reshuffling,
			const double& factor_h,
			const bool& active_threads_change,
			const unsigned int & thread)
	{

		if (RL_performance_reshuffling)
		{

			// for each thread, we compare its balanced performance compared to the previous one.
			// if the performance of a thread has been dropped significantly, then we initiate reshuffling
			// if ((balanced_performances_[t] < 0.7 * balanced_performances_before_[t]) && (!active_threads_change_) && (!change_in_action)){
			// the minimum strategy of a thread is an indication of its
			double max_estimate = *std::max_element(vec_estimates.begin(),vec_estimates.end());

			// originally this threshold was set to 0.7 / 0.95
			if ((current_performance < 0.6 * current_run_ave_performance) && (!active_threads_change) && (max_estimate > 0.99))
			{
				std::cout << " Reshuffling due to significant performance degradation \n";
				std::cout << " the maximum element of the strategy of the thread is " << *std::max_element(vec_estimates.begin(),vec_estimates.end()) << std::endl;
				std::cout << " reshuffling thread " << thread << " run-average performance " << current_run_ave_performance << " and current bal. performance " << current_performance << std::endl;
				RL_reshuffle_mixed(vec_estimates);
			}
		}

		if (RL_active_reshuffling)
		{
			if (active_threads_change)
				RL_reshuffle_mixed(vec_estimates);
		}

		/// We define a new step-size just for updating the RL
		double step_size_tmp(step_size_RL);
		if (current_run_ave_performance > 0)
			step_size_tmp = step_size_RL / current_run_ave_performance;    // originally 0.3
		else
			step_size_tmp = step_size_RL;

		/*
		 * Reshuffling of Strategies, due to changes in the Main resource
		 */
		if (action_main_changed)
		{
			std::cout <<  "thread : " << thread << " main resource changed ... reshuffling estimates of child ... " << std::endl;
			RL_reshuffle_mixed(vec_estimates);
		}

		for (unsigned int source=0; source < vec_estimates.size(); source++)
		{
			/* for each one of the sources, we need to update the corresponding vector of estimates */
			if (current_performance >= current_run_ave_performance * 0.98){
				if (source == current_action)
				{
					/* in case, we update the source corresponding to the current_action */
					vec_estimates[source] = vec_estimates[source] + step_size_tmp * (double)current_performance * ( 1 - vec_estimates[source] );
				}
				else
				{
					vec_estimates[source] = vec_estimates[source] - step_size_tmp * (double)current_performance * vec_estimates[source];
				}
			}
			else
			{

				double phi = std::max<double>(factor_h, 1 + (current_performance - current_run_ave_performance)/factor_h);

				//if (current_performance < current_run_ave_performance)
				if (source == current_action)
				{
					/* in case, we update the source corresponding to the current_action */
					vec_estimates[source] = vec_estimates[source] + step_size_tmp * (double)current_performance * ( 1 - vec_estimates[source] ) * phi;
				}
				else
				{
					vec_estimates[source] = vec_estimates[source] - step_size_tmp * (double)current_performance * vec_estimates[source] * phi;
				}
			}
		}

		// computing the new cumulative estimates
		for (unsigned int source=0; source < vec_estimates.size(); source++)
		{
			vec_cummulative_estimates[source] = (double)1/((double)vec_estimates.size());
			if (source==0)
				vec_cummulative_estimates[source] = vec_estimates[source];
			else
				vec_cummulative_estimates[source] = vec_cummulative_estimates[source-1] + vec_estimates[source];
		}
	}


	/*
	 * The purpose of the following function is to shuffle the strategies for the threads, when some other threads became idle (or non-active)
	 */
	void RL_reshuffle_mixed(std::vector<double>& vec_estimates)
	{

		int cpu_ind;
		int thread_counter(0);
		// if one or more threads have become idle or non-active, then we re-initialize the strategies of the remaining threads
		std::cout << " !!!!!!!!!!!!!!!!!!!! RESHUFFLE MIXED !!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";

		unsigned int num_sources = vec_estimates.size();
		std::cout << " size of vector of estimates" << num_sources << std::endl;

		for (unsigned int j=0; j < num_sources; j++)
		{
			vec_estimates[j] = 1 / (double)(num_sources);
		}
	}


	/**
	 * AL: Aspiration-Learning Estimates Update based on Running Average Performance
	 *
	 * @param current_performance				current performance
	 * @param current_run_ave_performance		current running average performance
	 * @param run_ave_performance_before		previous running average performance
	 * @param step_size							step size
	 * @param active_threads_change				indicator that the active threads have changed
	 * @param thread							thread indicator
	 * @param low_benchmark						low performance benchmark
	 * @param high_benchmark					high performance benchmark
	 * @param random_switch						indicator random switch of sources
	 * @param action_change						indicator that action has changed
	 */
	void AL_update
			(
				const double& current_performance
				, const double& current_run_ave_performance
				, const double& run_ave_performance_before
				, const double& step_size
				, const bool& active_threads_change
				, const unsigned int & thread
				, double & low_benchmark
				, double & high_benchmark
				, bool & random_switch
				, bool & action_change
			)
	{
		// the idea of updating the estimates here is as follows
		// we update the running average performance of the selected action (the remaining ones stay the same)
		// using the updated running average performances, we change the estimates
		std::cout << " ~~~~~~~~~~~~~~~~~~~~\n";
		std::cout << " updating main resource of thread " << thread << std::endl;

		random_switch = false;
		action_change = false;

		// Updating benchmark payoffs/performances
		if (current_run_ave_performance >= high_benchmark)
		{
			// high_benchmark = high_benchmark + 0.2 * ( current_run_ave_performance - high_benchmark);
			high_benchmark = current_run_ave_performance;		// originally 1.1
			low_benchmark = 0.8 * high_benchmark;
			random_switch = true;
			action_change = false;			// we allow the possibility to change actions, based on some randomization
		}
		else if (current_run_ave_performance < high_benchmark && current_run_ave_performance >= low_benchmark)
		{
			random_switch = true;
			action_change = false;
		}
		else if (current_run_ave_performance < low_benchmark)
		{
			random_switch = false;
			action_change = true;
			///< Updating benchmarks
			low_benchmark = 0.95 * current_run_ave_performance;	// originally 0.9
			high_benchmark = 1.2 * low_benchmark;
		}

		std::cout << " random switch " << random_switch << std::endl;
		std::cout << " action change " << action_change << std::endl;
	}
};


#endif /* METHODSESTIMATE_H_ */
