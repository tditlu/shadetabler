
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>

#include "image.h"
#include "path.h"
#include "lodepng/lodepng.h"
#include "exoquant/exoquant.h"
#include "cwalk/cwalk.h"

#define SHADETABLER_VERSION "1.0.4"
#define SHADETABLER_VERSION_DATE "2020-09-22"
#define SHADETABLER_HEADER "ShadeTabler (Shade Table Generator) by Todi / Tulou - version " SHADETABLER_VERSION " (" SHADETABLER_VERSION_DATE ")"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(num,min,max) (((num)<(min))?(min):(((max)<(num))?(max):(num)));

#define TYPE_LIGHT (0)
#define TYPE_DARK (1)
#define TYPE_BOTH (2)

static void usage() {
	printf(SHADETABLER_HEADER "\n");
	printf("Usage: shadetabler -o <output directory> <additional options> <input files...>\n");
	printf("\n");
	printf("Available options are:\n");
	printf(" -o, --output                    Output directory. (Required)\n");
	printf(" -v, --verbose                   Verbose output.\n");
	printf(" -f, --force                     Overwrite existing files, never prompt.\n");
	printf(" -c, --colors [2-256]            Number of colors saved in the output file. (Default 256)\n");
	printf(" -s, --shades [2-256]            Number of shades in shade table. (Default 64)\n");
	printf(" -t, --type [light|dark|both]    Type of shade table(s) to genereate. (Default \"light\")\n");
	printf(" -h, --hq                        High quality quantization quality.\n");
	printf(" -r, --reserve                   Reserve colors for black and white.\n");
	printf(" -p, --priority [0-...]          Priority of input file colors over generated shaded. (Default 0)\n");
	printf(" -l, --light                     Light shade table filename. (Default \"shadetable_light.png\")\n");
	printf(" -d, --dark                      Dark shade table filename. (Default \"shadetable_dark.png\")\n");
	printf("\n");
}

static int allocate_generate_and_save_shadetable(exq_data *exq, char *path, unsigned char *palette, unsigned int colors, unsigned int shades, int light, int verbose, int force);
static void exq_feed_extened(exq_data *pExq, unsigned char *pData, int nPixels, int shade, unsigned int forceBlackWhite);
static int palette_qsort_compare(const void *a, const void *b);
static void save_image(const char *filename, unsigned char *buffer, unsigned int width, unsigned int height, unsigned char *palette, unsigned int colors);

