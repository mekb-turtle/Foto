#include <err.h>
#include <errno.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "./util.h"
#include "./image.h"

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
		eprintf("Failed to open file\n");
		return NULL;
	}

	// read the file manually, because SDL can't read from stdin
	uint8_t *data = NULL;
	size_t read, size = 0, alloc = 1024;
	while (!feof(fp) && !ferror(fp)) {
		if (!data) {
			data = malloc(alloc);
			if (!data) {
				warn("malloc");
				return NULL;
			}
		} else {
			alloc *= 2;
			void *new_data = realloc(data, alloc);
			if (!new_data) {
				free(data);
				warn("realloc");
				return NULL;
			}
			data = new_data;
		}
		// read from offset at `size` to offset at `alloc`
		read = fread(data + size, 1, alloc - size, fp);
		if (read <= 0) break;
		size += read;
		if (size >= INT32_MAX) {
			free(data);
			eprintf("File is too large\n");
			return NULL;
		}
	}

	if (ferror(fp)) {
		eprintf("Error reading file\n");
	}

	SDL_RWops *rw = SDL_RWFromMem(data, (int) size);

	if (!rw) {
		free(data);
		eprintf("Failed to read image: %s\n", SDL_GetError());
		return NULL;
	}

	SDL_Surface *surface = IMG_Load_RW(rw, 0);

	SDL_RWclose(rw);
	free(data);

	if (!surface) {
		eprintf("Failed to read image: %s\n", IMG_GetError());
		return NULL;
	}

	return surface;
}
