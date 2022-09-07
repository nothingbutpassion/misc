/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2010-2012, Institute Of Software Chinese Academy Of Science, all rights reserved.
// Copyright (C) 2010-2012, Advanced Micro Devices, Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// @Authors
//    Zhang Ying, zhangying913@gmail.com
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other oclMaterials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors as is and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Build options: "-DanX=<anchor-x> -DanY=<anchor-y> -DksX=<kernel-width> -DksY=<kernel-height> -D<border-type>"
//
// Anchor: anX, anY 
// 
// Kernel size: ksX, ksY
// 
// Various border types, image boundaries are denoted with '|'
// BORDER_REPLICATE:     aaaaaa|abcdefgh|hhhhhhh
// BORDER_REFLECT:       fedcba|abcdefgh|hgfedcb
// BORDER_REFLECT_101:   gfedcb|abcdefgh|gfedcba
// BORDER_WRAP:          cdefgh|abcdefgh|abcdefg  (not supported)
// BORDER_CONSTANT:      iiiiii|abcdefgh|iiiiiii  with some specified 'i' 
//
/////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////8uC1////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
__kernel void boxFilter_C1_D0(__global const uchar* restrict src, __global uchar* dst, float alpha,
                                     int src_offset, int src_whole_rows, int src_whole_cols, int src_step,
                                     int dst_offset, int dst_rows, int dst_cols, int dst_step
                                     )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    
    int src_x_off = src_offset % src_step;
    int src_y_off = src_offset / src_step;
    int dst_x_off = dst_offset % dst_step;
    int dst_y_off = dst_offset / dst_step;
    
    uint sum = 0;
    for (int i=x-anX; i < x-anX+ksX; i++) {
        for (int j=y-anY; j < y-anY+ksY; j++) {
#if defined BORDER_CONSTANT 
            int src_x = i < 0 ? 0 : i >= dst_cols ? 0 : i;
            int src_y = j < 0 ? 0 : j >= dst_rows ? 0 : j;
#elif defined BORDER_REPLICATE 
            int src_x = i < 0 ? 0 : i >= dst_cols ? dst_cols-1 : i;
            int src_y = j < 0 ? 0 : j >= dst_rows ? dst_rows-1 : j;
#elif defined BORDER_REFLECT
            int src_x = i < 0 ? -i-1 : i >= dst_cols ? -i-1 + (dst_cols << 1) : i;
            int src_y = j < 0 ? -j-1 : j >= dst_rows ? -j-1 + (dst_rows << 1) : j;
#elif defined BORDER_REFLECT_101
            int src_x = i < 0 ? -i : i >= dst_cols ? -i-2 + (dst_cols << 1) : i;
            int src_y = j < 0 ? -i : j >= dst_rows ? -j-2 + (dst_rows << 1) : j;
#endif
            sum += src[src_x + src_x_off + (src_y + src_y_off)*src_step];
        }
    }
    dst[x + dst_x_off + (y + dst_y_off)*dst_step]=convert_uchar(sum/alpha);
}                                    

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////8uC////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
__kernel void boxFilter_C4_D0(__global const uchar4* restrict src, __global uchar4* dst, float alpha,
                                     int src_offset, int src_whole_rows, int src_whole_cols, int src_step,
                                     int dst_offset, int dst_rows, int dst_cols, int dst_step
                                     )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    int src_x_off = (src_offset % src_step) >> 2;
    int src_y_off = src_offset / src_step;
    int dst_x_off = (dst_offset % dst_step) >> 2;
    int dst_y_off = dst_offset / dst_step;
    
    uint4 sum = (uint4)(0, 0, 0, 0);
    for (int i=x-anX; i < x-anX+ksX; i++) {
        for (int j=y-anY; j < y-anY+ksY; j++) {
#if defined BORDER_CONSTANT 
            int src_x = i < 0 ? 0 : i >= dst_cols ? 0 : i;
            int src_y = j < 0 ? 0 : j >= dst_rows ? 0 : j;
#elif defined BORDER_REPLICATE 
            int src_x = i < 0 ? 0 : i >= dst_cols ? dst_cols-1 : i;
            int src_y = j < 0 ? 0 : j >= dst_rows ? dst_rows-1 : j;
#elif defined BORDER_REFLECT
            int src_x = i < 0 ? -i-1 : i >= dst_cols ? -i-1 + (dst_cols << 1) : i;
            int src_y = j < 0 ? -j-1 : j >= dst_rows ? -j-1 + (dst_rows << 1) : j;
#elif defined BORDER_REFLECT_101
            int src_x = i < 0 ? -i : i >= dst_cols ? -i-2 + (dst_cols << 1) : i;
            int src_y = j < 0 ? -i : j >= dst_rows ? -j-2 + (dst_rows << 1) : j;
#endif
            sum += convert_uint4(src[src_x + src_x_off + (src_y + src_y_off)*(src_step>>2)]);
        }
    }
    dst[x + dst_x_off + (y + dst_y_off)*(dst_step>>2)]=convert_uchar4(convert_float4(sum)/alpha);
}         


