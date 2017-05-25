set(the_description "imvt opencl-accelerated image processing")
set(OPENCV_MODULE_IS_PART_OF_WORLD OFF)
ocv_define_module(oclrenderpano opencv_imgproc opencv_core opencv_highgui)
target_link_libraries(opencv_oclrenderpano)
