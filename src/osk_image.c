#include <osk_image.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNM

#include "stb_image.h"
#include "rgbtools.h"

Image* CreateImageFromFile(const char* filename) {
	int x, y, n;
	uint8_t *data = stbi_load(filename, &x, &y, &n, 3);

	Image* img = malloc(sizeof(Image));
	img->data = data;
	img->width = x;
	img->height = y;

	return img;
}

void DestroyImage(Image* img) {
	stbi_image_free(img->data);
	free(img);
}

void DestroyImageList(ImageList *imageList) {
	for (int i = 0; i < imageList->count; i++) {
		DestroyImage(imageList->images[i]);
	}
	free(imageList);
}

/*
OskFade* CreateOskFade() {
	OskFade *f = malloc(sizeof(OskFade));
	return f;
}

void DestroyOskFade() {

}
*/
void FadeImage(uint8_t *in, uint8_t *out, double v) {
	for (int i = 0; i < 128*160; i++) {
		RgbColor rgb;
		HsvColor hsv;

		rgb.r = in[(i*3)+0];
		rgb.g = in[(i*3)+1];
		rgb.b = in[(i*3)+2];
		hsv = RgbToHsv(rgb);

		if (hsv.v > v)
			hsv.v -= v;
		else
			hsv.v = 0;

		rgb = HsvToRgb(hsv);

		out[(i*3)+0] = rgb.r;
		out[(i*3)+1] = rgb.g;
		out[(i*3)+2] = rgb.b;
	}
}

ImageList *CreateFadeSequence(Image *img, double *fadeTable) {
	ImageList *imageList;
	imageList = malloc(sizeof(ImageList));
	imageList->images = malloc(sizeof(Image) * FADE_FRAMES);

	uint8_t *frameData = malloc(160*128*3*FADE_FRAMES);
	Image *frameImages = malloc(sizeof(Image)*FADE_FRAMES);

	for (int i = 0; i < FADE_FRAMES; i++) {

		Image *newImg = &frameImages[i];
		newImg->width = 160;
		newImg->height = 128;
		uint8_t *data = &frameData[160*128*3*i];
		newImg->data = data;
		FadeImage(img->data, data, fadeTable[i]);
		imageList->images[i] = newImg;
		imageList->count = FADE_FRAMES;
	}

	return imageList;
}
