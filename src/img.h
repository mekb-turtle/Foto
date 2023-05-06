#include <cairo/cairo.h>
#include <IL/il.h>
#include <stdbool.h>
void clear_surface(cairo_surface_t *surface, cairo_t *cr, unsigned char r, unsigned char g, unsigned char b);
bool readfile(void (*die)(const char*, const char*, int), char *file, bool *is_stdin, char **filename, int blocksize, ILubyte **image_data, cairo_surface_t **surface, cairo_t **cr, ILuint *image, int *image_w, int *image_h);
