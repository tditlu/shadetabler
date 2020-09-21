#ifndef _IMAGE_H
#define _IMAGE_H

typedef struct {
	unsigned char *buffer;
	unsigned int width;
	unsigned int height;
} image_t;

typedef enum {
	IMAGE_ERROR_NO_ERROR = 0,
	IMAGE_ERROR_OPEN = 1
} image_error_t;

image_error_t image_load_rgba(image_t *const image, const char *infile);
void image_free_rgba(image_t *const image);
const char *image_error_text(image_error_t error);

#endif /* _IMAGE_H */
