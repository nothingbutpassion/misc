#include <stdio.h>
#include <turbojpeg.h>

static int compressJPEGFile(const char* infile, int width, int height, int subsample, const char* outfile) {
    FILE *fIn, *fOut;
    int inSize, outSize;
    void *inBuf, *outBuf;
    tjhandle handle;
    
    if (!(fIn = fopen(infile, "rb"))) {
    	fprintf(stderr, "can't open %s\n",  infile);
        return -1; 
    }
    if (!(fOut = fopen(outfile, "wb"))) {
    	fprintf(stderr, "can't open %s\n",  outfile);
        fclose(fIn);
        return -1; 
    }
    
    // prepare input
    fseek(fIn, 0L, SEEK_END);
    inSize = ftell(fIn);
    fseek(fIn, 0L, SEEK_SET);
    inBuf = malloc(inSize);
    fread(inBuf, inSize, 1, fIn);
    
    // prepare a big output buffer
    outSize = width * height * 4;
    outBuf = malloc(outSize); 
    
    // compress
    handle = tjInitCompress();
    if (!tjCompressFromYUV(handle, inBuf, width, 1, height, subsample, &outBuf, &outSize, 100, 0)) {
        fwrite(outBuf, outSize, 1, fOut);
    } else {
    	fprintf(stderr, "tjCompressFromYUV() error: %s\n",  tjGetErrorStr());
    }
    tjDestroy(handle);
    
    // clean
    free(inBuf);
    free(outBuf);
    fclose(fIn);
    fclose(fOut);
    return 0;
}

static int decompressJPEGFile(const char* infile, int width, int height, const char* outfile) {
    FILE *fIn, *fOut;
    int inSize, outSize;
    void *inBuf, *outBuf;
    tjhandle handle;
    
    if (!(fIn = fopen(infile, "rb"))) {
    	fprintf(stderr, "can't open %s\n",  infile);
        return -1; 
    }
    if (!(fOut = fopen(outfile, "wb"))) {
    	fprintf(stderr, "can't open %s\n",  outfile);
        fclose(fIn);
        return -1; 
    }
    
    // prepare input
    fseek(fIn, 0L, SEEK_END);
    inSize = ftell(fIn);
    fseek(fIn, 0L, SEEK_SET);
    inBuf = malloc(inSize);
    fread(inBuf, inSize, 1, fIn);
    
    // prepare output 
    outSize = width*height*3/2;
    outBuf =  malloc(outSize);
    
    // decompress
    handle = tjInitDecompress();
    if (!tjDecompressToYUV2(handle, inBuf, inSize, outBuf, width, 1, height, 0)) {
        fwrite(outBuf, outSize, 1, fOut);
    } else {
    	fprintf(stderr, "tjDecompressToYUV2() error: %s\n",  tjGetErrorStr());
    }
    tjDestroy(handle);
    
    // clean
    free(inBuf);
    free(outBuf);
    fclose(fIn);
    fclose(fOut);
    return 0;
}


static void showUsage(const char* appName) { 
	fprintf(stderr, "Usages: \n"
		"  %s d <jpeg-file> <width> <height> <yuv-file>\n"
		"  %s c <yuv-file> <width> <height> <subsample> <jpeg-file>\n"
		"Notes:\n"
		"  <bpp>: bytes per pixel\n"
		"  <subsample>: %d(TJSAMP_444), %d(TJSAMP_420) %d(TJSAMP_GRAY)\n",
		appName, appName, TJSAMP_444, TJSAMP_420, TJSAMP_GRAY);
	exit(-1);
}


int main(int argc, char* argv[]) {
    if (argc == 7 && argv[1][0] == 'c') {
        compressJPEGFile(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), argv[6]);
    } else if (argc == 6 && argv[1][0] == 'd') {
        decompressJPEGFile(argv[2], atoi(argv[3]), atoi(argv[4]), argv[5]);
    } else {
    	showUsage(argv[0]);
    }
	return 0;
}
