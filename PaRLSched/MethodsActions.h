/*
 * MethodsActions.h
 *
 *  Created on: Dec 07, 2016
 *      Author: Georgios C. Chasparis
 * Description: Provides methods and information structuring necessary for the actions of the threads (e.g., cpu, memory placement).
 */

#ifndef METHODSACTIONS_H_
#define METHODSACTIONS_H_

#include <vector>
#include <iostream>
#include <set>
#include <tuple>
#include <map>


/**
 * This structure gathers information that is relevant for keeping in track the actions selected by each
 * thread with respect to the resources that need to be optimized (e.g., cpu placement, memory placement)
 */
struct Struct_Actions
{

	/*
	 * We assume here that there are two kinds of resources (i.e., main resources and child resources)
	 * For example, a NUMA node may refer as the main resource, and the underlying CPU may refer to as a child resource.
	 * For now, it is assumed that there is a 'single' child resource for each main resource, however the current setup
	 * may as well be expanded to the case of multiple child resources.
	 */
	std::string resource_;													// This is the type of resource over which these measurements are recorded.
	std::string child_resource_;											// This is the type of child resource
	unsigned int num_actions_main_resource_;								// This is the number of actions per main resource
	std::vector<unsigned int> vec_num_child_actions_per_main_resource_;		// This is the number of actions w.r.t. the child resources per main resource.
	std::map<std::tuple<unsigned int,unsigned int,unsigned int>, std::vector<unsigned int>> map_neighbors_;  // <(thread_num, main_resource, child_resource), vec_neighbors>

	/*
	 * Running Average Performances Per Main & Child Source
	 *
	 * The idea here is that we would like to keep more specialized information about the average
	 * performances of each one of the Sources of either the main Resource or the child resources.
	 *
	 */
	unsigned int action_per_main_source_;				// e.g., this is per node running average performance
	unsigned int previous_action_per_main_source_;
	unsigned int action_per_child_source_;				// e.g., this is per cpu running average performance
	unsigned int previous_action_per_child_source_;
	std::vector < std::vector< unsigned int > > vec_child_sources_;			// e.g., this is the actual indicator of the child source

	/**
	 * We initialize the structure Struct_Actions
	 *
	 * @param resource				resource type
	 * @param num_sources			number of sources
	 * @param max_num_sources_user	maximum number of sources
	 * @param action_main_old		old action of the main resource
	 * @param thread_num			thread number
	 */
	void initialize
		(
				const std::string& resource
			  , const unsigned int& num_sources
			  , const unsigned int& max_num_sources_user
			  , std::vector<unsigned int>& action_main_old
			  , const unsigned int& thread_num
		)
	{
		// We initialize the selections with respect to the main resource (e.g., NUMA node) to be the first one
		resource_ = resource;
		num_actions_main_resource_ = std::min<unsigned int>(num_sources,max_num_sources_user);
		action_per_main_source_ = 0;
		previous_action_per_main_source_ = 0;
		action_main_old[thread_num] = action_per_main_source_;
	};


	/**
	 * Initialize the performance tracking variables when child resources are also present.
	 *
	 * @param resource							type of resource
	 * @param num_sources						number of sources
	 * @param max_num_sources_user				maximum number of sources
	 * @param child_resource					type of child resource
	 * @param vec_num_child_sources				vector of number of child sources per main source
	 * @param vec_max_num_child_sources_user	vector of maximum number of child sources per main source
	 * @param thread_num						thread number
	 * @param action_main_old					old action of the main resource
	 * @param policy							policy
	 * @param neighborhood_size					neighborhood size (for considering groups of child sources)
	 */
	void initialize_w_child
		(
		      const std::string& resource
			, const unsigned int& num_sources
			, const unsigned int& max_num_sources_user
			, const std::string& child_resource
			, const std::vector< std::vector < unsigned int> >& vec_num_child_sources
			, const std::vector< unsigned int >& vec_max_num_child_sources_user
			, const unsigned int& thread_num
			, std::vector<unsigned int>& action_main_old
			, const unsigned int & policy
			, const unsigned int & neighborhood_size
		)
	{
		/*
		 * We initialize the selections over the main & child sources to be the first choice available
		 */
		resource_ = resource;
		child_resource_ = child_resource;
		std::cout << " num of available main resources " << num_sources << " and number of resources set by the user " << max_num_sources_user << std::endl;
		num_actions_main_resource_ = std::min<unsigned int>(num_sources, max_num_sources_user);
		vec_num_child_actions_per_main_resource_.clear();
		vec_child_sources_.clear();
		std::cout << " num actions per main resource = " << num_actions_main_resource_ << std::endl;

		/*
		 * At this point, we have gathered the necessary information regarding the number of actions,
		 * the main resources and child resources.
		 */
		std::vector<unsigned int> initial_action;
		if (policy == 0)
			initial_action = compute_initial_action(vec_num_child_sources, vec_max_num_child_sources_user, thread_num, num_actions_main_resource_ );
		else if (policy == 1)
			initial_action = compute_initial_action_rr(vec_num_child_sources, vec_max_num_child_sources_user, thread_num );

		action_per_main_source_ = initial_action[0];
		action_main_old[thread_num] = action_per_main_source_;
		previous_action_per_main_source_ = initial_action[0];
		action_per_child_source_ = initial_action[1];
		previous_action_per_child_source_ = initial_action[1];

		/*
		 * Setting up the vector of available child resources
		 */
		for (unsigned int r = 0; r < num_actions_main_resource_; r++)
		{
			// for each one of the main sources, we assign the first option with respect to the child source
			unsigned int max_num_child_sources = std::min<unsigned int>(vec_num_child_sources[r].size(), vec_max_num_child_sources_user[r]);
			vec_num_child_actions_per_main_resource_.push_back(max_num_child_sources);
			std::cout << "      max number of child resources " << max_num_child_sources << std::endl;
			std::vector<unsigned int> tmp_vec_child_sources;
			for (unsigned int cr = 0; cr < max_num_child_sources; cr ++){
				tmp_vec_child_sources.push_back(vec_num_child_sources[r][cr]);
			}
			vec_child_sources_.push_back(tmp_vec_child_sources);

			/*
			 * Creating neighborhoods
			 * the vector vec_child_sources_ includes all child sources
			 * from this vector, we would like to create the map of neighbors
			 */
			std::vector<unsigned int> neighbors;
			for (unsigned int ns = 0; ns < neighborhood_size; ns++){
				neighbors.push_back(0);
			}
			unsigned int counter = 0;
			for (unsigned int cr = 0; cr < vec_child_sources_[r].size(); cr++){
				if (counter < neighborhood_size)
				{
					neighbors[counter] = vec_child_sources_[r][cr];
					counter++;
					if (counter == neighborhood_size || cr == vec_child_sources_[r].size()-1)
					{
						counter = 0;
						for (unsigned int n = 0; n < neighborhood_size; n++)
						{
							map_neighbors_.insert(std::pair< std::tuple< unsigned int, unsigned int, unsigned int >, std::vector< unsigned int > >(std::tuple< unsigned int, unsigned int, unsigned int>(thread_num,r,neighbors[n]), neighbors));
						}
					}
				}
			}
		}
	};

