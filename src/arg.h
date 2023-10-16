#include <stdbool.h>

bool is_num(char *str);

bool parse_num_array(char *str, long *out, int nums);

bool to_point(long in1, long in2, SDL_Point *out);

bool to_color(long in1, long in2, long in3, struct SDL_Color *out);
