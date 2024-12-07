#ifndef TERM_H
#define TERM_H
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "color.h"

#define TERM_DEPTH (32)
#define TERM_R_MASK (0xff000000)
#define TERM_G_MASK (0x00ff0000)
#define TERM_B_MASK (0x0000ff00)
#define TERM_A_MASK (0x000000ff)

enum bit_depth {
	BIT_AUTO = 0,
	BIT_4,
	BIT_8,
	BIT_24
};

struct position {
	unsigned int x, y;
};

enum render_callback {
	FAIL,
	SUCCESS,
	ABORT
};
enum render_callback render_image_to_terminal(SDL_Surface *surface, bool unicode, enum bit_depth bit_depth, FILE *fp, bool (*callback)());
#endif // TERM_H