///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////32fC1////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
__kernel void boxFilter_C1_D5(__global const float *restrict src, __global float *dst, float alpha,
                                     int src_offset, int src_whole_rows, int src_whole_cols, int src_step,
                                     int dst_offset, int dst_rows, int dst_cols, int dst_step
                                     )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    int src_x_off = (src_offset % src_step) >> 2;
    int src_y_off = src_offset / src_step;
    int dst_x_off = (dst_offset % dst_step) >> 2;
    int dst_y_off = dst_offset / dst_step;
    
    float sum = 0.0;
    for (int i=x-anX; i < x-anX+ksX; i++) {
        for (int j=y-anY; j < y-anY+ksY; j++) {
#if defined BORDER_CONSTANT 
            int src_x = i < 0 ? 0 : i >= dst_cols ? 0 : i;
            int src_y = j < 0 ? 0 : j >= dst_rows ? 0 : j;
#elif defined BORDER_REPLICATE 
            int src_x = i < 0 ? 0 : i >= dst_cols ? dst_cols-1 : i;
            int src_y = j < 0 ? 0 : j >= dst_rows ? dst_rows-1 : j;
#elif defined BORDER_REFLECT
            int src_x = i < 0 ? -i-1 : i >= dst_cols ? -i-1 + (dst_cols << 1) : i;
            int src_y = j < 0 ? -j-1 : j >= dst_rows ? -j-1 + (dst_rows << 1) : j;
#elif defined BORDER_REFLECT_101
            int src_x = i < 0 ? -i : i >= dst_cols ? -i-2 + (dst_cols << 1) : i;
            int src_y = j < 0 ? -i : j >= dst_rows ? -j-2 + (dst_rows << 1) : j;
#endif
            sum += src[src_x + src_x_off + (src_y + src_y_off)*(src_step>>2)]/alpha;
        }
    }
    dst[x + dst_x_off + (y + dst_y_off)*(dst_step>>2)]=sum;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////32fC4////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
__kernel void boxFilter_C4_D5(__global const float4 *restrict src, __global float4 *dst, float alpha,
                                     int src_offset, int src_whole_rows, int src_whole_cols, int src_step,
                                     int dst_offset, int dst_rows, int dst_cols, int dst_step
                                     )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    int src_x_off = (src_offset % src_step) >> 4;
    int src_y_off = src_offset / src_step;
    int dst_x_off = (dst_offset % dst_step) >> 4;
    int dst_y_off = dst_offset / dst_step;
    
    float4 sum = (float4)(0.0, 0.0, 0.0, 0.0);
    for (int i=x-anX; i < x-anX+ksX; i++) {
        for (int j=y-anY; j < y-anY+ksY; j++) {
#if defined BORDER_CONSTANT 
            int src_x = i < 0 ? 0 : i >= dst_cols ? 0 : i;
            int src_y = j < 0 ? 0 : j >= dst_rows ? 0 : j;
#elif defined BORDER_REPLICATE 
            int src_x = i < 0 ? 0 : i >= dst_cols ? dst_cols-1 : i;
            int src_y = j < 0 ? 0 : j >= dst_rows ? dst_rows-1 : j;
#elif defined BORDER_REFLECT
            int src_x = i < 0 ? -i-1 : i >= dst_cols ? -i-1 + (dst_cols << 1) : i;
            int src_y = j < 0 ? -j-1 : j >= dst_rows ? -j-1 + (dst_rows << 1) : j;
#elif defined BORDER_REFLECT_101
            int src_x = i < 0 ? -i : i >= dst_cols ? -i-2 + (dst_cols << 1) : i;
            int src_y = j < 0 ? -i : j >= dst_rows ? -j-2 + (dst_rows << 1) : j;
#endif
            sum += src[src_x + src_x_off + (src_y + src_y_off)*(src_step>>4)]/alpha;
        }
    }
    dst[x + dst_x_off + (y + dst_y_off)*(dst_step>>4)]=sum;
}

