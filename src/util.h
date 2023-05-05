struct color {
	unsigned char r, g, b;
};

struct vector2 {
	int x, y;
};

struct dvector2 {
	double x, y;
};

struct dvector2 vector2_to_dvector2(struct vector2 vec);
void letterboxing(struct vector2 window_size, struct vector2 image_size, struct dvector2 *translate, struct dvector2 *scale, struct dvector2 *scale_inv);
