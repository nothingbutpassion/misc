cmake_minimum_required(VERSION 2.8)
project(file_server)

# cxx flags
if (${DEBUG})
    set (CMAKE_CXX_FLAGS "-g -Wall ${CMAKE_CXX_FLAGS}")
else ()
    set (CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
endif ()
message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# set executable target for file_server
add_executable(file_server utils.cpp file_server.cpp)

# link libraries
target_link_libraries(file_server pthread)


# set executable target for file_client
add_executable(file_client file_client.cpp)