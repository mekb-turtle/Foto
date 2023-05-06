#include <stdbool.h>

struct color {
	unsigned char r, g, b;
};

struct vector2 {
	int x, y;
};

struct dvector2 {
	double x, y;
};

struct transform {
	struct dvector2 translate;
	struct dvector2 size;
	double scale;
	double scale_inv;
};

unsigned long color_to_long(struct color color);
struct dvector2 vector2_to_dvector2(struct vector2 vec);
void letterboxing(struct vector2 window_size, struct vector2 image_size, struct transform *transform);
bool transformcmp(struct transform a, struct transform b);
bool vector2cmp(struct vector2 a, struct vector2 b);
bool dvector2cmp(struct dvector2 a, struct dvector2 b);
