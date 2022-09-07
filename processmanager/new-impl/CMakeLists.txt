cmake_minimum_required(VERSION 2.8)
project(test_process_manager)

# cxx flags
if (${DEBUG})
    set (CMAKE_CXX_FLAGS "-std=c++11 -g -Wall ${CMAKE_CXX_FLAGS}")
else ()
    set (CMAKE_CXX_FLAGS "-std=c++11 -O2 ${CMAKE_CXX_FLAGS}")
endif ()
message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# include directories
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	)

# source files
set(SOURCE_FILES
	process.cpp
	test_process_manager.cpp
	)

# set executable target
add_executable(test_process_manager ${SOURCE_FILES})

# link libraries
target_link_libraries(test_process_manager
	pthread
	)
