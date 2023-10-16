#include <stdio.h>

FILE *open_file(char *filename);

void close_file(FILE *fp);

SDL_Surface *read_file(FILE *fp);
