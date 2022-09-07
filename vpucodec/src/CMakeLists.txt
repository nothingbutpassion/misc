cmake_minimum_required(VERSION 2.8)

project(vpu_codec)

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


# defines
add_definitions(-DLOG_LEVEL=LOG_ERROR+1)

# include dir
include_directories(${CMAKE_CURRENT_SOURCE_DIR})


#
# build libvpu_codec.so
#

# lib source files
set(LIB_SOURCE_FILES
	vpu_codec.cpp
	vpu_enc_jpeg.cpp
	vpu_enc_h264.cpp
	vpu_enc.cpp
	vpu_dec_jpeg.cpp
	vpu_dec_h264.cpp
	vpu_dec.cpp
	vpu_fb.cpp
	vpu_utils.cpp
    )

# set shared lib target
add_library(vpu_codec SHARED ${LIB_SOURCE_FILES})

# set lib output dir
set_property(TARGET vpu_codec PROPERTY LIBRARY_OUTPUT_DIRECTORY ../dist/${TARGET_ARCH}/lib)

# copy lib header files
add_custom_target(install_include ALL
	mkdir -p ../dist/${TARGET_ARCH}/include
	COMMAND cp -v ../src/vpu_codec.h ../dist/${TARGET_ARCH}/include/
	DEPENDS vpu_codec)

#
# build vpu_codec_test
#

# exe source files
set(TEST_SOURCE_FILES 
	vpu_codec_test.cpp
	vpu_codec.cpp
	vpu_enc_jpeg.cpp
	vpu_enc_h264.cpp
	vpu_enc.cpp
	vpu_dec_jpeg.cpp
	vpu_dec_h264.cpp
	vpu_dec.cpp
	vpu_fb.cpp
	vpu_utils.cpp
	)

# add executable target
add_executable(vpu_codec_test ${TEST_SOURCE_FILES})

# add dependencies
add_dependencies(vpu_codec_test vpu_codec)

# set executable output dir
set_property(TARGET vpu_codec_test PROPERTY RUNTIME_OUTPUT_DIRECTORY ../dist/${TARGET_ARCH}/bin)

# link libs
target_link_libraries(vpu_codec_test vpu ipu rt pthread)