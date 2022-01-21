# ------------------------------- SOURCES ---------------------------------

SET(AntColonyPthreads_w_Sched_SRCS
  AntPthreadsSched.cpp
  ThreadRoutine.h
  ThreadRoutine.cpp
)

SET(AntColonyPthreads_wo_Sched_SRCS
  AntPthreadsNoSched.cpp)
# ------------------------------- TARGETS --------------------------------

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})
include_directories(${Hwloc_INCLUDE_DIRS})

SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

add_executable(AntColonyPthreads_w_Sched ${AntColonyPthreads_w_Sched_SRCS})
add_executable(AntColonyPthreads_wo_Sched ${AntColonyPthreads_wo_Sched_SRCS})
target_link_libraries( AntColonyPthreads_wo_Sched pthread)
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(AntColonyPthreads_w_Sched parlsched "${CMAKE_THREAD_LIBS_INIT}" "${PAPI_LIBRARIES}" "${NUMA_LIBRARY}" "${Hwloc_LIBRARIES}")
endif()




#target_link_libraries( SchedulingExample pthread)

