#include <err.h>
#include <errno.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "./image.h"

#define BLOCKSIZE 1024

// get file pointer from filename
FILE *open_file(char *filename) {
	if (filename[0] == '-' && filename[1] == '\0') {
		return stdin;
	}

	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		warn("%s", filename);
		return NULL;
	}
	return fp;
}

void close_file(FILE *fp) {
	if (fp && fp != stdin && fp != stdout && fp != stderr) {
		fclose(fp);
	}
}

// get surface from file pointer
SDL_Surface *read_file(FILE *fp) {
	if (!fp) {
		warnx("Failed to read file");
		return NULL;
	}

	// read the file in blocks of blocksize bytes
	// because SDL can't read from stdin
	void *data = NULL;
	size_t read, size = 0;
	while (!feof(fp) && !ferror(fp)) {
		data = realloc(data, size + BLOCKSIZE);
		if (!data) {
			warnx("realloc");
			return NULL;
		}
		read = fread(data + size, 1, BLOCKSIZE, fp);
		if (read <= 0) break;
		size += read;
		if (size >= INT32_MAX) {
			warnx("file is too large");
			return NULL;
		}
	}

	if (ferror(fp)) {
		warnx("ferror");
	}

	SDL_RWops *rw = SDL_RWFromMem(data, (int) size);

	if (!rw) {
		free(data);
		warnx("%s", IMG_GetError());
		return NULL;
	}

	SDL_Surface *surface = IMG_Load_RW(rw, 0);

	SDL_RWclose(rw);
	free(data);

	if (!surface) {
		warnx("%s", IMG_GetError());
		return NULL;
	}

	return surface;
}
