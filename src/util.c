#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>

#include "./util.h"

// convert color struct to X color
unsigned long color_to_long(struct color color) {
	return (color.r << 020) | (color.g << 010) | color.b;
}

// convert int vector to double vector
struct dvector2 vector2_to_dvector2(struct vector2 vec) {
	if (vec.x == 0) vec.x = 1;
	if (vec.y == 0) vec.y = 1;
	return (struct dvector2){ (double)vec.x, (double)vec.y };
}

// finds the right transformation to make the image fit to the screen and nothing is cropped
void letterboxing(struct vector2 window_size, struct vector2 image_size, struct transform *transform) {
	struct dvector2 window_dsize = vector2_to_dvector2(window_size);
	struct dvector2 image_dsize  = vector2_to_dvector2(image_size);
	double scale;
	double scale_inv;
	if (image_dsize.x / image_dsize.y > window_dsize.x / window_dsize.y) {
		scale      = window_dsize.x / image_dsize.x;
		scale_inv  = image_dsize.x / window_dsize.x;
		transform->size.x = window_dsize.x;
		transform->size.y = scale * image_dsize.y;
		transform->translate.x = 0;
		transform->translate.y = window_dsize.y / 2.0 - image_dsize.y * scale / 2.0;
	} else {
		scale      = window_dsize.y / image_dsize.y;
		scale_inv  = image_dsize.y / window_dsize.y;
		transform->size.y = window_dsize.y;
		transform->size.x = scale * image_dsize.x;
		transform->translate.y = 0;
		transform->translate.x = window_dsize.x / 2.0 - image_dsize.x * scale / 2.0;
	}
	transform->scale     = scale;
	transform->scale_inv = scale_inv;
}

// compares transform struct
bool transformcmp(struct transform a, struct transform b) {
	return dvector2cmp(a.translate, b.translate) && dvector2cmp(a.size, b.size) && a.scale == b.scale && a.scale_inv == b.scale_inv;
}

// compares int vector struct
bool vector2cmp(struct vector2 a, struct vector2 b) {
	return a.x == b.x && a.y == b.y;
}

// compares double vector struct
bool dvector2cmp(struct dvector2 a, struct dvector2 b) {
	return a.x == b.x && a.y == b.y;
}

// get current time in ms
unsigned long long get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((unsigned long long)tv.tv_sec * 1000) + ((unsigned long long)tv.tv_usec / 1000);
}

// linear interpolation
double lerp(double a, double b, double t) {
	if (t == 0) return a;
	if (t == 1) return b;
	return (b-a)*t+a;
}

// linear interpolation but for unsigned char
unsigned char lerpc(unsigned char a, unsigned char b, unsigned char t) {
	if (t == 0) return a;
	if (t == UCHAR_MAX) return b;
	return (unsigned char)(lerp((double)a, (double)b, ((double)t)/UCHAR_MAX) + 0.5);
}

// read number from a string, all numbers have to be digits
bool to_int(char *input, char *end, bool allow_zero, bool byte, int *output) {
	if (*input == '\0') return false; // if first character is null, return false
	int r = 0;
	for (char *input_ = input; (!end || input_ < end) && *input_; ++input_, ++r) { // loop characters
		if (!isdigit(*input_)) return false; // if character is not a digit, return false
		if (r >= (byte ? 3 : 5)) return false; // if length of string is greater than 5 (or 3 if byte is true), return false
	}
	int output_ = atoi(input); // convert to number
	if (output_ < (allow_zero ? 0 : 1)) return false; // check minimum value
	if (output_ > (byte ? UCHAR_MAX : SHRT_MAX)) return false; // check maximum value
	*output = output_;
	return true;
}

// to_int but for multiple numbers separated by commas
bool multi_to_int(char *input, bool allow_zero, bool byte, int *output, ...) {
	if (*input == '\0' || *input == ',') return false;

	// loop output pointers
	va_list args;
	va_start(args, output);
	int *arg = output;
	char *input_next;

	while (true) {
		// position of next comma
		input_next = input ? strchr(input, ',') : NULL;

		// fail if we expect an argument but it is not there or if we don't expect an argument but it is there
		if ((arg && !input) || (!arg && input)) { va_end(args); return false; }
		if (!arg && !input) break; // exit and return true if we don't expect an argument and it is not there
		
		// convert the number
		if (!to_int(input, input_next, allow_zero, byte, arg)) { va_end(args); return false; }

		// get next argument
		arg = va_arg(args, int*);

		input = input_next;
		if (input) ++input; // set input to character after comma if it is not NULL
	}

	va_end(args);
	return true;
}

