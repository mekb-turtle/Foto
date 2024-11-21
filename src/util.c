#include <sys/time.h>

#include "./util.h"

unsigned long long get_time() {
	// get current time in ms
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((unsigned long long) tv.tv_sec * 1000) + ((unsigned long long) tv.tv_usec / 1000);
}

SDL_Rect get_fit_mode(SDL_Point image_size_, SDL_Point window_size_) {
	// gets the fit mode rect for an image and window size

	// convert image_size and window_size to float
	SDL_FPoint image_size = {.x = (float) image_size_.x, .y = (float) image_size_.y};
	SDL_FPoint window_size = {.x = (float) window_size_.x, .y = (float) window_size_.y};
	float scale;

	// avoid division by zero
	if (image_size.x == 0.0f || image_size.y == 0.0f || window_size.x == 0.0f || window_size.y == 0.0f)
		return (SDL_Rect){0, 0, 0, 0};

	// get fit mode rect from image and window size
	if (image_size.x / image_size.y > window_size.x / window_size.y) {
		scale = window_size.x / image_size.x;
		return (SDL_Rect){
		        .x = 0,
		        .y = (int) ((window_size.y * 0.5f) - (image_size.y * scale * 0.5f)),
		        .w = window_size_.x,
		        .h = (int) (scale * image_size.y)};
	} else {
		scale = window_size.y / image_size.y;
		return (SDL_Rect){
		        .x = (int) ((window_size.x * 0.5f) - (image_size.x * scale * 0.5f)),
		        .y = 0,
		        .w = (int) (scale * image_size.x),
		        .h = window_size_.y};
	}
}

struct lconv get_lconv() {
	static struct lconv *lconv = NULL;
	if (!lconv) lconv = localeconv();
	return lconv;
}

char *str_fallback(char *str, char *fallback) {
	if (!str || !*str) return fallback;
	return str;
}
