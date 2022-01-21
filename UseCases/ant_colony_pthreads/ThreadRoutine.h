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
 * ThreadRoutine.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Georgios Chasparis
 * Description: This library defines the 'work' function that executes the computational task of the threads.
 */

#ifndef THREADROUTINE_H_
#define THREADROUTINE_H_

#include <iostream>
#include <ctime>
#include <vector>
#include <numeric>
#include <chrono>

#include "ThreadInfo.h"

using namespace std;

class ThreadRoutine
{
public:

	ThreadRoutine()
	{
	};

	~ThreadRoutine()
	{
	};

	// virtual void * execute(void * arg);
	void work(thread_info* info);

private:

	void combinations_go_w_repetitions( unsigned int offset, unsigned int k, std::vector< std::vector<unsigned int> >& combs, std::vector<unsigned int>& tmp_combs, std::vector<unsigned int>& available_items );
	void combinations_w_repetitions( std::vector< std::vector<unsigned int> >& combs, unsigned int num_N, unsigned int num_k );

};


#endif /* THREADROUTINE_H_ */
