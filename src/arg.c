#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <SDL2/SDL.h>

#include "./arg.h"

bool is_num(char *str) {
	// see if a char* is entirely digits (or starts with minus)
	if (!*str) return false;
	for (bool first = true; *str; ++str) {
		if (!isdigit(*str) && !(*str == '-' && first))
			return false;
		first = false;
	}
	return true;
}

bool parse_num_array(char *str, long *out, int nums) {
	// split char* into array of numbers
	const char *delim = ",";
	char *saveptr = NULL;
	char *token = strtok_r(str, delim, &saveptr);
	int i = 0;
	for (; token; ++i) {
		if (i >= nums) return false;
		if (!is_num(token)) return false;
		errno = 0;
		out[i] = strtol(token, NULL, 10);
		if (errno != 0) return false;
		token = strtok_r(NULL, delim, &saveptr);
	}
	return i == nums;
}

bool to_point(long in1, long in2, SDL_Point *out) {
	// convert longs to pos
	if (in1 >= 0 && in1 <= INT_MAX
	    && in2 >= 0 && in2 <= INT_MAX) {
		*out = (SDL_Point) {.x = (int) in1, .y = (int) in2};
		return true;
	}
	return false;
}

bool to_color(long in1, long in2, long in3, SDL_Color *out) {
	// convert longs to color
	if (in1 >= 0 && in1 <= UINT8_MAX
	    && in2 >= 0 && in2 <= UINT8_MAX
	    && in3 >= 0 && in3 <= UINT8_MAX) {
		*out = (SDL_Color) {.r = (Uint8) in1, .g = (Uint8) in2, .b = (Uint8) in3, .a = 255};
		return true;
	}
	return false;
}
