/**
 *******************************************************************************
 *                       Continental Confidential
 *                  Copyright (c) Continental AG. 2016
 *
 *      This software is furnished under license and may be used or
 *      copied only in accordance with the terms of such license.
 *******************************************************************************
 * @file     ipu_cvt_test.cpp
 * @brief   The example implementation file which shows how to use IPU converting API
 *******************************************************************************
 */
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "ipu_cvt.h"


const char* USAGE = "Usage:\n"
                    "    ipu_cvt_test <options> <input-file> <output-file>\n"
                    "<options>:\n"
                    "    -i <width>,<height>,<format>,<crop>               Specify input width, height, pixel format, crop rect\n"
                    "    -o <width>,<height>,<format>,<rotation>,<crop>    Specify input width, height, pixel format, rotate mode, crop rect\n"
                    "    -p <loop-count>                                   Used for profiling, specify the loop count\n"
                    "<format>:\n"
                    "    I420   is  YUV420P\n"
                    "    RGBP   is  RGB565\n"
                    "    RGB3   is  RGB24\n"
                    "    BGR3   is  BGR24\n"
                    "    RGB4   is  RGR32\n"
                    "    BGR4   is  BGR32\n"
                    "    RGBA   is  RGRA32\n"
                    "    BGRA:  is  BGRA32\n"
                    "    GREY:  is  GREY8\n"
                    "<crop>: is a rect represented as x,y,w,h \n"
                    "<rotaion>:\n"
                    "    0      is  No rotation\n"
                    "    1      is  Vertical flip\n"
                    "    2      is  Horizontal flip\n"
                    "    3      is  Rotate 180 degrees\n"
                    "    4      is  Rotate 90 degrees to righ\n"
                    "    5      is  Rotate 90 degrees to righ, and vertical flip\n"
                    "    6      is  Rotate 90 degrees to righ, and horizontal flip\n"
                    "    7      is  Rotate 90 degrees to left\n"
                    "Example:\n"
                    "    ipu_cvt_test -i 1280,800,I420,0,0,640,400 -o 800,600,RGB3,0,0,0,400,300 input.yuv output.rgb\n";
                    

#define show_info(...)          fprintf(stdout, __VA_ARGS__)
#define show_error(...)         fprintf(stderr, __VA_ARGS__)
#define show_error_exit(...)    do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); } while (0)
#define show_usage_exit()       show_error_exit("%s", USAGE)


struct cvt_args {
    uint32_t    iw;
    uint32_t    ih;
    uint32_t    ifmt;
    ipu_rect    icrop;
    const char* ifile;
    
    uint32_t    ow;
    uint32_t    oh;
    uint32_t    ofmt;
    ipu_rect    ocrop;
    const char* ofile;
    uint32_t    ortt;
    
    uint32_t    loop;
};

unsigned long long now() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + tv.tv_usec/1000;
}


uint32_t pixel_bits(uint32_t pixfmt) {
    unsigned int bpp;
    switch (pixfmt) { 
        case IPU_PIX_FMT_RGB565:
        case IPU_PIX_FMT_YUYV:
        case IPU_PIX_FMT_UYVY:
        case IPU_PIX_FMT_YUV422P:
        case IPU_PIX_FMT_YVU422P:
            bpp = 16;
            break;
            
        case IPU_PIX_FMT_BGR24:
        case IPU_PIX_FMT_RGB24:
        case IPU_PIX_FMT_YUV444:
        case IPU_PIX_FMT_YUV444P:
            bpp = 24;
            break;
            
        case IPU_PIX_FMT_BGR32:
        case IPU_PIX_FMT_BGRA32:
        case IPU_PIX_FMT_RGB32:
        case IPU_PIX_FMT_RGBA32:
        case IPU_PIX_FMT_ABGR32:
            bpp = 32;
            break;
          
        case IPU_PIX_FMT_YUV420P:
        case IPU_PIX_FMT_YVU420P:
        case IPU_PIX_FMT_YUV420P2:
        case IPU_PIX_FMT_NV12:
        case IPU_PIX_FMT_TILED_NV12:
            bpp = 12;
            break;
            
        default:
            bpp = 8;
            break;
    }
    return bpp;
}


