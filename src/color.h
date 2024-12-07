#ifndef COLOR_H
#define COLOR_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <SDL2/SDL.h>

struct color {
	union {
		struct {
			uint8_t r, g, b, a;
		};
		uint32_t u32;
	};
};

size_t closest_color(struct color color, struct color *color_table, size_t color_len);
uint8_t rgb_to_8bit(struct color color);
uint8_t rgb_to_4bit(struct color color);

#endif