int main(int argc, char *argv[]) {
	const struct option longopts[] = {
		{"output", required_argument, 0, 'o' },
		{"verbose", no_argument, 0, 'v' },
		{"force", no_argument, 0, 'f' },
		{"colors", optional_argument, 0, 'c' },
		{"shades", optional_argument, 0, 's' },
 		{"type", optional_argument, 0, 't' },
		{"hq", no_argument, 0, 'h' },
		{"reserve", no_argument, 0, 'r' },
		{"priority", optional_argument, 0, 'p' },
		{"light", optional_argument, 0, 'l' },
		{"dark", optional_argument, 0, 'd' },
		{0, 0, 0, 0}
	};

	const char *optstring = "o:vfc:s:t:hrp:l:d:";

	if (argc <= 1) {
		usage();
		exit(EXIT_FAILURE);
	}

	char *output = NULL;
	unsigned int verbose = 0;
	unsigned int force = 0;
	unsigned int colors = 256;
	unsigned int shades = 64;
	unsigned int type = TYPE_LIGHT;
	unsigned int hq = 0;
	unsigned int reserve = 0;
	unsigned int priority = 0;
	char *light_output = "shadetable_light.png";
	char *dark_output = "shadetable_dark.png";

	int opt = 0;
	while ((opt = getopt_long(argc, argv, optstring, longopts, 0)) != EOF) {
		switch (opt) {
			case 'o':
				output = optarg;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'f':
				force = 1;
				break;
			case 'c':
				colors = atoi(optarg);
				break;
			case 's':
				shades = atoi(optarg);
				break;
			case 't':
				if (optarg[0] == 'd' || optarg[0] == 'D') {
					type = TYPE_DARK;
				} else if (optarg[0] == 'b' || optarg[0] == 'B') {
					type = TYPE_BOTH;
				}
				break;
			case 'h':
				hq = 1;
				break;
			case 'r':
				reserve = 1;
				break;
			case 'p':
				priority = atoi(optarg);
				break;
			case 'l':
				light_output = optarg;
				break;
			case 'd':
				dark_output = optarg;
				break;
			default:
				usage();
				exit(EXIT_FAILURE);
		}
	}

	if (output == NULL) {
		usage();
		fprintf(stderr, "No output directory specified.\n\n");
		exit(EXIT_FAILURE);
	}

	if ((argc - optind) == 0) {
		usage();
		fprintf(stderr, "No input file specified.\n\n");
		exit(EXIT_FAILURE);
	}

	if ((colors < 4 && reserve) || colors < 2 || colors > 256) {
		usage();
		fprintf(stderr, "Invalid number of colors%s.\n\n", reserve ? " (2 colors are reserved for black & white)." : "");
		exit(EXIT_FAILURE);
	}

	if (shades < 2 || shades > 256) {
		usage();
		fprintf(stderr, "Invalid number of shades.\n\n");
		exit(EXIT_FAILURE);
	}

	if (verbose) {
		printf(SHADETABLER_HEADER "\n");
	}

	exq_data *exq = exq_init();
	exq_no_transparency(exq);

	image_t *images = NULL;
	unsigned char *palette = NULL;
	char path[FILENAME_MAX];

	unsigned int number_of_images = (argc - optind);
	images = calloc(number_of_images, sizeof(image_t));
	if (images == NULL) {
		fprintf(stderr, "Unable to allocate memory for images.\n");
		goto error;
	}

	for (unsigned int i = 0; i < number_of_images; i++) {
		const char *infile = argv[optind + i];
		if (cwk_path_normalize(infile, path, sizeof(path)) == 0)  {
			fprintf(stderr, "Invalid input file path \"%s\".", infile);
			goto error;
		}

		const image_error_t image_error = image_load_rgba(&images[i], path);
		if (image_error) {
			fprintf(stderr, "Could not load input file \"%s\". %s.\n", path, image_error_text(image_error));
			goto error;
		}

		unsigned char *buffer = images[i].buffer;
		unsigned int width = images[i].width;
		unsigned int height = images[i].height;

		if (verbose) {
			printf("Generating priority colors for input file \"%s\".\n", path);
		}

		for (int j = 0; j < priority; j++) {
			exq_feed_extened(exq, buffer, width * height, 0, reserve);
		}

		if (type == TYPE_LIGHT || type == TYPE_BOTH) {
			if (verbose) {
				printf("Generating light shades for input file \"%s\".\n", path);
			}
			for (int j = 1; j < shades; j++) {
				int shade = (int)floor((((float)j) / ((float)(shades - 1)) * 255.0f) + 0.5);
				exq_feed_extened(exq, buffer, width * height, shade, reserve);
			}
		}

		if (type == TYPE_DARK || type == TYPE_BOTH) {
			if (verbose) {
				printf("Generating dark shades for input file \"%s\".\n", path);
			}
			for (int j = 0; j < (shades - 1); j++) {
				int shade = ((int)floor((((float)j) / ((float)(shades - 1)) * 255.0f) - 0.5)) - 255;
				exq_feed_extened(exq, buffer, width * height, shade, reserve);
			}
		}
	}

	if (verbose) {
		printf("Quantizing images%s.\n", hq ? ", high quality" : "");
	}

	exq_quantize_ex(exq, reserve ? colors - 2 : colors, hq);

	palette = malloc(colors * 4 * sizeof(unsigned char));
	if (palette == NULL) {
		fprintf(stderr, "Unable to allocate memory for palette.\n");
		goto error;
	}

	if (verbose) {
		printf("Generating & sorting palette.\n");
	}

	exq_get_palette(exq, palette, reserve ? colors - 2 : colors);

	if (reserve) {
		palette[((colors - 2) * 4) + 0] = 0; // Force black
		palette[((colors - 2) * 4) + 1] = 0;
		palette[((colors - 2) * 4) + 2] = 0;
		palette[((colors - 2) * 4) + 3] = 255;

		palette[((colors - 1) * 4) + 0] = 255; // Force white
		palette[((colors - 1) * 4) + 1] = 255;
		palette[((colors - 1) * 4) + 2] = 255;
		palette[((colors - 1) * 4) + 3] = 255;
	}

	qsort(palette, colors, sizeof(unsigned char) * 4, palette_qsort_compare); // Sort palette

	exq_set_palette(exq, palette, colors);

	for (unsigned int i = 0; i < number_of_images; i++) {
		const char *infile = argv[optind + i];
		path_get_output_filename(path, sizeof(path), output, infile);

		int write_image = 1;
		if (!force && path_file_exists(path)) {
			fprintf(stderr,"File '%s' already exists. Overwrite? [y/N] ", path);
			fflush(stderr);
			int c = getchar();
			write_image = (c == 'y' || c == 'Y') ? 1 : 0;
			while (c != '\n' && c != EOF) { c = getchar(); }
		}

		if (write_image) {
			unsigned int width = images[i].width;
			unsigned int height = images[i].height;
			unsigned char *buffer = malloc(width * height * sizeof(unsigned char));
			if (buffer == NULL) {
				fprintf(stderr, "Unable to allocate memory for image \"%s\".\n", path);
				goto error;
			}

			if (verbose) {
				printf("Saving image \"%s\"\n", path);
			}

			exq_map_image(exq, width * height, images[i].buffer, buffer);
			save_image(path, buffer, width, height, palette, colors);
			free(buffer);
		} else {
			if (verbose) {
				printf("Skipping image \"%s\"\n", path);
			}
		}
	}

	if (type == TYPE_LIGHT || type == TYPE_BOTH) {
		path_get_output_filename(path, sizeof(path), output, light_output);
		if (!allocate_generate_and_save_shadetable(exq, path, palette, colors, shades, 1, verbose, force)) {
			goto error;
		}
	}

	if (type == TYPE_DARK || type == TYPE_BOTH) {
		path_get_output_filename(path, sizeof(path), output, dark_output);
		if (!allocate_generate_and_save_shadetable(exq, path, palette, colors, shades, 0, verbose, force)) {
			goto error;
		}
	}

error:
	if (palette != NULL) {
		free(palette);
	}

	if (images != NULL) {
		for (unsigned int i = 0; i < number_of_images; i++) {
			image_free_rgba(&images[i]);
		}
		free(images);
	}

	exq_free(exq);
}

