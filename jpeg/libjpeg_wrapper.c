#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>
#include "libjpeg_wrapper.h"

typedef struct {
    struct jpeg_error_mgr base;
    jmp_buf jmpBuf;
} ErrorMgr;


static void errorCallback(j_common_ptr cinfo) {
    ErrorMgr* myerr = (ErrorMgr*)cinfo->err;
    longjmp(myerr->jmpBuf, 1);
}


static void dumpDecompress(struct jpeg_decompress_struct* cinfo) {
    fprintf(stderr, "jpeg_decompress_struct:\n"
        "image_width=%d, image_height=%d, num_components=%d, jpeg_color_space=%d\n"
        "output_width=%d, output_height=%d, output_components=%d, out_color_space=%d\n",
        cinfo->image_width, cinfo->image_height, cinfo->num_components, cinfo->jpeg_color_space,
        cinfo->output_width, cinfo->output_height, cinfo->output_components, cinfo->out_color_space);
}

int decompressJPEG(const ImageData* inImage, ImageData* outImage) {
    struct jpeg_decompress_struct cinfo;
	ErrorMgr errorMgr;
	JSAMPROW ptr;
    
    // set error callback
	cinfo.err = jpeg_std_error(&errorMgr.base);
	errorMgr.base.error_exit = errorCallback;
	if (setjmp(errorMgr.jmpBuf)) {
		errorMgr.base.output_message(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return -1;
	}
    
    // create decompress object and set input bufer
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, inImage->data, inImage->size);
    
    // read jpeg header and start decompress
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
    
    // loop read the decompressed data to output buffer
	ptr = outImage->data;
	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, &ptr, 1);
		ptr += cinfo.output_width * cinfo.output_components;
	}
    
    // get the output image info
    outImage->width = cinfo.output_width;
	outImage->height = cinfo.output_height;
	outImage->bpp = cinfo.output_components;
	outImage->format = cinfo.out_color_space;
    outImage->size = cinfo.output_width * cinfo.output_height * cinfo.output_components;

    // finish decompress and destroy decompress object
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return 0;
}


static void dumpCompress(struct jpeg_compress_struct* cinfo) {
    fprintf(stderr, "jpeg_compress_struct:\n"
        "image_width=%d, image_height=%d, input_components=%d, in_color_space=%d\n"
        "free_in_buffer=%d, data_precision=%d, num_components=%d, jpeg_color_space=%d\n",
        cinfo->image_width, cinfo->image_height, cinfo->input_components, cinfo->in_color_space,
        cinfo->dest->free_in_buffer, cinfo->data_precision, cinfo->num_components, cinfo->jpeg_color_space);
}

int compressJPEG(const ImageData* inImage, ImageData* outImage) {

    struct jpeg_compress_struct cinfo;
    ErrorMgr errorMgr;
	JSAMPROW ptr;
    
	// set error callback
	cinfo.err = jpeg_std_error(&errorMgr.base);
	errorMgr.base.error_exit = errorCallback;
	if (setjmp(errorMgr.jmpBuf)) {
		errorMgr.base.output_message(&cinfo);
		jpeg_destroy_compress(&cinfo);
		return -1;
	}
    
    // create compress object and set output buffer
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &outImage->data, &outImage->size);
    
    // set parameters for compression
    cinfo.image_width = inImage->width;
    cinfo.image_height = inImage->height;
    cinfo.input_components = inImage->bpp;
    cinfo.in_color_space = inImage->format;
    jpeg_set_defaults(&cinfo);
    // after jpeg_set_defaults(), we can call jpeg_set_quality(&cinfo, quality, TRUE);

    // start compressor
    jpeg_start_compress(&cinfo, TRUE);
    
    // loop feed input buffer for compressing
    ptr = inImage->data;
    while (cinfo.next_scanline < cinfo.image_height) {
        jpeg_write_scanlines(&cinfo, &ptr, 1);
        ptr += cinfo.image_width * cinfo.input_components;
    }
    
    // set the output image info
    outImage->width = cinfo.image_width;
    outImage->height = cinfo.image_height;
    outImage->bpp = cinfo.input_components;
    outImage->format = cinfo.in_color_space;
    
    // finish compression and destory compression object
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
	return 0;
}


