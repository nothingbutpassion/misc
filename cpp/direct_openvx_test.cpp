//
// Build cmd: $CXX direct_openvx_test.cpp -o direct_openvx_test -lopencv_core -lopencv_imgproc -lopencv_imgcodecs  -lOpenVX -lOpenVXU 
//
#include <stdio.h>
#include <vector>
#include <iostream>
#include "VX/vx.h"
#include "VX/vxu.h"
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

using namespace std;
using namespace cv;

struct TimeTrack {
   TimeTrack() {
       start = getTickCount();
   }
   void reset() {
       durations.clear();
       start = getTickCount();
   }
   void mark(double devider=1.0) { 
       durations.push_back(double(getTickCount()- start)*1000/(devider*getTickFrequency()));
       start = getTickCount();
   }
   double operator[](int i) {
       return durations[i];
   }
   
private:
   int64_t start;
   std::vector<double> durations;
};


template <typename T>
void checkStatus(T reference, const char* msg) {
    vx_status status = vxGetStatus((vx_reference)reference);
    if (status != VX_SUCCESS) {
        printf("%s failed with status: %d\n", msg, status);
        exit(-1);
    }
}
template<>
void checkStatus<vx_status>(vx_status status, const char* msg) {
    if (status != VX_SUCCESS) {
        printf("%s failed with status: %d\n", msg, status);
        exit(-1);
    }
}

inline vx_df_image matTypeToFormat(int matType) {
    switch (matType) {
    case CV_8UC4:  return VX_DF_IMAGE_RGBX;
    case CV_8UC3:  return VX_DF_IMAGE_RGB;
    case CV_8UC1:  return VX_DF_IMAGE_U8;
    case CV_16UC1: return VX_DF_IMAGE_U16;
    case CV_16SC1: return VX_DF_IMAGE_S16;
    case CV_32SC1: return VX_DF_IMAGE_S32;
    case CV_32FC1: return VX_DF_IMAGE('F', '0', '3', '2');
    }
}

inline int formatToMatType(vx_df_image format, vx_uint32 planeIdx = 0) {
    switch (format) {
    case VX_DF_IMAGE_RGB:  return CV_8UC3;
    case VX_DF_IMAGE_RGBX: return CV_8UC4;
    case VX_DF_IMAGE_U8:   return CV_8UC1;
    case VX_DF_IMAGE_U16:  return CV_16UC1;
    case VX_DF_IMAGE_S16:  return CV_16SC1;
    case VX_DF_IMAGE_U32:
    case VX_DF_IMAGE_S32:  return CV_32SC1;
    case VX_DF_IMAGE('F', '0', '3', '2'):
                           return CV_32FC1;
    case VX_DF_IMAGE_YUV4:
    case VX_DF_IMAGE_IYUV: return CV_8UC1;
    case VX_DF_IMAGE_UYVY:
    case VX_DF_IMAGE_YUYV: return CV_8UC2;
    case VX_DF_IMAGE_NV12:
    case VX_DF_IMAGE_NV21: return planeIdx == 0 ? CV_8UC1 : CV_8UC2;
    default: return CV_USRTYPE1;
    }
}

vx_threshold createRange(vx_context context, vx_enum dataType, vx_int32 valLower, vx_int32 valUpper) {
    vx_threshold threshold = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, dataType);
    checkStatus(threshold, "vxCreateThreshold");
    vx_status status = vxSetThresholdAttribute(threshold, VX_THRESHOLD_THRESHOLD_LOWER, &valLower, sizeof(valLower));
    checkStatus(status, "vxSetThresholdAttribute");
    status = vxSetThresholdAttribute(threshold, VX_THRESHOLD_THRESHOLD_UPPER, &valUpper, sizeof(valUpper));
    checkStatus(status, "vxSetThresholdAttribute");
    return threshold;
}

