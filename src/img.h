#include <IL/il.h>
#include <stdbool.h>

#include "./util.h"

bool readimage(char *file, bool *is_stdin, char **filename, size_t blocksize, ILubyte **image_data, ILuint *image, struct vector2 *image_size, ILint *image_bpp, bool apply_transparency, struct color apply_color);
