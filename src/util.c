#include "./util.h"

struct dvector2 vector2_to_dvector2(struct vector2 vec) {
	if (vec.x == 0) vec.x = 1;
	if (vec.y == 0) vec.y = 1;
	return (struct dvector2){ (double)vec.x, (double)vec.y };
}

void letterboxing(struct vector2 window_size, struct vector2 image_size, struct dvector2 *translate, struct dvector2 *scale, struct dvector2 *scale_inv) {
	struct dvector2 window_dsize = vector2_to_dvector2(window_size);
	struct dvector2 image_dsize  = vector2_to_dvector2(image_size);
	double scale_;
	double scale_inv_;
	if (image_dsize.x / image_dsize.y > window_dsize.x / window_dsize.y) {
		scale_     = window_dsize.x / image_dsize.x;
		scale_inv_ = image_dsize.x / window_dsize.x;
		translate->x = 0;
		translate->y = window_dsize.y / 2.0 - image_dsize.y * scale_ / 2.0;
	} else {
		scale_     = window_dsize.y / image_dsize.y;
		scale_inv_ = image_dsize.y / window_dsize.y;
		translate->y = 0;
		translate->x = window_dsize.x / 2.0 - image_dsize.x * scale_ / 2.0;
	}
	scale->x     = scale->y     = scale_;
	scale_inv->x = scale_inv->y = scale_inv_;
}
