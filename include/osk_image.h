#pragma once
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#define FADE_FRAMES 20

typedef struct {
	uint8_t width;
	uint8_t height;
	uint8_t *data;
	//uint8_t channels;
} Image;

typedef struct {
	Image **images;
	uint8_t count;
} ImageList;

typedef struct {
    double H;
    double S;
    double L;
} HSL;

typedef struct {
    double H;
    double S;
    double V;
} HSV;

typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} RGB;

typedef struct {
	const char * imagePath;

} OskFade;

Image* CreateImageFromFile(const char* filename);
void DestroyImage(Image* img);
//Image* BlendImages(Image *imgA, Image *imgB, uint8_t step);
//ImageList *CreateFadeSequence(Image *img);
ImageList *CreateFadeSequence(Image *img, double *fadeTable);
void DestroyImageList(ImageList *imageList);
