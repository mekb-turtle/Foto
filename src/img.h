#include <cairo/cairo.h>
#include <IL/il.h>
#include <stdbool.h>
#include "./util.h"
void clear_surface(cairo_surface_t *surface, cairo_t *cr, unsigned char r, unsigned char g, unsigned char b);
bool readfilesurface(void (*die)(const char*, const char*, int), char *file, bool *is_stdin, char **filename, size_t blocksize, ILubyte **image_data, cairo_surface_t **surface, cairo_t **cr, ILuint *image, struct vector2 *image_size, ILint *image_bpp, bool apply_transparency, struct color apply_color);
bool readfile(void (*die)(const char*, const char*, int), char *file, bool *is_stdin, char **filename, size_t blocksize, ILubyte **image_data, ILuint *image, struct vector2 *image_size, ILint *image_bpp, ILenum format, ILenum type, bool apply_transparency, struct color apply_color);