vx_image createImageFromMat(vx_context context, Mat& mat) {
    //
    // NOTES:
    // Only dim_x, dim_y, stride_x and stride_y fields of the vx_imagepatch_addressing_t need to be provided
    // Other fields (step_x, step_y, scale_x & scale_y) are ignored 
    // The layout of the imported memory must follow a row-major order. 
    // In other words, stride_x should be sufficiently large so that there is no overlap between data elements 
    // corresponding to different pixels, and stride_y >= stride_x * dim_x.
    //
    vx_imagepatch_addressing_t addr;
    addr.dim_x = mat.cols;
    addr.dim_y = mat.rows;
    addr.stride_x = mat.elemSize();
    addr.stride_y = mat.step;
    void* ptr[] = { mat.data };
    vx_df_image color = matTypeToFormat(mat.type());
    vx_image image = vxCreateImageFromHandle(context, color, &addr, ptr, VX_MEMORY_TYPE_HOST );
    checkStatus(image, "vxCreateImageFromHandle");
    return image;
}

vx_map_id map(vx_image image, Mat& mat) {
    vx_rectangle_t rect;
    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x = mat.cols;
    rect.end_y = mat.rows;
    vx_imagepatch_addressing_t addr;
    void* ptr = 0;
    vx_map_id mapId;
    vx_status status = vxMapImagePatch(image, &rect, 0, &mapId, &addr, &ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    checkStatus(image, "vxMapImagePatch");
    return mapId;
}

void unmap(vx_image image, vx_map_id mapId) {
    vx_status status = vxUnmapImagePatch(image, mapId);
    checkStatus(image, "vxUnmapImagePatch");
}

void testDilate3x3(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(gray.size(), CV_8UC1);

    TimeTrack tt;
    vx_image input  = createImageFromMat(context, m1);
    vx_image output = createImageFromMat(context, m2);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuDilate3x3(context, input, output);
        checkStatus(status, "vxuDilate3x3");
    }
    tt.mark(runTimes);
    
    vx_map_id id1 = map(input, m1);
    vx_map_id id2 = map(output, m2);
    //imwrite("vxuDilate3x3_output.jpg", m2);
    unmap(input, id1);
    unmap(output, id2);
    vxReleaseImage(&input);
    vxReleaseImage(&output);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) { 
        //
        // NOTES: 
        // OpenCV throw 'ivx::RuntimeError' (on i.MX8 target) 
        // what():  query() : vxQueryContext(ref, att, &value, sizeof(value))
        //
        //dilate(m1, m2, Mat());
    }
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "dilate3x3", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "dilate3x3", tt[3]);
}

void testErode3x3(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(gray.size(), CV_8UC1);

    TimeTrack tt;
    vx_image input  = createImageFromMat(context, m1);
    vx_image output = createImageFromMat(context, m2);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuErode3x3(context, input, output);
        checkStatus(status, "vxuErode3x3");
    }
    tt.mark(runTimes);
    
    vx_map_id id1 = map(input, m1);
    vx_map_id id2 = map(output, m2);
    //imwrite("vxuErode3x3_output.jpg", m2);
    unmap(input, id1);
    unmap(output, id2);
    vxReleaseImage(&input);
    vxReleaseImage(&output);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) { 
        //
        // NOTES: 
        // OpenCV throw 'ivx::RuntimeError' (on i.MX8 target) 
        // what():  query() : vxQueryContext(ref, att, &value, sizeof(value))
        //
        //erode(m1, m2, Mat());
    }
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "erode3x3", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "erode3x3", tt[3]);
}

