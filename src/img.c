#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "./img.h"

void clear_surface(cairo_surface_t *surface, cairo_t *cr, unsigned char r, unsigned char g, unsigned char b) {
	cairo_set_source_rgba(cr, (double)r / 255.0, (double)g / 255.0, (double)b / 255.0, 1.0);
	cairo_paint(cr);
	cairo_surface_flush(surface);
}

bool readfile(void (*die)(const char*, const char*, int), char *file, bool *is_stdin, char **filename, int blocksize, ILubyte **image_data, cairo_surface_t **surface, cairo_t **cr, ILuint *image, int *image_w, int *image_h) {
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
		if (!fp) { die(file, strerror(errno), 1); }
		char *slash = strrchr(file, '/'); // gets the basename of file path
		*filename = slash ? slash + 1 : file;
	}

	// reads a FILE* and outputs the data
	void *data = NULL;
	size_t size = 0;
	size_t read = 0;
	while (!feof(fp) && !ferror(fp)) {
		data = realloc(data, size + blocksize);
		if (!data) die("Failed realloc", NULL, 1);
		read = fread(data + size, 1, blocksize, fp);
		if (read <= 0) break;
		size += read;
	}

	bool ferr = ferror(fp);
	if (f) fclose(fp); // close the file
	if (ferr) {
		free(data);
		die("Failed to read file", NULL, 1);
		return false;
	}
	
	// read image
	ILuint image_;
	ilGenImages(1, &image_);
	ilBindImage(image_);
	bool ret = ilLoadL(IL_TYPE_UNKNOWN, data, size);
	free(data);
	if (!ret) die("Failed to read image", NULL, 1);

	ILint image_w_ = ilGetInteger(IL_IMAGE_WIDTH);
	ILint image_h_ = ilGetInteger(IL_IMAGE_HEIGHT);
	*image_w = image_w_;
	*image_h = image_h_;

	ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);

	ILubyte *image_data_ = ilGetData();
	if (!image_data_) die("Failed to load image data", NULL, 1);

	*image_data = image_data_;

	cairo_surface_t *image_surface_ = cairo_image_surface_create_for_data(
			image_data_, CAIRO_FORMAT_ARGB32, image_w_, image_h_,
			cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, image_w_));
	if (!image_surface_) die("Failed to create cairo surface", NULL, 1);

	cairo_t *cr_ = cairo_create(image_surface_);
	if (!cr_) die("Failed to create cairo context", NULL, 1);

	*surface = image_surface_;
	*cr = cr_;
	*image = image_;
	
	return ret;
}
