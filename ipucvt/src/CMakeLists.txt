cmake_minimum_required(VERSION 2.8)

project(ipu_cvt)

# Check the platform arch
if (DEFINED ENV{CROSS_COMPILE})
    set (TARGET_ARCH "arm")
else ()
    # NOTES: Here should report error and exit
    # set (TARGET_ARCH "x64")
    message(FATAL_ERROR "You can NOT build for x64, CMake will exit." )
endif ()

# cxx flags
if ($ENV{COMPILE_MODE} STREQUAL "release")
    set (CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
else ()
    set (CMAKE_CXX_FLAGS "-g -Wall ${CMAKE_CXX_FLAGS}")
endif ()

message(STATUS "Target Arch: ${TARGET_ARCH}")
message(STATUS "CMake CXX Flags: ${CMAKE_CXX_FLAGS}")

# include dir
include_directories(${CMAKE_CURRENT_SOURCE_DIR})


#
# build libipu_cvt.so
#

# lib source files
set(LIB_SOURCE_FILES
	ipu_cvt.cpp
	ipu_utils.cpp
    )

# set shared lib target
add_library(ipu_cvt SHARED ${LIB_SOURCE_FILES})

# set lib output dir
set_property(TARGET ipu_cvt PROPERTY LIBRARY_OUTPUT_DIRECTORY ../dist/${TARGET_ARCH}/lib)

# copy lib header files
add_custom_target(install_include ALL
	mkdir -p ../dist/${TARGET_ARCH}/include
	COMMAND cp -v ../src/ipu_cvt.h ../dist/${TARGET_ARCH}/include/
	DEPENDS ipu_cvt)

#
# build ipu_cvt_test
#

# exe source files
set(TEST_SOURCE_FILES 
	ipu_cvt_test.cpp
	ipu_cvt.cpp
	ipu_utils.cpp
	)

# add executable target
add_executable(ipu_cvt_test ${TEST_SOURCE_FILES})

# add dependencies
add_dependencies(ipu_cvt_test ipu_cvt)

# set executable output dir
set_property(TARGET ipu_cvt_test PROPERTY RUNTIME_OUTPUT_DIRECTORY ../dist/${TARGET_ARCH}/bin)

