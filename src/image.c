#include <stdlib.h>
#include "lodepng/lodepng.h"
#include "image.h"

void image_free_rgba(image_t *const image) {
	if (image->buffer != NULL) {
		free(image->buffer);
		image->buffer = NULL;
	}
}

image_error_t image_load_rgba(image_t *const image, const char *infile) {
	image->buffer = NULL;

	image_error_t error;

	unsigned int status = lodepng_decode32_file(&image->buffer, &image->width, &image->height, infile);
	if (status || image->buffer == NULL) {
		error = IMAGE_ERROR_OPEN;
		goto error;
	}

	for (unsigned int y = 0; y < image->height; y++) {
		for (unsigned int x = 0; x < image->width; x++) {
			if (image->buffer[((y * image->width *4 ) + (x * 4)) + 3] != 255) {
				float a = ((float)image->buffer[((y * image->width * 4) + (x * 4)) + 3]) / 255.0f;
				image->buffer[((y * image->width * 4) + (x * 4)) + 0] = (unsigned char)(((float)image->buffer[((y * image->width * 4) + (x * 4)) + 0]) * a);
				image->buffer[((y * image->width * 4) + (x * 4)) + 1] = (unsigned char)(((float)image->buffer[((y * image->width * 4) + (x * 4)) + 1]) * a);
				image->buffer[((y * image->width * 4) + (x * 4)) + 2] = (unsigned char)(((float)image->buffer[((y * image->width * 4) + (x * 4)) + 2]) * a);
				image->buffer[((y * image->width * 4) + (x * 4)) + 3] = 255;
			}
		}
	}

	return IMAGE_ERROR_NO_ERROR;

error:
	if (image->buffer != NULL) {
		image_free_rgba(image);
	}
	return error;
}

const char *image_error_text(image_error_t error) {
	switch(error) {
		case IMAGE_ERROR_NO_ERROR: return "No error";
		case IMAGE_ERROR_OPEN: return "Unable to open input file";
	}

	return "Unknown error";
}
