#
# Build commands: 
#   mkdir build
#   cd build64
#   cmake -G "Visual Studio 14 2015 Win64" ../
#   cmake --build . --config Release
#
cmake_minimum_required(VERSION 2.6.0)
project("PanoTest")

# make sure OpenCV_DIR is set
find_package(OpenCV REQUIRED)
message("OpenCV_LIBS: ${OpenCV_LIBS}")
message("OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")

find_path(OpenCL_INCLUDE_DIRS 
    NAMES OpenCL/cl.h CL/cl.h
    HINTS $ENV{OpenCL_DIR}/include)
find_library(OpenCL_LIBS
	NAMES OpenCL
	HINTS $ENV{OpenCL_DIR}/lib)	
message("OpenCL_INCLUDE_DIRS: ${OpenCL_INCLUDE_DIRS}")
message("OpenCL_LIBS: ${OpenCL_LIBS}")
	


if (OpenCL_INCLUDE_DIRS AND OpenCL_LIBS)
    add_executable(pano_test
        pano_test.cpp
        utils.cpp
        ocl_utils.cpp)
	include_directories(${OpenCV_INCLUDE_DIRS} ${OpenCL_INCLUDE_DIRS})
    target_compile_definitions(pano_test -DUSE_OPENCL)
	target_link_libraries(pano_test ${OpenCV_LIBS} ${OpenCL_LIBS})
else()
    add_executable(pano_test
        pano_test.cpp
        utils.cpp)
	include_directories(${OpenCV_INCLUDE_DIRS})
	target_link_libraries(pano_test ${OpenCV_LIBS})
endif()

if (MSVC)
	set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()

add_custom_command(
	TARGET pano_test POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/plane_to_sphere_map.cl ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})