#include <stdbool.h>
#include <SDL2/SDL.h>
#include <locale.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

unsigned long long get_time();
SDL_Rect get_fit_mode(SDL_Point image_size_, SDL_Point window_size_);
struct lconv get_lconv();
char *str_fallback(char *str, char *fallback);