void testGaussian3x3(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(gray.size(), CV_8UC1);
    
    TimeTrack tt;
    vx_image input  = createImageFromMat(context, m1);
    vx_image output = createImageFromMat(context, m2);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuGaussian3x3(context, input, output);
        checkStatus(status, "vxuGaussian3x3");
    }
    tt.mark(runTimes);
    
    
    vx_map_id id1 = map(input, m1);
    vx_map_id id2 = map(output, m2);
    //imwrite("vxuGaussian3x3_output.jpg", m2);
    unmap(input, id1);
    unmap(output, id2);
    vxReleaseImage(&input);
    vxReleaseImage(&output);
    tt.mark();

    for (int i=0; i < runTimes; ++i) {
        GaussianBlur(m1, m2, Size(3, 3), 0);
    }
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "Gaussian3x3", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "Gaussian3x3", tt[3]);
}

void testSobel3x3(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(gray.size(), CV_16SC1);
    Mat m3(gray.size(), CV_16SC1);
    
    TimeTrack tt;
    int64_t start = getTickCount();
    vx_image input    = createImageFromMat(context, m1);
    vx_image output_x = createImageFromMat(context, m2);
    vx_image output_y = createImageFromMat(context, m3);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuSobel3x3(context, input, output_x, output_y);
        checkStatus(status, "vxuSobel3x3");
    }
    tt.mark(runTimes);
    
    vx_map_id id1 = map(input, m1);
    vx_map_id id2 = map(output_x, m2);
    vx_map_id id3 = map(output_y, m3);
    // Mat abs_grad_x;
    // Mat abs_grad_y;
    // convertScaleAbs(m2, abs_grad_x);
    // convertScaleAbs(m3, abs_grad_y);
    // imwrite("vxuSobel3x3_output_x.jpg", abs_grad_x);
    // imwrite("vxuSobel3x3_output_y.jpg", abs_grad_y);
    unmap(input, id1);
    unmap(output_x, id2);
    unmap(output_y, id3);
    vxReleaseImage(&input);
    vxReleaseImage(&output_x);
    vxReleaseImage(&output_x);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        Sobel(m1, m2, CV_16S, 1, 0, 3);
        Sobel(m1, m3, CV_16S, 0, 1, 3);
    }
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "Sobel3x3", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "Sobel3x3", tt[3]);  
}

void testBoxFilter3x3(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(gray.size(), CV_8UC1);
    
    TimeTrack tt;
    vx_image input  = createImageFromMat(context, m1);
    vx_image output = createImageFromMat(context, m2);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuBox3x3(context, input, output);
        checkStatus(status, "vxuBox3x3");
    }
    tt.mark(runTimes);
    
    vx_map_id id1 = map(input, m1);
    vx_map_id id2 = map(output, m2);
    //imwrite("vxuBox3x3_output.jpg", m2);
    unmap(input, id1);
    unmap(output, id2);
    vxReleaseImage(&input);
    vxReleaseImage(&output);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        boxFilter(m1, m2, -1, Size(3, 3));
    }
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "boxFilter3x3", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "boxFilter3x3", tt[3]);   
}

void testCanny(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(gray.size(), CV_8UC1);
    
    TimeTrack tt;
    vx_threshold threshold = createRange(context, VX_TYPE_UINT8, 100, 200);
    vx_image input  = createImageFromMat(context, m1);
    vx_image output = createImageFromMat(context, m2);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuCannyEdgeDetector(context, input, threshold, 3, VX_NORM_L2, output);
        checkStatus(status, "Canny");
    }
    tt.mark(runTimes);
    
    vx_map_id id1 = map(input, m1);
    vx_map_id id2 = map(output, m2);
    //imwrite("vxuCannyEdgeDetector_output.jpg", m2);
    unmap(input, id1);
    unmap(output, id2);
    vxReleaseImage(&input);
    vxReleaseImage(&output);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        Canny(m1, m2, 100, 200, 3, true);
    }
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "Canny", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "Canny", tt[3]);
}  

