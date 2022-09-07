#ifndef JPEG_WRAPPER_H
#define JPEG_WRAPPER_H

typedef struct {
	int width;      // image width
	int height;     // image height 
	int bpp;        // bytes per pixel
	int format;     // same as libjpeg color space: 0(UNKNOWN), 1(GRAYSCALE), 2(RGB), 3(YCbCr), ...
	char* data;     // raw/compressed data
    int size;       // data size
} ImageData;


#ifdef __cplusplus
extern "C" {
#endif

int decompressJPEG(const ImageData* inImage, ImageData* outImage);
int compressJPEG(const ImageData* inImage, ImageData* outImage);

#ifdef __cplusplus
}
#endif

#endif // JPEG_WRAPPER_H
