#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "./img.h"

void clear_surface(cairo_surface_t *surface, cairo_t *cr, unsigned char r, unsigned char g, unsigned char b) {
	cairo_set_source_rgba(cr, (double)r / (double)UCHAR_MAX, (double)g / (double)UCHAR_MAX, (double)b / (double)UCHAR_MAX, 1.0);
	cairo_paint(cr);
	cairo_surface_flush(surface);
}

bool readfilesurface(void (*die)(const char*, const char*, int), char *file, bool *is_stdin, char **filename, size_t blocksize, ILubyte **image_data, cairo_surface_t **surface, cairo_t **cr, ILuint *image, struct vector2 *image_size, ILint *image_bpp, bool apply_transparency, struct color apply_color) {
	if (!readfile(die, file, is_stdin, filename, blocksize, image_data, image, image_size, image_bpp, IL_BGRA, IL_UNSIGNED_BYTE, apply_transparency, apply_color)) return false;

	cairo_surface_t *image_surface_ = cairo_image_surface_create_for_data(
			*image_data, CAIRO_FORMAT_ARGB32, image_size->x, image_size->y,
			cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, image_size->x));
	if (!image_surface_) { die("Failed to create cairo surface", NULL, 1); return false; }

	cairo_t *cr_ = cairo_create(image_surface_);
	if (!cr_) { die("Failed to create cairo context", NULL, 1); return false; }

	*surface = image_surface_;
	*cr = cr_;
	return true;
}

bool readfile(void (*die)(const char*, const char*, int), char *file, bool *is_stdin, char **filename, size_t blocksize, ILubyte **image_data, ILuint *image, struct vector2 *image_size, ILint *image_bpp, ILenum format, ILenum type, bool apply_transparency, struct color apply_color) {
	// gets a FILE* from the file name
	FILE *fp;
	bool f = false;
	if (file[0] == '-' && file[1] == '\0') {
		*is_stdin = true;
		fp = stdin;
		*filename = "stdin";
	} else {
		*is_stdin = false;
		f = true;
		fp = fopen(file, "rb"); // rb = read + binary
		if (!fp) { die(file, strerror(errno), 1); return false; }
		char *slash = strrchr(file, '/'); // gets the basename of file path
		*filename = slash ? slash + 1 : file;
	}

	// reads a FILE* and outputs the data
	void *data = NULL;
	size_t size = 0;
	size_t read = 0;
	while (!feof(fp) && !ferror(fp)) {
		data = realloc(data, size + blocksize);
		if (!data) { die("Failed realloc", NULL, 1); return false; }
		read = fread(data + size, 1, blocksize, fp);
		if (read <= 0) break;
		size += read;
	}

	bool ferr = ferror(fp);
	if (f) fclose(fp); // close the file
	if (ferr) {
		free(data);
		die(file, "Failed to read file", 1);
		return false;
	}
	
	// read image
	ILuint image_ = 0;
	ilGenImages(1, &image_);
	if (!image_) { die(file, "Failed to create image", 1); return false; }
	ilBindImage(image_);
	bool ret = ilLoadL(IL_TYPE_UNKNOWN, data, size);
	free(data);
 	if (!ret) { die(file, "Failed to read image, file is not recognized by DevIL", 1); return false; }

	struct vector2 image_size_ = { ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT) };
	*image_size = image_size_;

	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	ILubyte *image_data_ = ilGetData();
	if (!image_data_) { die(file, "Failed to get image data", 1); return false; }

	ILint image_bpp_ = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	*image_bpp = image_bpp_;

	if (apply_transparency) {
		for (ILint y = 0; y < image_size_.x; ++y) {
			for (ILint x = 0; x < image_size_.y; ++x) {
				ILubyte *pix = &image_data_[(y * image_size_.y + x) * image_bpp_];
				if (pix[3] != UCHAR_MAX) {
					pix[0] = lerpc(apply_color.r, pix[0], pix[3]);
					pix[1] = lerpc(apply_color.g, pix[1], pix[3]);
					pix[2] = lerpc(apply_color.b, pix[2], pix[3]);
				}
			}
		}
	}

	ilConvertImage(format, type);
 
	image_data_ = ilGetData();
	if (!image_data_) { die(file, "Failed to get image data", 1); return false; }

	*image_data = image_data_;
	return true;
}
