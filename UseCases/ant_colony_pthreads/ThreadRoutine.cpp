/* -----------------------------------------------------------------------------
 * Copyright (C) 2012-2014 Software Competence Center Hagenberg GmbH (SCCH)
 * <georgios.chasparis@scch.at>, <office@scch.at>
 * -----------------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
 * This code is subject to dual-licensing. Please contact office@scch.at
 * if you are interested in obtaining a differently licensed version.
*/

/*
 * ThreadRoutine.cpp
 *
 *  Created on: May 31, 2016
 *      Author: Georgios Chasparis
 */


#include "ThreadRoutine.h"
#include "ThreadControl.h"
#include <assert.h>


/*
 * The function 'work' defines the computational tasks executed by each thread.
 * In this example, it is defined to be the same function for each thread, however
 * we may define different function running for each thread (conditioned on info->thread_id)
 */
void ThreadRoutine::work(thread_info* info)
{
	std::vector< std::vector<unsigned int> > combs;

	/*
	 * Computational tasks.
	 * Can be defined differently for each thread if we condition the computations to each thread_id.
	 * Here the computations are the same for each thread and correspond to computing combinations
	 */
	for (unsigned int i=1; i < info->thread_goal; i++)
	{
		combs.clear();
		combinations_w_repetitions(combs,40,8);
	}
}


void ThreadRoutine::combinations_go_w_repetitions( unsigned int offset, unsigned int k, std::vector< std::vector<unsigned int> >& combs, std::vector<unsigned int>& tmp_combs, std::vector<unsigned int>& available_items )
{
	if( k == 0 ) {
		//combs.push_back( tmp_combs );
        return;
    }

    for(unsigned int i = offset; i <= available_items.size() - k; ++i ) {
    	assert( available_items[i] == i );
        tmp_combs.push_back( available_items[i] );
        combinations_go_w_repetitions( i , k - 1, combs, tmp_combs, available_items );     // if you want to include repetitions, set 'i' instead of 'i+1'
        tmp_combs.pop_back();
    }
}

void ThreadRoutine::combinations_w_repetitions( std::vector< std::vector<unsigned int> >& combs, unsigned int num_N, unsigned int num_k )
{
       /*
        * this function simply computes the combinations of max_num_bands out of num_bands
        */

       combs.clear();
       std::vector<unsigned int> tmp_combs;
       tmp_combs.clear();

       std::vector<unsigned int> available_items;
       available_items.clear();

       for( unsigned int i = 0; i < num_N; i++ ) {
    	   available_items.push_back( i );
       }

       combinations_go_w_repetitions( 0, num_k, combs, tmp_combs, available_items );     // we initiate computation of combinations

}
