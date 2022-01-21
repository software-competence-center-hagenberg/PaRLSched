# ------------------------------- SOURCES ---------------------------------

SET(blackscholes_SRCS
  blackscholes-pthreads.cpp)

SET(inputgen_SRCS
  inputgen.cpp
)

# ------------------------------- TARGETS --------------------------------

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})
include_directories(${Hwloc_INCLUDE_DIRS})

SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

add_definitions(-DNCO=4)
add_executable(blackscholes_pthreads ${blackscholes_SRCS})
add_executable(blackscholes_pthreads_sched ${blackscholes_SRCS})
add_executable(inputgen ${inputgen_SRCS})
target_compile_definitions(blackscholes_pthreads_sched PUBLIC -DSCHEDULER)
target_link_libraries( blackscholes_pthreads pthread)
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(blackscholes_pthreads_sched parlsched "${CMAKE_THREAD_LIBS_INIT}" "${PAPI_LIBRARIES}" "${NUMA_LIBRARY}" "${Hwloc_LIBRARIES}")
endif()
install(TARGETS blackscholes_pthreads_sched DESTINATION bin)

