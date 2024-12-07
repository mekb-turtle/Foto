#include "term.h"
#include "util.h"
#include <SDL2/SDL_image.h>
#include "color.h"

static bool get_pixel(SDL_Surface *surface, struct position position, struct color *color) {
	Uint8 *p = &((Uint8 *) surface->pixels)[position.y * surface->pitch + position.x * surface->format->BytesPerPixel];
	Uint32 c = *(Uint32 *) p;

	// Extract RGBA values
	SDL_GetRGBA(c, surface->format, &color->r, &color->g, &color->b, &color->a);

	return true;
}

static void print_color(struct color color, bool fg, enum bit_depth bit_depth, FILE *fp) {
	fprintf(fp, "\x1b[%c", fg ? '3' : '4');
	switch (bit_depth) {
		case BIT_4:
			fprintf(fp, "8;5;%d", rgb_to_4bit(color));
			break;
		case BIT_8:
			fprintf(fp, "8;5;%d", rgb_to_8bit(color));
			break;
		case BIT_24:
			fprintf(fp, "8;2;%d;%d;%d", color.r, color.g, color.b);
			break;
		default:
			fprintf(fp, "0");
			break;
	}
	fprintf(fp, "m");
}

static void move_cursor(struct position position) {
	fprintf(stdout, "\x1b[%d;%dH", position.y + 1, position.x + 1);
}

enum render_callback render_image_to_terminal(SDL_Surface *surface, bool unicode, enum bit_depth bit_depth, FILE *fp, bool (*callback)()) {
	enum render_callback ret = FAIL;
	// lock surface
	if (SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0) return FAIL;

	if (surface->format->BitsPerPixel != TERM_DEPTH) goto end;
	if (surface->h <= 0 || surface->w <= 0) goto end;

	// print each pixel
	struct position c;
	unsigned int y_mul = unicode ? 2u : 1u;

	for (c.y = 0; c.y < (unsigned int) surface->h; c.y += y_mul) {
		struct color col_bg_old, col_fg_old;
		bool bg_set = false;
		for (c.x = 0; c.x < (unsigned int) surface->w; ++c.x) {
			// check if we should stop
			if (callback && !callback()) {
				ret = ABORT;
				goto end;
			}

			struct position term_pos = {.x = c.x, .y = c.y / y_mul};

			// get pixel color(s)
			struct color col_bg, col_fg;
			if (!get_pixel(surface, c, &col_bg)) goto end;
			if (unicode) {
				if (c.y + 1 >= (unsigned int) surface->h) col_fg = col_bg; // out of bounds
				if (!get_pixel(surface, (struct position){.x = c.x, .y = c.y + 1}, &col_fg)) goto end;
			}

			// set cursor for first column, the cursor is moved automatically by the terminal for the next columns
			if (c.x == 0) {
				move_cursor(term_pos);
			}

			// update bg color if last color was different
			if (c.x == 0 || col_bg.u32 != col_bg_old.u32) {
				col_bg_old = col_bg;
				print_color(col_bg, false, bit_depth, fp);
			}

			// print unicode block
			if (unicode && col_bg.u32 != col_fg.u32) { // save bandwidth
				// update fg color if last color was different
				if (!bg_set || col_fg.u32 != col_fg_old.u32) {
					bg_set = true;
					col_fg_old = col_fg;
					print_color(col_fg, true, bit_depth, fp);
				}
				fprintf(fp, "\u2584"); // lower half block
			} else {
				fprintf(fp, " ");
			}
		}
		fprintf(fp, "\x1b[0m");
	}

	ret = SUCCESS;
end:
	fprintf(fp, "\x1b[0m");
	fflush(fp);
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
	return ret;
}