	/**
	 * Compute initial action
	 *
	 * @param vec_num_child_sources				vector of number of child sources
	 * @param vec_max_num_child_sources_user	vector of maximum number of child sources
	 * @param thread_num						thread number
	 * @param max_number_available_main_sources	maximum number of available main sources
	 * @return
	 */
	std::vector<unsigned int> compute_initial_action(
				  const std::vector< std::vector < unsigned int> >& vec_num_child_sources
				, const std::vector< unsigned int >& vec_max_num_child_sources_user
				, const unsigned int& thread_num
				, const unsigned int& max_number_available_main_sources)
	{
		// We first compute the correct placement of this thread into the set of CPU's available
		unsigned int overall_counter_cpu = 0;
		unsigned int counter_cpu = 0;
		unsigned int counter_numa = 0;
		bool found_cpu = false;

		std::cout << "    number of NUMA nodes used in initialization = " << vec_num_child_sources.size() << std::endl;

		while (!found_cpu)
		{
			counter_numa = 0;

			for (unsigned int r = 0; r < max_number_available_main_sources; r++)
			{
				// this is a loop over the main resource (e.g., NUMA nodes)
				counter_cpu = 0;
				unsigned int max_num_child_sources_available = std::min<unsigned int>(vec_num_child_sources[r].size(), vec_max_num_child_sources_user[r]);
				for (unsigned int j=0; j < max_num_child_sources_available; j++)
				{
					if ( thread_num == overall_counter_cpu)
					{
						// if the thread number coincides with the cpu number, then
						// we will assign this cpu to the thread
						found_cpu = true;
						break;
					}
					overall_counter_cpu ++;
					counter_cpu++;
				}

				if (found_cpu)
					break;
				else{
					counter_numa ++;
				}
			}
		}
		return {counter_numa, counter_cpu};
	}


	/**
	 * Compute initial action with Round Robin algorithm
	 *
	 * @param vec_num_child_sources				vector of number of child sources
	 * @param vec_max_num_child_sources_user	vector of maximum number of child sources per user
	 * @param thread_num						thread number
	 * @return
	 */
	std::vector<unsigned int> compute_initial_action_rr
		(
				  const std::vector< std::vector < unsigned int> >& vec_num_child_sources
				, const std::vector< unsigned int >& vec_max_num_child_sources_user
				, const unsigned int& thread_num
		)
	{
		// We first compute the correct placement of this thread into the set of CPU's available
		unsigned int numa_nodes = vec_num_child_sources.size();
		std::cout << "    number of NUMA nodes used in initialization = " << numa_nodes << std::endl;
		std::vector<unsigned int> basis;
		for (unsigned int r=0;r<numa_nodes;r++)
			basis.push_back(r+1);

		unsigned int minimum_outcome(1e+10);
		unsigned int arg_minimum(0);
		for (unsigned int r=0;r<numa_nodes;r++)
		{
			// We compute the division result:
			double outcome = (thread_num+1) / basis[r];
			unsigned int remainder = (thread_num+1) % basis[r];
			if (remainder == 0 && outcome < minimum_outcome)
			{
				minimum_outcome = outcome;
				arg_minimum = r;
			}
		}

		// After selecting the best numa node, we select the best cpu node
		unsigned int max_num_child_sources_available = std::min<unsigned int>(vec_num_child_sources[arg_minimum].size(), vec_max_num_child_sources_user[arg_minimum]);
		unsigned int minimum_outcome2(1e+10);
		unsigned int arg_minimum2(0);
		unsigned int counter_cpu(0);
		for (unsigned int j=0; j < max_num_child_sources_available; j++)
		{
			counter_cpu++;
			double outcome2 = minimum_outcome / counter_cpu;
			unsigned int remainder2 = minimum_outcome % counter_cpu;
			if (remainder2 == 0 && outcome2 < minimum_outcome2)
			{
				minimum_outcome2 = outcome2;
				arg_minimum2 = vec_num_child_sources[arg_minimum][j];
			}
		}
		std::cout << " allocation of thread " << thread_num << " is ( " << arg_minimum << " , " << arg_minimum2 << " ) " << std::endl;
		return {arg_minimum, arg_minimum2};
	}

};

#endif /* METHODSACTIONS_H_ */
