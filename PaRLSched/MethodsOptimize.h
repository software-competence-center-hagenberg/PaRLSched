/*
 * MethodsOptimize.h
 *
 *  Created on: Dec 12, 2016
 *      Author: Georgios C. Chasparis
 * Description: Provides methods and information structuring necessary for optimizing the
 * 				resources for each one of the threads.
 */

#ifndef METHODSOPTIMIZE_H_
#define METHODSOPTIMIZE_H_

#include <vector>
#include <iostream>
#include <set>



/**
 * It provides the necessary functions/algorithms for optimizing the selection of resources from the threads.
 * It may or may not make use of the estimates over the most rewarding resources, as created by the MethodsEstimate.h library.
 *
 */
struct Struct_MethodsOptimize
{

	/**
	 * Provides a framework for computing the optimal allocation of resources based on the estimates created by an RL estimator.
	 *
	 * @param vec_cummulative_estimates			vector of cummulative estimates
	 * @param num_choices						number of choices
	 * @param action							action
	 * @param LAMBDA							perturbation factor
	 * @param current_run_ave_performance		current running average performance
	 */
	void RL_optimize(
			const std::vector< double >& vec_cummulative_estimates,
			const unsigned int& num_choices,
			unsigned int& action,
			const double& LAMBDA,
			const double& current_run_ave_performance)
	{
		/*
		 * Updating Main Resource
		 */
		double rnd = rand() % 100;
		if (rnd <= LAMBDA * 100)
		{
			// this is the case where we perturb the action (with a probability that it is rather small)
			// we then select one of the available choices by using a uniform distribution
			action = random_selection_uniform(num_choices);
		}
		else
			action = random_selection_strategy(num_choices, vec_cummulative_estimates);

	};

	/**
	 * Random selection of resources based on a uniform distribution
	 *
	 * @param num_choices		available number of choices
	 * @return
	 */
	unsigned int random_selection_uniform(const unsigned int& num_choices)
	{
		unsigned int action(0);
		double rnd = rand() % 100;
		for (unsigned int a = 0; a < num_choices; a++)
		{
			// for each one of the main sources, we perform the following steps

			if ( (a==0) && (rnd >=0) && ( rnd < (double)(a+1) * (1/(double)num_choices) * 100 )  )
			{
				action = a;
				break;
			}
			else if ( (a>0) && (rnd >= (double)a * (1/(double)num_choices) * 100) && (rnd < (double)(a+1) * (1/(double)num_choices) * 100))
			{
				action = a;
				break;
			}
		}

		return action;
	};


	/**
	 * Random Selection Strategy assigns a random selection based on the provided estimates
	 * @param num_choices
	 * @param estimates
	 * @return
	 */
	unsigned int random_selection_strategy(const unsigned int& num_choices, const std::vector<double>& estimates)
	{

		// here, we follow a policy based on the current strategy
		unsigned int action(0);
		double rnd = rand() % 100;
		for (unsigned int a = 0; a < num_choices; a++)
		{
			if ((a==0) && (rnd >=0) && (rnd < estimates[a] * 100)){
				action = a;
				break;
			}
			else if ((a > 0)&&(rnd >= estimates[a-1] * 100) && (rnd < estimates[a] * 100)){
				action = a;
				break;
			}
		}

		return action;
	}


	/*
	 * AL_optimize
	 *
	 * @description:
	 *
	 * @inputs: The function receives as inputs
	 * 			a) prior estimates
	 * 			b) prior actions
	 * 			and computes the next allocation that needs to be implemented by all threads.
	 */

	/**
	 * It provides a framework for computing the optimal allocation of resources based on Aspiration Learning
	 *
	 * @param random_switch								Indicator that a Random Switch should be executed
	 * @param action_change								Indicator that the action should be changed
	 * @param run_average_balanced_performance			Running average balanced performance
	 * @param run_ave_performance_per_main_resource		Running average performance per main resource
	 * @param low_benchmark								Low benchmark of aspiration learning
	 * @param high_benchmark							High benchmark of aspiration learning
	 * @param action									Action
	 * @param num_actions								Number of actions
	 * @param LAMBDA									Perturbation factor
	 * @param thread									Thread indicator
	 * @param numa_switch_threshold						Threshold over which the NUMA (main resource) is updated
	 */
	void AL_optimize
	(
		const bool & random_switch
		, const bool & action_change
		, const double& run_average_balanced_performance
		, const std::vector<double>& run_ave_performance_per_main_resource
		, const double & low_benchmark
		, const double& high_benchmark
		, unsigned int & action
		, unsigned int & num_actions
		, const double& LAMBDA
		, const unsigned int& thread
		, const double& numa_switch_threshold
	)
	{
		/*
		 * The reasoning behind the selection of an action based on Aspiration Learning is based upon the following:
		 *  - We retrieve the running average performance of this thread (this is a running average performance over the history)
		 *  -
		 */

		std::cout << " AL optimize main resource for thread " << thread << std::endl;
		unsigned int NUMA_nodes = run_ave_performance_per_main_resource.size();
		unsigned int action_previous = action;
		unsigned int action_new;

		// the action changes only if the running average performance drops below the threshold

		if (random_switch || action_change)
		{

			// in this case, we need to randomize according to LAMBDA
			double rnd = rand() % 100;
			if ( (rnd <= LAMBDA * 100) && LAMBDA > 0){
				action_new = random_selection_uniform(num_actions);
				std::cout << " total number of actions " << num_actions << std::endl;
				std::cout << " action " << action << std::endl;
				std::cout << "action new " << action_new << std::endl;
				if (run_ave_performance_per_main_resource[action_new] > 1e-3){

					if (run_ave_performance_per_main_resource[action_previous] < numa_switch_threshold * run_ave_performance_per_main_resource[action_new] ) // && rand_tmp < 0.2
						action = action_new;
				}
				else
					action = action_new;
			}
		}

	}

};


#endif /* METHODSOPTIMIZE_H_ */