void parse_args(int argc, char** argv, cvt_args& cvtargs) {
    
    uint32_t    iw = 0;
    uint32_t    ih = 0;
    uint32_t    ifmt = 0;
    ipu_rect    icrop;
    char        ifourcc[5] = {0};
    const char* ifile = NULL;
    
    uint32_t    ow = 0;
    uint32_t    oh = 0;
    uint32_t    ofmt = 0;
    ipu_rect    ocrop;
    char        ofourcc[5] = {0};
    const char* ofile = NULL;
    uint32_t    ortt = 0;
    
    uint32_t    loop = 1;
    
    int opt = -1;
    int ret = -1;
    optind = 0;
    while ((opt = getopt(argc, argv, "i:o:p:")) != -1) {
        switch (opt) {
        case 'i':
            ret = sscanf(optarg, "%u,%u,%c%c%c%c,%u,%u,%u,%u", 
                &iw, &ih, &ifourcc[0], &ifourcc[1], &ifourcc[2], &ifourcc[3], &icrop.x, &icrop.y, &icrop.w, &icrop.h);
            if (ret != 10) {
                show_error("option '-i' has invalid value\n");
                show_usage_exit();    
            }
            break;
        case 'o':
            ret = sscanf(optarg, "%u,%u,%c%c%c%c,%u,%u,%u,%u,%u", 
                &ow, &oh, &ofourcc[0], &ofourcc[1], &ofourcc[2], &ofourcc[3], &ortt, &ocrop.x, &ocrop.y, &ocrop.w, &ocrop.h);
            if (ret != 11) {
                show_error("option '-o' has invalid value\n");
                show_usage_exit();    
            }
            break;
        case 'p':
            loop = atoi(optarg);
            if (!loop) {
                show_error("option '-p' has invalid value\n");
                show_usage_exit();                  
            }
            break;
        default:
            show_usage_exit();
        }
    } 

    if (optind + 1 >= argc) {
        show_usage_exit();
    }
    ifmt = fourcc(ifourcc[0], ifourcc[1], ifourcc[2], ifourcc[3]);
    ofmt = fourcc(ofourcc[0], ofourcc[1], ofourcc[2], ofourcc[3]); 
    ifile = argv[optind];
    ofile = argv[optind+1];

    show_info("Parsed args: -i %u,%u,%s,%u,%u,%u,%u  -o %u,%u,%s,%u,%u,%u,%u,%u  %s  %s\n", 
        iw, ih, ifourcc, icrop.x, icrop.y, icrop.w, icrop.h,
        ow, oh, ofourcc, ortt, ocrop.x, ocrop.y, ocrop.w, ocrop.h, 
        ifile, ofile);
    cvt_args args = {iw, ih, ifmt, icrop, ifile, ow, oh, ofmt, ocrop, ofile, ortt%8, loop};
    cvtargs = args;
}

void check_args(const cvt_args& cvt) {
    if (!cvt.iw || !cvt.ih || !cvt.ifmt || !cvt.ifile ||
        !cvt.ow || !cvt.oh || !cvt.ofmt || !cvt.ofile) {
        show_error_exit("Invalid args\n");    
    }

    int err = access(cvt.ifile, R_OK);
    if (err) {
        show_error_exit("Can't read input file\n");  
    }
}


int main(int argc, char** argv) {
    cvt_args cvt;
    parse_args(argc, argv, cvt);
    check_args(cvt);

    // Open input and output file
    FILE* infp = fopen(cvt.ifile, "rb");
    if (!infp) {
        show_error_exit("Can't open %s\n", cvt.ifile);
    }
    FILE* outfp = fopen(cvt.ofile, "wb");
    if (!outfp) {
        show_error_exit("Can't open %s\n", cvt.ofile);
    }

    // Open IPU device
    void* handle = ipu_open();
    if (!handle) {
        show_error_exit("ipu_open() failed: %s\n", ipu_error());
    }

    // Prepare input and output for IPU converting
    ipu_in in;
    in.buflen = cvt.iw * cvt.ih * pixel_bits(cvt.ifmt)/8;
    in.buf = ipu_alloc(handle, in.buflen);
    in.format = cvt.ifmt;
    in.width = cvt.iw;
    in.height = cvt.ih;
    in.crop = cvt.icrop;
    ipu_out out;
    out.buflen = cvt.ow * cvt.oh * pixel_bits(cvt.ofmt)/8;
    out.buf = ipu_alloc(handle, out.buflen);
    out.format = cvt.ofmt;
    out.width = cvt.ow;
    out.height = cvt.oh;
    out.crop = cvt.ocrop;
    out.rotate = cvt.ortt;
    if (!in.buf || !out.buf) {
        show_error_exit("ipu_alloc() failed for input and/or output buffer\n");     
    }

    
    // Fill input buffer by reading file
    if (in.buflen != fread(in.buf, 1, in.buflen, infp)) {
        show_error_exit("Can't read %u bytes from %s\n", in.buflen, cvt.ifile);    
    }


    // Loop Converting
    for (uint32_t i = 0; i < cvt.loop; i++) {

        // Converting start
        unsigned long long start = now();
        
        // Converting once
        if (-1 == ipu_convert(handle, &in, &out)) {
            show_error("ipu_convert() failed: %s\n", ipu_error());
            continue;
        }
        // Converting end
        unsigned long long end = now(); 
        show_info("%s => %s succeed: duration=%llums\n", cvt.ifile, cvt.ofile, end - start);
    }

    // Write output buffer to file
    if (out.buflen != fwrite(out.buf, 1, out.buflen, outfp)) {
        show_error_exit("Can't write %u bytes to %s\n", out.buflen, cvt.ofile);    
    }

    // Free IPU buffers
    if (-1 == ipu_free(handle, in.buf) || -1 == ipu_free(handle, out.buf)) {
        show_error_exit("ipu_free() failed for input and/or output buffer\n");
    }

    // Close IPU device
    if (-1 == ipu_close(handle)) {
        show_error_exit("ipu_close() failed: %s\n", ipu_error());
    }

    // Close input and output file
    fclose(infp);
    fclose(outfp);
    return 0;
}