void testCalcHist(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(Size(1, 64), CV_32SC1);
    
    TimeTrack tt;
    vx_image input  = createImageFromMat(context, m1);
    tt.mark();
    
    vx_distribution distribution = vxCreateDistribution	(context, 64, 0, 255);
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuHistogram(context, input, distribution);
        checkStatus(status, "vxuHistogram");
    }
    tt.mark(runTimes);
    
    vxCopyDistribution(distribution, m2.data, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    //cout << m2 << endl;
    vxReleaseDistribution(&distribution);
    vxReleaseImage(&input);
    tt.mark();
    
    int channel = 0;
    int bins = 64;
    float sranges[] = { 0, 255 };
    const float* ranges[] = { sranges };      
    Mat hist;
    for (int i=0; i < runTimes; ++i) {
        calcHist(&m1, 1, &channel, Mat(), hist, 1, &bins, ranges, true, false);
    }
    //cout << hist << endl;
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "calcHist", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "calcHist", tt[3]);
}

void testMinMaxLoc(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    
    TimeTrack tt;
    uchar min = 0;
    uchar max = 0;
    vx_scalar minVal = vxCreateScalar(context, VX_TYPE_UINT8, &min);
    vx_scalar maxVal = vxCreateScalar(context, VX_TYPE_UINT8, &max);
    vx_array minLoc = vxCreateArray(context, VX_TYPE_COORDINATES2D, 1);
    vx_array maxLoc = vxCreateArray(context, VX_TYPE_COORDINATES2D, 1);
    vx_image input  = createImageFromMat(context, m1);
    vx_distribution distribution = vxCreateDistribution	(context, 64, 0, 255);
    tt.mark();
    
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuMinMaxLoc(context, input, minVal, maxVal, minLoc, maxLoc, 0, 0);
        checkStatus(status, "vxuMinMaxLoc");
    }
    tt.mark(runTimes);
    
    vxCopyScalar(minVal, &min, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyScalar(maxVal, &max, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vx_coordinates2d_t minpos;
    vx_coordinates2d_t maxpos;
    vxCopyArrayRange(minLoc, 0, 1, sizeof(minpos), &minpos, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyArrayRange(maxLoc, 0, 1, sizeof(maxpos), &maxpos, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    //printf("min=%d, max=%d, min_pos=(%d,%d), max_pos=(%d,%d)\n", min, max, minpos.x, minpos.y, maxpos.x, maxpos.y);
    vxReleaseScalar(&minVal);
    vxReleaseScalar(&maxVal);
    vxReleaseArray(&minLoc);	
    vxReleaseArray(&maxLoc);
    vxReleaseImage(&input);
    tt.mark();
    
    double minv;
    double maxv;
    Point minPos;
    Point maxPos;
    for (int i=0; i < runTimes; ++i) {
        minMaxLoc(m1, &minv, &maxv, &minPos, &maxPos);
    }
    //printf("min=%.6f, max=%.6f, min_pos=(%d,%d), max_pos=(%d,%d)\n", minv, maxv, minPos.x, minPos.y, maxPos.x, maxPos.y);
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "minMaxLoc", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "minMaxLoc", tt[3]);
}

void testRemap(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(gray.size(), CV_8UC1);
    int64_t start = getTickCount();
    vx_remap table = vxCreateRemap(context, m1.cols, m1.rows, m2.cols, m2.rows);
    for (int y=0; y < m2.rows; ++y) {
        for (int x=0; x < m2.cols; ++x) {
            vxSetRemapPoint(table, x, y, m1.cols-1-x, m1.rows-1-y);
        }
    }
    
    TimeTrack tt;
    vx_image input  = createImageFromMat(context, m1);
    vx_image output = createImageFromMat(context, m2);
    tt.mark();
    
    vx_distribution distribution = vxCreateDistribution	(context, 64, 0, 255);
    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuRemap(context, input, table, VX_INTERPOLATION_BILINEAR, output);
        checkStatus(status, "vxuRemap");
    }
    tt.mark(runTimes);

    vx_map_id id1 = map(input, m1);
    vx_map_id id2 = map(output, m2);
    //imwrite("vxuRemap_output.jpg", m2);
    unmap(input, id1);
    unmap(output, id2);
    vxReleaseRemap(&table);
    vxReleaseImage(&input);
    vxReleaseImage(&output);
    tt.mark();
    
    
    Mat mapxy(gray.size(), CV_32FC2);
    for (int y=0; y < m2.rows; ++y) {
        for (int x=0; x < m2.cols; ++x) {
            mapxy.at<Point2f>(y, x) = Point2f(m1.cols-1-x, m1.rows-1-y);
        }
    }
    
    tt.mark();
    for (int i=0; i < runTimes; ++i) {
        remap(m1, m2, mapxy, Mat(), INTER_LINEAR);
    }
    //imwrite("rmap_output.jpg", m2);
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "remap", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "remap", tt[4]);
}


