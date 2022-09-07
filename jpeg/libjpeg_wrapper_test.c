#include <stdio.h>
#include <jpeglib.h>
#include "libjpeg_wrapper.h"

static int decompressJPEGFile(const char* infile, const char* outfile) {
    FILE* fIn;
    FILE* fOut;
    ImageData inImage;
    ImageData outImage;
    
    if (!(fIn = fopen(infile, "rb"))) {
        return -1; 
    }
    if (!(fOut = fopen(outfile, "wb"))) {
        fclose(fIn);
        return -1; 
    }
    
    fseek(fIn, 0L, SEEK_END);
    inImage.size = ftell(fIn);
    fseek(fIn, 0L, SEEK_SET);
    inImage.data = malloc(inImage.size);
    fread(inImage.data, inImage.size, 1, fIn);
    
    outImage.data =  malloc(1280*720*3);
    if (!decompressJPEG(&inImage, &outImage)) {
        fwrite(outImage.data, outImage.size, 1, fOut);
    }
    
    free(inImage.data);
    free(outImage.data);
    fclose(fIn);
    fclose(fOut);
    return 0;
}

static int compressJPEGFile(const char* infile, int width, int height, int bpp, int format, const char* outfile) {
    FILE* fIn;
    FILE* fOut;
    ImageData inImage;
    ImageData outImage;
    
    if (!(fIn = fopen(infile, "rb"))) {
        return -1; 
    }
    if (!(fOut = fopen(outfile, "wb"))) {
        fclose(fIn);
        return -1; 
    }
    
    fseek(fIn, 0L, SEEK_END);
    inImage.size = ftell(fIn);
    fseek(fIn, 0L, SEEK_SET);
    inImage.data = malloc(inImage.size);
    fread(inImage.data, inImage.size, 1, fIn);

    inImage.width = width;
    inImage.height = height;
    inImage.bpp = bpp;
    inImage.format = format;
    
    outImage.size = inImage.width * inImage.height * inImage.bpp;
    outImage.data = malloc(outImage.size);
    if (!compressJPEG(&inImage, &outImage)) {
        fwrite(outImage.data, outImage.size, 1, fOut);
    }
    
    free(inImage.data);
    free(outImage.data);
    fclose(fIn);
    fclose(fOut);
    return 0;
}

static void showUsage(const char* appName)
{ 
	fprintf(stderr, "Usages: \n"
		"  %s d <jpeg-file> <raw-file>\n"
		"  %s c <raw-file> <width> <height> <bpp> <color-space> <jpeg-file>\n"
		"Notes:\n"
		"  <bpp>: bytes per pixel\n"
		"  <color-space>: %d(UNKNOWN), %d(GRAYSCALE), %d(RGB), %d(YCbCr)\n",
		appName, appName, JCS_UNKNOWN, JCS_GRAYSCALE, JCS_RGB, JCS_YCbCr);
	exit(-1);
}

int main(int argc, char* argv[]) {
    if (argc == 8 && argv[1][0] == 'c') {
        compressJPEGFile(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), argv[7]);
    } else if (argc == 4 && argv[1][0] == 'd') {
        decompressJPEGFile(argv[2], argv[3]);
    } else {
    	showUsage(argv[0]);
    }
	return 0;
}

