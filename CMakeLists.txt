# CMakeLists files 
cmake_minimum_required (VERSION 2.8.12)
project (PaRL-sched)
# SET(CMAKE_BUILD_TYPE "Release")

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")
endif()


include(CheckStructHasMember)
include(CheckLibraryExists)

#find_package(PAPI REQUIRED)
#if (PAPI_FOUND)
#  include_directories(${PAPI_INCLUDE_DIRS})
#  target_link_libraries (PaRL-sched ${PAPI_LIBRARIES})
##  message(STATUS "PAPI Lib Dir: ${PAPI_LIBRARIES}")
##  message(STATUS "PAPI Inc Dir: ${PAPI_INCLUDE}")
#endif (PAPI_FOUND)

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(PUBLIC PaRL-sched "-pthread")
endif()

# Finding PAPI library
FIND_LIBRARY(PAPI_LIBRARIES NAMES libpapi.a papi)
FIND_PATH(PAPI_INCLUDE_DIRS NAMES papi.h)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PAPI DEFAULT_MSG PAPI_LIBRARIES PAPI_INCLUDE_DIRS)
IF(PAPI_FOUND)
  ADD_DEFINITIONS(-DUSE_PAPI)
ENDIF(PAPI_FOUND)

# Finding NUMA library
find_path(NUMA_ROOT_DIR
  NAMES include/numa.h
  PATHS ENV NUMA_ROOT
  DOC "NUMA root directory")

find_path(NUMA_INCLUDE_DIRS
  NAMES numa.h
  HINTS ${NUMA_ROOT_DIR}
  PATH_SUFFIXES include
  DOC "NUMA include directory")

find_library(NUMA_LIBRARY
  NAMES numa
  HINTS ${NUMA_ROOT_DIR}
  DOC "NUMA library")

if (NUMA_LIBRARY)
    get_filename_component(NUMA_LIBRARY_DIR ${NUMA_LIBRARY} PATH)
endif()

mark_as_advanced(NUMA_INCLUDE_DIR NUMA_LIBRARY_DIR NUMA_LIBRARY)

find_package_handle_standard_args(NUMA REQUIRED_VARS NUMA_ROOT_DIR NUMA_INCLUDE_DIRS NUMA_LIBRARY)

FIND_LIBRARY(NUMA_LIBRARIES NAMES libnuma numa)
FIND_PATH(NUMA_INCLUDE_DIRS NAMES numa.h)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NUMA DEFAULT_MSG NUMA_LIBRARIES NUMA_INCLUDE_DIRS)
IF(NUMA_FOUND)
  ADD_DEFINITIONS(-DUSE_NUMA)
ENDIF(NUMA_FOUND)

#-------------------------------------------------------------------------------------------
#Finding Hwloc Library
find_path(Hwloc_INCLUDE_DIRS
  NAMES
    hwloc.h
  PATHS
  ENV HWLOC_ROOT
)

find_library(Hwloc_LIBRARIES
  NAMES
    hwloc
  PATHS
  ENV HWLOC_ROOT
)

if(Hwloc_INCLUDE_DIRS AND Hwloc_LIBRARIES)
  message(WARNING "HWLOC library found using find_library() - cannot determine version. Assuming 1.7.0")
  set(Hwloc_FOUND 1)
  set(Hwloc_VERSION "1.7.0")
endif()

#------------------------------------------------------------------------------------------
#INCLUDE DIRECTORIES

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})
include_directories(${Hwloc_INCLUDE_DIRS})

set(CMAKE_INSTALL_PATH "${PROJECT_SOURCE_DIR}")
# Recurse into the "src" subdirectory. 

include_directories( 
    ${PROJECT_SOURCE_DIR}/PaRLSched
   #${PROJECT_SOURCE_DIR}/UseCases/combs_calculation
    ${PROJECT_SOURCE_DIR}/UseCases/ant_colony_pthreads
   #${PROJECT_SOURCE_DIR}/UseCases/matmult_pthreads
   #${PROJECT_SOURCE_DIR}/UseCases/fluidanimate/src
   #${PROJECT_SOURCE_DIR}/UseCases/evopro
    ${PROJECT_SOURCE_DIR}/UseCases/blackscholes/src
   #${PROJECT_SOURCE_DIR}/UseCases/synthetic
    ${PROJECT_SOURCE_DIR}/UseCases/swaptions/src
   #${PROJECT_SOURCE_DIR}/examples/raytrace/src
   #${PROJECT_SOURCE_DIR}/examples/CSO_benchmark_omp
   #${PROJECT_SOURCE_DIR}/UseCases/CSO_benchmark_ff
)

add_subdirectory (PaRLSched)
#add_subdirectory (UseCases/combs_calculation)
add_subdirectory (UseCases/ant_colony_pthreads)
#add_subdirectory (UseCases/matmult_pthreads)
#add_subdirectory (UseCases/fluidanimate/src)
#add_subdirectory (examples/raytrace/src)
#add_subdirectory (UseCases/evopro)
add_subdirectory (UseCases/blackscholes/src)
#add_subdirectory (UseCases/synthetic)
add_subdirectory (UseCases/swaptions/src)
#add_subdirectory (examples/CSO_benchmark_omp)
#add_subdirectory (UseCases/CSO_benchmark_ff)

