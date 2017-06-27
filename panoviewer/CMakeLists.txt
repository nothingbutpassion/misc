#
# Build commands: 
#   mkdir build64 && cd build64
#   cmake -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 14 2015 Win64" ../
#   cd ../ && cmake --build build64 --config Releas
#
cmake_minimum_required(VERSION 2.6.0)
project("PanoTest")

# make sure OpenCV_DIR is set
find_package(OpenCV REQUIRED)
message("OpenCV_LIBS: ${OpenCV_LIBS}")
message("OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")

find_path(OpenCL_INCLUDE_DIRS 
    NAMES OpenCL/cl.h CL/cl.h
    HINTS $ENV{AMDAPPSDKROOT}/include)
find_library(OpenCL_LIBRARIES
	NAMES OpenCL
	HINTS $ENV{AMDAPPSDKROOT}/lib/x86_64)	
message("OpenCL_INCLUDE_DIRS: ${OpenCL_INCLUDE_DIRS}")
message("OpenCL_LIBRARIES: ${OpenCL_LIBRARIES}")
	
add_executable(pano_test
	pano_test.cpp 
	utils.cpp 
	ocl_utils.cpp)
include_directories(${OpenCV_INCLUDE_DIRS} ${OpenCL_INCLUDE_DIRS})
target_link_libraries(pano_test ${OpenCV_LIBS} ${OpenCL_LIBRARIES})

#set_target_properties(pano_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
#get_target_property(MY_EXECUTABLE_OUTPUT_DIR pano_test RUNTIME_OUTPUT_DIRECTORY)
if(MSVC)
	set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin/${BITNESS_SUFFIX})
else()
	set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin/${BITNESS_SUFFIX}/${CMAKE_BUILD_TYPE})
endif()

add_custom_command(
	TARGET pano_test POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/sum.cl ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR} 
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/plane_to_sphere_map.cl ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}
	)