static int palette_qsort_compare(const void *a, const void *b) {
	unsigned char ra = ((unsigned char *)a)[0];
	unsigned char ga = ((unsigned char *)a)[1];
	unsigned char ba = ((unsigned char *)a)[2];

	unsigned char rb = ((unsigned char *)b)[0];
	unsigned char gb = ((unsigned char *)b)[1];
	unsigned char bb = ((unsigned char *)b)[2];

	float af = (0.2126f * ((float)ra)) + (0.7152f * ((float)ga)) + (0.0722 * ((float)ba));
	float bf = (0.2126f * ((float)rb)) + (0.7152f * ((float)gb)) + (0.0722 * ((float)bb));

	if (af > bf) {
		return 1;
	} else if (bf > af) {
		return -1;
	} else {
		return 0;
	}
}

static unsigned char *allocate_and_generate_shadetable(unsigned char *palette, unsigned int colors, unsigned int shades, int light) {
	unsigned char *buffer = malloc(colors * shades * 4 * sizeof(unsigned char));
	if (buffer != NULL) {
		for (unsigned int y = 0; y < shades; y++) {
			int shade = (int)floor((((float)y) / ((float)(shades - 1)) * 255.0f) + 0.5);
			if (!light) { shade = shade - 255; }
			for (unsigned int x = 0; x < colors; x++) {
				buffer[((y * colors * 4) + (x * 4)) + 0] = (unsigned char)CLAMP(((int)palette[(x * 4) + 0]) + shade, 0, 255);
				buffer[((y * colors * 4) + (x * 4)) + 1] = (unsigned char)CLAMP(((int)palette[(x * 4) + 1]) + shade, 0, 255);
				buffer[((y * colors * 4) + (x * 4)) + 2] = (unsigned char)CLAMP(((int)palette[(x * 4) + 2]) + shade, 0, 255);
				buffer[((y * colors * 4) + (x * 4)) + 3] = 255;
			}
		}
	}
	return buffer;
}

static int allocate_generate_and_save_shadetable(exq_data *exq, char *path, unsigned char *palette, unsigned int colors, unsigned int shades, int light, int verbose, int force) {
	int write_shadetable = 1;
	if (!force && path_file_exists(path)) {
		fprintf(stderr,"File '%s' already exists. Overwrite? [y/N] ", path);
		fflush(stderr);
		int c = getchar();
		write_shadetable = (c == 'y' || c == 'Y') ? 1 : 0;
		while (c != '\n' && c != EOF) { c = getchar(); }
	}

	if (write_shadetable) {
		unsigned char *buffer_rgba = allocate_and_generate_shadetable(palette, colors, shades, light);
		if (buffer_rgba != NULL) {
			unsigned char *buffer_8bit = malloc(colors * shades * sizeof(unsigned char));
			if (buffer_8bit != NULL) {
				exq_map_image(exq, colors * shades, buffer_rgba, buffer_8bit);

				if (verbose) {
					printf("Saving image \"%s\"\n", path);
				}

				save_image(path, buffer_8bit, colors, shades, palette, colors);
				free(buffer_8bit);
			} else {
				free(buffer_rgba);
				fprintf(stderr, "Unable to allocate memory for shadetable.\n");
				return 0;
			}
			free(buffer_rgba);
		} else {
			fprintf(stderr, "Unable to allocate memory for shadetable.\n");
			return 0;
		}
	} else {
		if (verbose) {
			printf("Skipping image \"%s\"\n", path);
		}
	}
	return 1;
}

