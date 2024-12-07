#include "color.h"

#define SQ(x) ((x) * (x))

size_t closest_color(struct color color, struct color *color_table, size_t color_len) {
	size_t closest;
	uint32_t closest_dist = -1; // max value
	for (size_t i = 0; i < color_len; ++i) {
		uint32_t dist = SQ(color.r - color_table[i].r) + SQ(color.g - color_table[i].g) + SQ(color.b - color_table[i].b);
		if (dist < closest_dist) {
			closest = i;
			closest_dist = dist;
		}
	}
	return closest;
}

#define VAL(n) ((n) == 0 ? 0 : 40 * (n) + 55) // 0, 95, 135, 175, 215, 255

uint8_t rgb_to_8bit(struct color color) {
	static struct color color_table[240];
	static bool color_table_init = false;
	if (!color_table_init) {
		color_table_init = true;
		uint8_t i = 0;
		struct color color;
		color.a = 0xff;

		// fill colors (6^3)
		for (color.r = 0; color.r < 6; ++color.r)
			for (color.g = 0; color.g < 6; ++color.g)
				for (color.b = 0; color.b < 6; ++color.b, ++i) {
					color_table[i] = (struct color){
					        .r = VAL(color.r),
					        .g = VAL(color.g),
					        .b = VAL(color.b),
					};
				}
		// fill grayscale (24)
		for (color.r = 8; color.r < 238; color.r += 10, ++i) {
			color.b = color.g = color.r;
			color_table[i] = color;
		}
	}

	// find closest color (16-255)
	return closest_color(color, color_table, 240) + 16;
}

uint8_t rgb_to_4bit(struct color color) {
	static struct color color_table[16];
	static bool color_table_init = false;
	if (!color_table_init) {
		color_table_init = true;
		for (uint8_t i = 0; i < 16; ++i) {
			color_table[i] = (struct color){.a = 0xff};
			uint8_t n = (i & 0x8) ? 0xff : 0x80;
			// dynamically generate color table
			color_table[i].r = (i & 0x1) ? n : 0x00;
			color_table[i].g = (i & 0x2) ? n : 0x00;
			color_table[i].b = (i & 0x4) ? n : 0x00;
		};
		color_table[8] = color_table[7];
		color_table[7] = (struct color){{{0xc0, 0xc0, 0xc0, 0xff}}};
	}

	// find closest color (0-15)
	return closest_color(color, color_table, 16);
}
