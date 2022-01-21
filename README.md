# PaRLSched Library

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General 
Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

For further information please contact: Software Competence Center Hagenberg GmbH (SCCH)
<georgios.chasparis@scch.at>, <office@scch.at>

## Main Contributors

* Georgios Chasparis, PhD, Key Researcher, Software Competence Center Hagenberg GmbH, Softwarepark 32a, Hagenberg 4232, Austria. email: `georgios.chasparis@scch.at`

* Vladimir Janjic, PhD, School of Science and Engineering, University of Dundee. email: `VJanjic001@dundee.ac.uk`

* Michael Rossbory, Senior Research Project Manager Data Science, Software Competence Center Hagenberg GmbH, Softwarepark 32a, Hagenberg 4232, Austria. email: `michael.rossbory@scch.at`


## Documentation & References

Doxygen documentation files are provided within the PaRLSched source files, which can be accessed through `./PaRLSched/html/index.html`. 
Instructions regarding the deployment and use of the software are provided below.

Furthermore, this work is based on several scientific papers that have led to the development of this software. 
We would really appreciate referencing the corresponding scientific papers in case this software is used for further scientific research.

* G. Chasparis, M. Rossbory, “Efficient Dynamic Pinning of Parallelized Applications by Distributed Reinforcement Learning,” International Journal of Parallel Programming, vol 47(1), pp. 24-38, 2019. 
* G. Chasparis, V. Janjic, M. Rossbory, and K. Hammond, “Learning-based Dynamic Pinning of Parallelized Applications in Many-Core Systems,” Euromicro PDP’19, Pavia, Italy, 2019.
* G. Chasparis, M. Rossbory, and V. Janjic, “Efficient Dynamic Pinning of Parallelized Applications by Reinforcement Learning with Applications,” LNCS 10417, Euro-Par 2017, pp. 1-13, 2017.


## General Description

The source code provided is divided into two sub-directories, namely:

* PaRLSched
* UseCases

The "PaRLSched" directory provides the source code for building the scheduler library, while the "UseCases" 
directory provides several use-cases of how the library can be linked and utilized for efficiently allocating 
a large number of threads. Then, a number of parallel (POSIX) threads following a standard farm pattern are created, while the 
computational task assigned to each thread corresponds to the computation of combinations of numbers. 
In fact, the pattern over which the threads are defined and the computational tasks executed is subject to 
the user and does not constrain the applicability of the scheduler. Furthermore, the details and nature of 
these computational tasks could be different for each thread and can be defined by the user within 
the "src/thread_routine.h" header file which is also provided.

The use-cases provided are described in the following subsections.

### Ant Colony Optimization (ACO)
It is a metaheuristic used for solving NP-hard combinatorial optimization problems. In particular, we apply ACO to the 
Single Machine Total Weighed Tardiness Problem (SMTWP). Briefly, this is a scheduling problem of jobs that are characterized 
by varying processing times, deadlines and weights. The objective is to find the schedule that minimizes the total tardiness. 
Further information can be found in `M. Dorigo and T. Stülze, "Ant Colony Optimization" Scituate, MA, USA: Bradford Company, 2004.`.  

### Blackscholes (BLA)

It calculates, using differential equations, how the value of an option changes as the price of the underlying asset changes; 
parallel implementation calculates values for the number of options at the same time, assigning a thread to each option 
(or a group of options). If the options are equally divided between threads, this results in a regular (in terms of task 
sizes) parallel application. In practice similar calculations are used by financial houses to price 10-100 thousands of options. 
This use-case has been extracted from the [Parsec Benchmark Suite](https://parsec.cs.princeton.edu/).

### Swaptions (SWA) 
It uses the Heath-Jarrow-Morton (HJM) framework to price a portfolio of swaptions. The HJM framework describes how 
interest rates evolve for risk management and asset liability management. For further information, please check the following reference *D. Heath, R. Jarrow, and A. Morton, "Bond pricing and the term structure of interest rates: A new 
methodologyfor contigent claims valuation," Econometrica, vol. 60, no. 1, pp. 77-105, Jan. 1992.* The application employs Monte-Carlo simulation to 
compute the prices. It is regular in terms of task sizes, with a low degree of communication between different threads. It has 
been extracted from the [Parsec Benchmark Suite](https://parsec.cs.princeton.edu/).


## Installation

Currently, the provided scheduler has been developed and tested over `Linux Kernel 64bit 3.13.0-43-generic`. 
For compiling the code, `gcc (version 4.9.3)` compiler has been used under the C++11 standard.

Before building the library, it is necessary that the PAPI-5.4.3 library interface for performance monitoring is 
being downloaded and installed. Download links and instructions can be found at [PAPI Software](http://icl.cs.utk.edu/papi/software). 
Furthermore, the pthread library should be installed, which is available in several Linux distributions. 

For example, for building the use-case located at `./UseCases` and linking to the PaRLSched library, 
CMakeLists files have been provided. To build and run the use-cases, simply run the following commands:

```
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DPAPI_LIBRARIES=/usr/local/papi-5.5.0/lib/libpapi.a -DPAPI_INCLUDE_DIRS=/usr/local/papi-5.5.0/include -S ../src/ .
make
```

In case it is derirable to edit the use-cases provided or create new use-cases, the following 
commands generate the corresponding C++ project in Eclipse IDE.

```
mkdir build
cd build
cmake -G"Eclipse CDT4 - Unix Makefiles" -D CMAKE_BUILD_TYPE=Release ..
make
```

For example, in Eclipse IDE (Version: Mars.1 Release (4.5.1)) the project can be imported as follows: 
"File -> Import -> C/C++" and then select "Existing Code as Makefile Project". Then, enter the root directory of the 
project and select the Linux GCC toolchain. 

## Scheduler Configuration

In the file `./scheduler_inputs/scheduler.config` you may find the list of configuration inputs necessary for running the scheduler together with a short description for each parameter.
Alternatively, the perl script `./runExps` automatically generates the configuration file and runs the selected use-case. It may also perform a comparison with the corresponding execution times of the OS 
scheduler providing the resulting statistics. We should mention that the current version of the scheduler has been tested only for maximum 2 NUMA nodes, however it would be straightforward to 
expand it to a higher number of NUMA nodes if needed.