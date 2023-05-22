#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <magic.h>

#include "./main.h"
#include "./img.h"
#include "./cover.h"

bool strstarts(const char *str, const char *substr) {
	size_t substrlen = strlen(substr);
	if (strlen(str) < substrlen) return false;
	return strncmp(str, substr, substrlen) == 0;
}

bool readimage(char *file, bool *is_stdin, char **filename, size_t blocksize, ILubyte **image_data, ILuint *image, struct vector2 *image_size, ILint *image_bpp, bool apply_transparency, struct color apply_color) {
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
		if (!fp) {
			die(1, file, strerror(errno), NULL);
		}
		char *slash = strrchr(file, '/'); // gets the basename of file path
		*filename = slash ? slash + 1 : file;
	}

	// reads a FILE* and outputs the data
	void *data = NULL;
	size_t read, size = 0;
	while (!feof(fp) && !ferror(fp)) {
		// read the file in blocks of blocksize bytes
		data = realloc(data, size + blocksize);
		if (!data) die(1, "Failed realloc", NULL);
		read = fread(data + size, 1, blocksize, fp);
		if (read <= 0) break;
		size += read;
	}

	bool ferr = ferror(fp);
	if (f) fclose(fp); // close the file
	if (ferr) {
		free(data);
		die(1, file, "Failed to read file", NULL);
	}

	if (size == 0) {
		free(data);
		die(1, file, "Empty file", NULL);
	}

	// detect file type
	magic_t magic = magic_open(MAGIC_MIME_TYPE);
	if (magic_load(magic, NULL) != 0) {
		free(data);
		die(1, "Failed to load magic", NULL);
	}

	// get mime type
	const char *file_type = magic_buffer(magic, data, size);
	if (!file_type) {
		magic_close(magic);
		free(data);
		die(1, file, "Failed to determine file type", NULL);
	}

	if (strstarts(file_type, "image/")) {
		// file is image, we don't need to do anything
	} else if (strstarts(file_type, "audio/")) {
		// TODO
		void *new_data;
		if (!get_cover_image(data, size, &new_data, &size)) {
			magic_close(magic);
			free(data);
			die(1, file, "Failed to read cover image", NULL);
		}
		free(data);
		data = new_data;
	} else {
		free(data);
		die(1, file, "Not an image file", file_type, NULL);
	}
	magic_close(magic);

	// read image
	ILuint image_ = 0;
	ilGenImages(1, &image_);
	if (!image_) {
		free(data);
		die(1, file, "Failed to create image", ilGetError(), NULL);
	}
	ilBindImage(image_);
	bool ret = ilLoadL(IL_TYPE_UNKNOWN, data, size);
	free(data);
 	if (!ret) {
		ILenum error = ilGetError();
		ilDeleteImages(1, &image_);
		die(1, file, "Failed to read image, image is not recognized by DevIL", error, NULL);
	}

	struct vector2 image_size_ = { ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT) };
	*image_size = image_size_;

	ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);
	ILenum error = ilGetError();
	if (error != IL_NO_ERROR) {
		ilDeleteImages(1, &image_);
		die(1, file, "Failed to convert image to readable format", error, NULL);
	}

	ILubyte *image_data_ = ilGetData();
	if (!image_data_) {
		ilDeleteImages(1, &image_);
		die(1, file, "Failed to get image data", NULL);
	}

	ILint image_bpp_ = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	*image_bpp = image_bpp_;

	if (apply_transparency) {
		for (ILint y = 0; y < image_size_.x; ++y) {
			for (ILint x = 0; x < image_size_.y; ++x) {
				ILubyte *pix = &image_data_[(y * image_size_.y + x) * image_bpp_];
				if (pix[3] != UCHAR_MAX) {
					pix[0] = lerpc(apply_color.b, pix[0], pix[3]);
					pix[1] = lerpc(apply_color.g, pix[1], pix[3]);
					pix[2] = lerpc(apply_color.r, pix[2], pix[3]);
				}
			}
		}
	}

	*image_data = image_data_;
	return true;
}
