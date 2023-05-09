#include "./util.h"
#include <sys/time.h>
#include <stdio.h>

unsigned long color_to_long(struct color color) {
	return (color.r << 020) | (color.g << 010) | color.b;
}

struct dvector2 vector2_to_dvector2(struct vector2 vec) {
	if (vec.x == 0) vec.x = 1;
	if (vec.y == 0) vec.y = 1;
	return (struct dvector2){ (double)vec.x, (double)vec.y };
}

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

bool transformcmp(struct transform a, struct transform b) {
	return dvector2cmp(a.translate, b.translate) && dvector2cmp(a.size, b.size) && a.scale == b.scale && a.scale_inv == b.scale_inv;
}

bool vector2cmp(struct vector2 a, struct vector2 b) {
	return a.x == b.x && a.y == b.y;
}

bool dvector2cmp(struct dvector2 a, struct dvector2 b) {
	return a.x == b.x && a.y == b.y;
}

unsigned long long get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((unsigned long long)tv.tv_sec * 1000) + ((unsigned long long)tv.tv_usec / 1000);
}
