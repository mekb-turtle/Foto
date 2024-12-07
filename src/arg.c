#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <SDL2/SDL.h>

#include "arg.h"
#include "util.h"

bool is_num(char *str) {
	// locale-aware
	char *plus = str_fallback(get_lconv()->positive_sign, "+");
	char *minus = str_fallback(get_lconv()->negative_sign, "-");
	size_t plus_len = strlen(plus);
	size_t minus_len = strlen(minus);
	// see if a char* is entirely digits (optionally with a leading '+' or '-' string)
	if (!*str) return false;
	for (bool first = true; *str; first = false) {
		if (!isdigit(*str)) {
			if (first) {
				// if the '+' string is encountered, skip it
				if ((strncmp(str, plus, plus_len) == 0)) {
					str += plus_len;
					continue;
				}
				// if the '-' string is encountered, skip it
				if ((strncmp(str, minus, minus_len) == 0)) {
					str += minus_len;
					continue;
				}
			}
			return false;
		}
		++str;
	}
	return true;
}

bool parse_num_array(char *str, long *out, int nums) {
	// split char* into array of numbers
	static const char *delim = ",";
	char *saveptr = NULL;
	char *token = strtok_r(str, delim, &saveptr);
	int i = 0;
	for (; token; ++i) {
		if (i >= nums) return false;
		if (!is_num(token)) return false;
		int errno_ = errno;
		errno = 0;
		out[i] = strtol(token, NULL, 10);
		if (errno != 0) return false;
		errno = errno_;
		token = strtok_r(NULL, delim, &saveptr);
	}
	return i == nums;
}

bool to_point(long in1, long in2, SDL_Point *out) {
	// convert longs to pos
	if (in1 >= 0 && in1 <= INT_MAX && in2 >= 0 && in2 <= INT_MAX) {
		*out = (SDL_Point){.x = (int) in1, .y = (int) in2};
		return true;
	}
	return false;
}

bool to_color(long in1, long in2, long in3, SDL_Color *out) {
	// convert longs to color
	if (in1 >= 0 && in1 <= UINT8_MAX && in2 >= 0 && in2 <= UINT8_MAX && in3 >= 0 && in3 <= UINT8_MAX) {
		*out = (SDL_Color){.r = (Uint8) in1, .g = (Uint8) in2, .b = (Uint8) in3, .a = 255};
		return true;
	}
	return false;
}