static void save_image(const char *filename, unsigned char *buffer, unsigned int width, unsigned int height, unsigned char *palette, unsigned int colors) {
	LodePNGState state;
	lodepng_state_init(&state);
	state.info_raw.colortype = LCT_PALETTE;
	state.info_raw.bitdepth = 8;
	state.info_png.color.colortype = LCT_PALETTE;
	state.info_png.color.bitdepth = 8;

	state.encoder.filter_palette_zero = 0;
	state.encoder.filter_strategy = LFS_ZERO;
	state.encoder.auto_convert = 0;
	state.encoder.force_palette = 1;

	unsigned char *p = palette;
	for (unsigned int i = 0; i < colors; i++) {
		lodepng_palette_add(&state.info_png.color, p[0], p[1], p[2], p[3]);
		lodepng_palette_add(&state.info_raw, p[0], p[1], p[2], p[3]);
		p += 4;
	}

	unsigned char *output_file_data;
	size_t output_file_size;
	unsigned int status = lodepng_encode(&output_file_data, &output_file_size, buffer, width, height, &state);
	if (status) {
		fprintf(stderr, "Can't encode image: %s\n", lodepng_error_text(status));
		goto error;
	}

	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "Unable to write to %s\n", filename);
		goto error;
	}
	fwrite(output_file_data, 1, output_file_size, fp);
	fclose(fp);

error:
	lodepng_state_cleanup(&state);
}

#define EXQ_SCALE_R 1.0f
#define EXQ_SCALE_G 1.2f
#define EXQ_SCALE_B 0.8f
#define EXQ_SCALE_A 1.0f

static unsigned int exq_make_hash(unsigned int rgba)
{
	rgba -= (rgba >> 13) | (rgba << 19);
	rgba -= (rgba >> 13) | (rgba << 19);
	rgba -= (rgba >> 13) | (rgba << 19);
	rgba -= (rgba >> 13) | (rgba << 19);
	rgba -= (rgba >> 13) | (rgba << 19);
	rgba &= EXQ_HASH_SIZE - 1;
	return rgba;
}

static void exq_feed_extened(exq_data *pExq, unsigned char *pData, int nPixels, int shade, unsigned int forceBlackWhite)
{
	int i;
	unsigned int hash;
	unsigned char r, g, b, a;
	exq_histogram *pCur;
	unsigned char channelMask = 0xff00 >> pExq->numBitsPerChannel;

	for(i = 0; i < nPixels; i++)
	{
		r = *pData++; g = *pData++; b = *pData++; a = *pData++;

		r = (unsigned char)CLAMP(((int)r) + shade, 0, 255);
		g = (unsigned char)CLAMP(((int)g) + shade, 0, 255);
		b = (unsigned char)CLAMP(((int)b) + shade, 0, 255);

		if(forceBlackWhite && !pExq->transparency)
		{
			if (r == 0 && g == 0 && b == 0) { continue; }
			if (r == 255 && g == 255 && b == 255) { continue; }
		}

		hash = exq_make_hash(((unsigned int)r) | (((unsigned int)g) << 8) | (((unsigned int)b) << 16) | (((unsigned int)a) << 24));

		pCur = pExq->pHash[hash];
		while(pCur != NULL && (pCur->ored != r || pCur->ogreen != g ||
			pCur->oblue != b || pCur->oalpha != a))
			pCur = pCur->pNextInHash;

		if(pCur != NULL)
			pCur->num++;
		else
		{
			pCur = (exq_histogram*)malloc(sizeof(exq_histogram));
			pCur->pNextInHash = pExq->pHash[hash];
			pExq->pHash[hash] = pCur;
			pCur->ored = r; pCur->ogreen = g; pCur->oblue = b; pCur->oalpha = a;
			r &= channelMask; g &= channelMask; b &= channelMask;

			pCur->color.r = r / 255.0f * EXQ_SCALE_R;
			pCur->color.g = g / 255.0f * EXQ_SCALE_G;
			pCur->color.b = b / 255.0f * EXQ_SCALE_B;
			pCur->color.a = a / 255.0f * EXQ_SCALE_A;

			if(pExq->transparency)
			{
				pCur->color.r *= pCur->color.a;
				pCur->color.g *= pCur->color.a;
				pCur->color.b *= pCur->color.a;
			}

			pCur->num = 1;
			pCur->palIndex = -1;
			pCur->ditherScale.r = pCur->ditherScale.g = pCur->ditherScale.b =
				pCur->ditherScale.a = -1;
			pCur->ditherIndex[0] = pCur->ditherIndex[1] = pCur->ditherIndex[2] =
				pCur->ditherIndex[3] = -1;
		}
	}
}
