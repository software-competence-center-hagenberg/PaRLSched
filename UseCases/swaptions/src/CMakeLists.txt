# ------------------------------- SOURCES ---------------------------------

SET(swaptions_SRCS
	nr_routines.cpp CumNormalInv.cpp  HJM.cpp  HJM_Securities.cpp  HJM_SimPath_Forward_Blocking.cpp  HJM_Swaption_Blocking.cpp  icdf.cpp  MaxFunction.cpp  RanUnif.cpp
)

# ------------------------------- TARGETS --------------------------------

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})
include_directories(${Hwloc_INCLUDE_DIRS})

SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

add_executable(swaptions ${swaptions_SRCS})
target_link_libraries( swaptions pthread)
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(swaptions parlsched "${CMAKE_THREAD_LIBS_INIT}" "${PAPI_LIBRARIES}" "${NUMA_LIBRARY}" "${Hwloc_LIBRARIES}")
endif()
install(TARGETS swaptions DESTINATION bin)

