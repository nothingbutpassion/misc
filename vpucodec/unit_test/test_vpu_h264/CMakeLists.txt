cmake_minimum_required (VERSION 2.8)
project (test_vpu_h264)

#
# All internal variables are named with prefix "MY_"
# 

# Check the platform arch
if (DEFINED ENV{CROSS_COMPILE})
    set (MY_PLATFORM_ARCH "arm")
else ()
    set (MY_PLATFORM_ARCH "x64")
endif ()

# cxx flags
# NOTES: you should add "-fprofile-arcs -ftest-coverage -fPIC" flags for gcov
set (CMAKE_CXX_FLAGS "-std=c++11 -fprofile-arcs -ftest-coverage -fPIC -g -Wall ${CMAKE_CXX_FLAGS}")

# source files
set (MY_SOURCE_FILES
	${PROJECT_SOURCE_DIR}/../../src/vpu_enc_h264.cpp
	${PROJECT_SOURCE_DIR}/../../src/vpu_dec_h264.cpp
	${PROJECT_SOURCE_DIR}/test_vpu_h264.cpp
	)

# build static library taget
add_executable (test_vpu_h264 ${MY_SOURCE_FILES})


# include directories
# NOTES: target_include_directories () is preferable if cmake support
include_directories (${PROJECT_SOURCE_DIR})
include_directories (${PROJECT_SOURCE_DIR}/../common)
include_directories (${PROJECT_SOURCE_DIR}/../../src)
include_directories (${PROJECT_SOURCE_DIR}/../../../gmock/dist/${MY_PLATFORM_ARCH}/include)

# add -D define flags to the compilation of source files
add_definitions(-Dprivate=public -DLOG_LEVEL=LOG_ERROR+1)

# link libraries
find_library(MY_GMOCK_LIBRARY roadDB_gmock HINTS ${PROJECT_SOURCE_DIR}/../../../gmock/dist/${MY_PLATFORM_ARCH}/lib)
target_link_libraries (test_vpu_h264 ${MY_GMOCK_LIBRARY})


# specify the generated executable output directory
set_target_properties(test_vpu_h264 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../dist/${MY_PLATFORM_ARCH}/bin)