void testResize(vx_context& context, Mat& image, Mat& gray, int runTimes) {
    Mat m1 = gray.clone();
    Mat m2(gray.size()/2, CV_8UC1);
    
    TimeTrack tt;
    vx_image input  = createImageFromMat(context, m1);
    vx_image output = createImageFromMat(context, m2);
    tt.mark();

    for (int i=0; i < runTimes; ++i) {
        vx_status status = vxuScaleImage(context, input, output, VX_INTERPOLATION_BILINEAR);
        checkStatus(status, "vxuScaleImage");
    }
    tt.mark(runTimes);

    vx_map_id id1 = map(input, m1);
    vx_map_id id2 = map(output, m2);
    //imwrite("vxuScaleImage_output.jpg", m2);
    unmap(input, id1);
    unmap(output, id2);
    vxReleaseImage(&input);
    vxReleaseImage(&output);
    tt.mark();

    for (int i=0; i < runTimes; ++i) {
        resize(m1, m2, m2.size(), 0, 0, INTER_LINEAR);
    }
    //imwrite("resize_output.jpg", m2);
    tt.mark(runTimes);
    printf("OpenVX %-16s %-6.1f %-6.1f %-6.1f\n", "resize", tt[1], tt[0], tt[2]);
    printf("OpenCV %-16s %-6.1f\n", "resize", tt[3]);
}
  
int main(int argc, char* argv[]) {
    
    if (argc < 2) {
        printf("Usage: %s <imageFile> [runTimes]\n", argv[0]);
		return -1;
	}

    // Load one color image and one gray image for testing
    Mat img = imread(argv[1], IMREAD_COLOR);
	Mat gray = imread(argv[1], IMREAD_GRAYSCALE);
	if (img.empty()) {
		printf("Can't open image file: %s\n", argv[1]);
		return -1;
	}
    
    // Total run times for each opencv/openvx function
    int runTimes = 1000;
    if (argc > 2)
        runTimes = atoi(argv[2]);
    
    // Disable OpenVX in OpenCV
    setUseOpenVX(false);
    
    // Create OpenVX Context
    vx_context context = vxCreateContext();
    checkStatus(context, "vxCreateContext");
  
    // Dilate
    testDilate3x3(context, img, gray, runTimes);
   
    // Erode
    testErode3x3(context, img, gray, runTimes);
    
    // vxuGaussian3x3
    testGaussian3x3(context, img, gray, runTimes);

    // Soble3x3
    testSobel3x3(context, img, gray, runTimes);
    
    // boxFilter3x3
    testBoxFilter3x3(context, img, gray, runTimes);
    
    // Canny
    testCanny(context, img, gray, runTimes);

    // calcHist
    testCalcHist(context, img, gray, runTimes);
    
    // minMaxLoc
    testMinMaxLoc(context, img, gray, runTimes);

    // remap
    testRemap(context, img, gray, runTimes);
    
    // resize
    testResize(context, img, gray, runTimes);
    
    // Release OpenVX Context
    vx_status status = vxReleaseContext(&context);
    checkStatus(status, "vxReleaseContext");
    
    return 0;
}
