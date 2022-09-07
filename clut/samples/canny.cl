
///////////////////////////////////////////////////////////////////////////////////////////////////
// gaussian Filtering
///////////////////////////////////////////////////////////////////////////////////////////////////
/*
__constant int gaussian_kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1} 
};
*/
__kernel void gaussianFilter(__global const uchar* restrict src, 
                             __global const short* gaussian_kernel,
                             __global uchar* dst, 
                             int src_offset, int src_step, int dst_step)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int cols = get_global_size(0);
    const int rows = get_global_size(1);

    int src_x_off = src_offset % src_step;
    int src_y_off = src_offset / src_step;

    short sum = 0;
    for (int j=y-1; j <= y+1; j++) {
        for (int i=x-1; i <= x+1; i++) {
            int src_x = i < 0 ? 0 : i >= cols ? cols-1 : i;
            int src_y = j < 0 ? 0 : j >= rows ? rows-1 : j;
            sum += src[src_x + src_x_off + (src_y + src_y_off)*src_step]*gaussian_kernel[i-x+1 + 3*(j-y+1)];
        }
    }
    dst[x + y*dst_step]=convert_uchar(sum/16.0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Sobel Filtering
///////////////////////////////////////////////////////////////////////////////////////////////////
/*
sobel_x_kernel[3][3] = { 
        {-1,  0, 1},
        {-2,  0, 2},
        {-1,  0, 1}
};
__constant int sobel_y_kernel[3][3] = { 
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1} 
};
*/
#define TAN_22_5    convert_float(0.414213562373095)    // tan(22.5)
#define TAN_67_5    convert_float(2.414213562373095)    // tan(67.5)
#define TAN_157_5   convert_float(-0.414213562373095)   // tan(157.5)
#define TAN_112_5   convert_float(-2.414213562373095)   // tan(112.5)

__kernel void sobelFilter(__global const uchar* restrict src, 
                          __global const short* restrict sobel_x_kernel, 
                          __global const short* restrict sobel_y_kernel,
                          __global short* magnitude, 
                          __global uchar* theta, 
                          int src_step, int magnidue_step, int theta_step)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int cols = get_global_size(0);
    const int rows = get_global_size(1);

    short sum_x = 0;
    short sum_y = 0;
    for (int j=y-1; j <= y+1; j++) {
        for (int i=x-1; i <= x+1; i++) {
            int src_x = i < 0 ? 0 : i >= cols ? cols-1 : i;
            int src_y = j < 0 ? 0 : j >= rows ? rows-1 : j;
            sum_x += src[src_x + src_y*src_step] * sobel_x_kernel[i-x+1 + 3*(j-y+1)];
            sum_y += src[src_x + src_y*src_step] * sobel_y_kernel[i-x+1 + 3*(j-y+1)];
        }
    }

    magnitude[x + y*(magnidue_step>>1)]= hypot(sum_x, sum_y);

    float tan_theta = convert_float(sum_y/(sum_x + 0.000001));
    if (TAN_22_5 < tan_theta && tan_theta < TAN_67_5) {
        theta[x + y*theta_step] = 45;
    } else if (TAN_112_5 < tan_theta && tan_theta < TAN_157_5) {
        theta[x + y*theta_step] = 135;
    } else if (tan_theta >= TAN_67_5 || tan_theta <= TAN_112_5  ) {
        theta[x + y*theta_step] = 90;
    }  else {
        theta[x + y*theta_step] = 0;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Non-Maximum Suppression
///////////////////////////////////////////////////////////////////////////////////////////////////
__kernel void nonMaxSuppress(__global const short* restrict magnitude, 
                             __global const uchar* restrict theta, 
                             __global short*  dst, 
                             int magnidue_step, int theta_step, int dst_step)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int cols = get_global_size(0);
    const int rows = get_global_size(1);

    short m = magnitude[x + y*(magnidue_step>>1)];
    int angle = theta[x + y*theta_step];
            
    switch (angle) {
    case 0:
        // compare with (x -1, y)  (x + 1, y)
        if ((x > 0 && magnitude[x - 1 + y*(magnidue_step>>1)] > m) ||
            (x < cols-1 && magnitude[x + 1  + y*(magnidue_step>>1)] > m)) {
            dst[x + y*(dst_step>>1)] = 0;   
        } else {
            dst[x + y*(dst_step>>1)] = m; 
        }
        break;
    case 45:
        // compare with (x-1, y+1), (x+1, y-1)
        if ((x > 0 && y < rows-1 && magnitude[x - 1 + (y + 1)*(magnidue_step>>1)] > m) ||
            (x < cols-1 && y > 0 && magnitude[x + 1 + (y - 1)*(magnidue_step>>1)] > m)) {
              dst[x + y*(dst_step>>1)] = 0;   
        } else {
              dst[x + y*(dst_step>>1)] = m;
        }
        break;
    case 90:
        // compare with (x, y-1), (x, y+1)
        if ((y > 0 && magnitude[x + (y - 1)*(magnidue_step>>1)] > m) ||
            (y < rows-1 && magnitude[x + (y + 1)*(magnidue_step>>1)] > m)) {
            dst[x + y*(dst_step>>1)] = 0;  
        } else {
            dst[x + y*(dst_step>>1)] = m; 
        }
        break;
    case 135:
        // compare with (x-1, y-1), (x+1, y+1)
        if ((x > 0 && y > 0 && magnitude[x - 1 + (y - 1)*(magnidue_step>>1)] > m) ||
            (x < cols-1 && y < rows-1  && magnitude[x + 1 + (y + 1)*(magnidue_step>>1)] > m)) {
              dst[x + y*(dst_step>>1)] = 0;   
        } else {
              dst[x + y*(dst_step>>1)] = m; 
        }
        break;
    default:
        dst[x + y*(dst_step>>1)] = 0; 
        break;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Hysteresis Thresholding
///////////////////////////////////////////////////////////////////////////////////////////////////
__kernel void hysteresisThreshold(__global const short* restrict src, __global uchar* dst, 
                                  int src_step, int dst_step, float low_threshold, float high_threshold)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    float magnitude = convert_float(src[x + y*(src_step>>1)]);

    if (magnitude < low_threshold) {
        dst[x + y*dst_step] = 0;  
    } else if (magnitude > high_threshold) {
        dst[x + y*dst_step] = 255;
    } else {
        // Here, we do the approximate estimation in order to reduce computational complexity
        if (2*magnitude > low_threshold + high_threshold) {
            dst[x + y*dst_step] = 255;
        } else {
            dst[x + y*dst_step] = 0;
        }  
    }
}                                  

