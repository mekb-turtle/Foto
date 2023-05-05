#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <bsd/inttypes.h>
#include <ctype.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <IL/il.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

XClassHint *classHint = NULL;
XSizeHints *sizeHint = NULL;
Window win;
bool win_init = false;
bool il_init = false;
ILuint image;
bool image_init = false;
Pixmap pixmap;
bool pixmap_init = false;
cairo_surface_t *image_surface = NULL;
cairo_t *image_cr = NULL;
cairo_surface_t *draw_surface = NULL;
cairo_t *draw_cr = NULL;
Display *dpy = NULL;

void die(const char *msg, const char *msg2, int exitcode) {
	if (image_cr) cairo_destroy(image_cr);
	if (image_surface) cairo_surface_destroy(image_surface);
	if (draw_cr) cairo_destroy(draw_cr);
	if (draw_surface) cairo_surface_destroy(draw_surface);
	if (pixmap_init) XFreePixmap(dpy, pixmap);
	if (image_init) ilDeleteImages(1, &image);
	if (classHint) XFree(classHint);
	if (sizeHint) XFree(sizeHint);
	if (win_init) XDestroyWindow(dpy, win);
	if (dpy) XCloseDisplay(dpy);
	if (il_init) ilShutDown();
	if (msg2 && !msg) { msg = msg2; msg2 = NULL; }
	if (msg2) eprintf("%s: %s\n", msg, msg2);
	else if (msg) eprintf("%s\n", msg);
	exit(exitcode);
}

void clear_surface(cairo_surface_t *surface, cairo_t *cr, unsigned char r, unsigned char g, unsigned char b) {
	cairo_set_source_rgba(cr, (double)r / 255.0, (double)g / 255.0, (double)b / 255.0, 1.0);
	cairo_paint(cr);
	cairo_surface_flush(surface);
}

int usage(char *argv0) {
	eprintf("Usage: %s <file>\n\
	-t --title <title>      : set window title\n\
	-c --class <class>      : set window class name\n\
	-p --pos <x> <y>        : set window position, default to center\n\
	-s --size <w> <h>       : set window size, default to image size\n\
	-b --bg <r> <g> <b>     : set background colour where there is transparency\n\
	-h --hotreload          : reload image when it is changed, file\n\
	                          pointer is kept so deleting the file and\n\
	                          recreating it probably won't work,\n\
	                          will not work with stdin\n\
	-B --borderless         : remove the border from the window\n\
	-u --transparent        : window's transparency will match the image\n", argv0);
	return 2;
}

bool to_int(char *input, int *output, bool allow_zero, bool byte) {
	if (!*input) return false;
	int r = 0;
	for (char *input_ = input; *input_; ++input_) {
		if (!isdigit(*input_)) return false;
		if (r++ > (byte ? 3 : 10)) return false;
	}
	r = 0;
	*output = strtoi(input, NULL, 10, allow_zero ? 0 : 1, byte ? 255 : INT_MAX, &r);
	if (r < 0) return false;
	return true;
}

bool argch(char *flag, char *possible_flags) {
	// checks if all flags in flag are in possible_flags exactly once
	// returns false if an unknown flag appears or a flag appears more than once
	// if you know a better way to do this please make an issue
	int len = strlen(possible_flags);
	char *_possible_flags = malloc(len); // create a backup of it so we can edit it
	if (!_possible_flags) die("Failed malloc", strerror(errno), errno);
	memcpy(_possible_flags, possible_flags, len);
	for (; *flag; ++flag) { // loop over current flags
		char *f = _possible_flags;
		bool found = false;
		for (int i = 0; i < len; ++i, ++f) { // loop over possible flags that exist
			if (!*f) continue;
			// we are using len instead of checking if it is null because we are setting characters in it to null
			if (*f != *flag) continue; // if the current flag is this flag
			found = true; // we found the flag
			*f = '\0'; // set this flag to zero so we don't repeat over it again
			break;
		}
		if (!found) { // if the current flag is unknown
			free(_possible_flags);
			return false;
		}
	}
	free(_possible_flags);
	return true;
}

bool readfile(char *file, bool *is_stdin, char **filename, int blocksize, cairo_surface_t **surface, cairo_t **cr, ILuint *image, ILint *image_w, ILint *image_h) {
	// gets a FILE* from the file name
	FILE *fp;
	bool f = false;
	if (file[0] == '-' && file[1] == '\0') {
		*is_stdin = true;
		fp = stdin;
		*filename = "stdin";
	} else {
		*is_stdin = false;
		f = true;
		fp = fopen(file, "rb"); // rb = read + binary
		if (!fp) { die(file, strerror(errno), 1); }
		char *slash = strrchr(file, '/'); // gets the basename of file path
		*filename = slash ? slash + 1 : file;
	}

	// reads a FILE* and outputs the data
	void *data = NULL;
	size_t size = 0;
	size_t read = 0;
	while (!feof(fp) && !ferror(fp)) {
		data = realloc(data, size + blocksize);
		if (!data) die("Failed realloc", NULL, 1);
		read = fread(data + size, 1, blocksize, fp);
		if (read <= 0) break;
		size += read;
	}

	bool ferr = ferror(fp);
	if (f) fclose(fp); // close the file
	if (ferr) {
		free(data);
		die("Failed to read file", NULL, 1);
		return false;
	}
	
	// read image
	ILuint image_;
	ilGenImages(1, &image_);
	ilBindImage(image_);
	bool ret = ilLoadL(IL_TYPE_UNKNOWN, data, size);
	free(data);
	if (!ret) die("Failed to read image", NULL, 1);

	ILint image_w_ = ilGetInteger(IL_IMAGE_WIDTH);
	ILint image_h_ = ilGetInteger(IL_IMAGE_HEIGHT);
	*image_w = image_w_;
	*image_h = image_h_;
	ILint image_depth = ilGetInteger(IL_IMAGE_DEPTH);
	ILint image_type = ilGetInteger(IL_IMAGE_TYPE);

	ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);

	ILubyte *image_data = ilGetData();
	if (!image_data) die("Failed to load image data", NULL, 1);

	cairo_surface_t *image_surface_ = cairo_image_surface_create_for_data(
			image_data, CAIRO_FORMAT_ARGB32, image_w_, image_h_,
			cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, image_w_));
	if (!image_surface_) die("Failed to create cairo surface", NULL, 1);

	cairo_t *cr_ = cairo_create(image_surface_);
	if (!cr_) die("Failed to create cairo drawing", NULL, 1);

	*surface = image_surface_;
	*cr = cr_;
	*image = image_;
	
	return ret;
}

void create_pixmap_surface(int window_x, int window_y, unsigned int depth, Visual *visual) {
	pixmap = XCreatePixmap(dpy, win, window_x, window_y, depth);
	if (!pixmap) die("Failed to create pixmap", NULL, 1);

	draw_surface = cairo_xlib_surface_create(dpy, pixmap, visual, window_x, window_y);
	if (!draw_surface) die("Failed to create cairo surface", NULL, 1);

	draw_cr = cairo_create(draw_surface);
	if (!draw_cr) die("Failed to create cairo drawing", NULL, 1);
}

int main(int argc, char *argv[]) {
#define invalid return usage(argv[0])
#define argcmp(flag, shortflag, longflag) \
		(strcmp(flag, longflag) == 0 || (flag[1] != '-' && strchr(flag+1, shortflag) != NULL))
#define argflag(shortflag, longflag, set, set2) \
	int set2 = 0; \
	if (argcmp(argv[i], shortflag, longflag)) { \
		set2 = 1; \
		set = 1; \
	}
#define argoption(shortflag, longflag, failcondition, set) \
	if (argcmp(argv[i], shortflag, longflag)) { \
		if (failcondition) invalid; \
		set = 1; \
	}

	bool flag_done = false, flag_hotreload = false, flag_borderless = false, flag_transparent = false;
	char *file = NULL, *title = NULL, *class = NULL;
	int window_x, window_y, window_w, window_h,
		bg_r, bg_g, bg_b;
	bool set_pos = false, set_size = false, set_bg = false;

	{
	char flag_set_title = 0, flag_set_class = 0, flag_set_pos = 0, flag_set_size = 0, flag_set_bg = 0;
	char *str_x = NULL, *str_y = NULL, *str_w = NULL, *str_h = NULL,
		 *str_r = NULL, *str_g = NULL, *str_b = NULL;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; else // finish flags/options

			// if we try to put a flag/option before finishing the last option 
			if (flag_set_title) invalid; else
			if (flag_set_class) invalid; else
			if (flag_set_pos) invalid; else
			if (flag_set_size) invalid; else
			if (flag_set_bg) invalid; else

			if (argv[i][1] != '-' && !argch(argv[i]+1, "tcprsbhBuT")) invalid;
			argoption('t', "--title", title, flag_set_title);
			argoption('c', "--class", class, flag_set_class);
			argoption('p', "--pos", str_x || str_y, flag_set_pos);
			argoption('s', "--size", str_w || str_h, flag_set_size);
			argoption('b', "--bg", str_r || str_g || str_b, flag_set_bg);
			argflag('h', "--hotreload", flag_hotreload, flag_set_hotreload);
			argflag('B', "--borderless", flag_borderless, flag_set_borderless);
			argflag('u', "--transparent", flag_transparent, flag_set_transparent);

			// prevent setting multiple options at once or invalid flag
			char total = flag_set_title + flag_set_class + flag_set_pos + flag_set_size + flag_set_bg;
			if (total > 1 || (total == 0 && !(flag_set_hotreload || flag_set_borderless || flag_set_transparent)))
				invalid;
		} else if (flag_set_title) {
			flag_set_title = 0;
			title = argv[i]; // set title option

		} else if (flag_set_class) {
			flag_set_class = 0;
			class = argv[i]; // set class option

		} else if (flag_set_pos == 1) {
			++flag_set_pos;
			char *str_x = argv[i]; // set x of pos
			if (!to_int(str_x, &window_x, true, false)) invalid;
		} else if (flag_set_pos == 2) {
			set_pos = true;
			flag_set_pos = 0;
			char *str_y = argv[i]; // set y of pos
			if (!to_int(str_y, &window_y, true, false)) invalid;

		} else if (flag_set_size == 1) {
			++flag_set_size;
			char *str_w = argv[i]; // set w of size
			if (!to_int(str_w, &window_w, true, false)) invalid;
		} else if (flag_set_size == 2) {
			set_size = true;
			flag_set_size = 0;
			char *str_h = argv[i]; // set h of size
			if (!to_int(str_h, &window_h, true, false)) invalid;

		} else if (flag_set_bg == 1) {
			++flag_set_bg;
			char *str_r = argv[i]; // set r of bg
			if (!to_int(str_r, &bg_r, true, false)) invalid;
		} else if (flag_set_bg == 2) {
			++flag_set_bg;
			char *str_g = argv[i]; // set g of bg
			if (!to_int(str_g, &bg_g, true, false)) invalid;
		} else if (flag_set_bg == 3) {
			set_bg = true;
			flag_set_bg = 0;
			char *str_b = argv[i]; // set b of bg
			if (!to_int(str_b, &bg_b, true, false)) invalid;
		} else if (!file) {
			file = argv[i];
		} else {
			invalid;
		}
	}

	if (flag_set_title || flag_set_pos || flag_set_size || flag_set_bg) invalid; // if we didn't finish an option
	if (!file) invalid; // if file wasn't specified
	}

	ilInit();
	il_init = true;

	ILint image_w, image_h;

	char *filename = NULL;
	bool is_stdin;
	if (!readfile(file, &is_stdin, &filename, 1024, &image_surface, &image_cr, &image, &image_w, &image_h)) die("Failed to read data", NULL, 1);
	image_init = true;

	struct stat st;
	time_t prev_mtime = 0;
	if (flag_hotreload) {
		if (is_stdin) die("Cannot use hotreload with stdin", NULL, 1);
		if (stat(file, &st) != 0) die(file, strerror(errno), 1);
		prev_mtime = st.st_mtime;
	}

	if (!title) {
		size_t size = strlen(filename) + 32;
		title = malloc(size);
		if (!title) die("Failed malloc", strerror(errno), errno);
		snprintf(title, size, "foto: %s", filename);
	}
	if (!class) class = "foto";

	if (!set_size) {
		// image size by default
		window_w = image_w,
		window_h = image_h;
	}

	if (!set_bg) {
		bg_r = bg_g = bg_b = 0;
	}

	dpy = XOpenDisplay(NULL);
	if (!dpy) die("Cannot open display", NULL, 1);

	int screen = DefaultScreen(dpy);
	Window root = DefaultRootWindow(dpy);
	Visual *visual = DefaultVisual(dpy, screen);
	unsigned int depth = DefaultDepth(dpy, screen);
	Colormap colormap = DefaultColormap(dpy, screen);
	GC gc = DefaultGC(dpy, screen);

	win = XCreateSimpleWindow(dpy, root, 0, 0, window_w, window_h, 0,
			BlackPixel(dpy, screen), WhitePixel(dpy, screen));
	if (!win) die("Cannot create window", NULL, 1);

	Atom wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);

	XSelectInput(dpy, win, ExposureMask | StructureNotifyMask | SubstructureNotifyMask);
	XMapWindow(dpy, win);
	XStoreName(dpy, win, title);

	win_init = true;

	classHint = XAllocClassHint();
	if (!classHint) die("Failed to allocate class hint", NULL, 1);
	XGetClassHint(dpy, win, classHint);
	classHint->res_name = class;
	classHint->res_class = class;
	XSetClassHint(dpy, win, classHint);

	if (set_pos) XMoveWindow(dpy, win, window_x, window_y);

	sizeHint = XAllocSizeHints();
	if (!sizeHint) die("Failed to allocate size hint", NULL, 1);
	XSetWMNormalHints(dpy, win, sizeHint);

	create_pixmap_surface(window_w, window_h, depth, visual);

	bool first_resize = true;
	bool running = true;
    XEvent e;

	while (running) {
		while (XPending(dpy)) {
			XNextEvent(dpy, &e);
			switch (e.type) {
				case ConfigureNotify:
					if (first_resize) {
						first_resize = false;
						if (set_size && (e.xconfigure.width != window_w || e.xconfigure.height != window_h)) {
							eprintf("Failed to resize window or did not resize to the correct size. Using a tiling window manager?\n");
						}
					}
					if (e.xconfigure.width == window_w && e.xconfigure.height == window_h) break;
					cairo_destroy(draw_cr);
					cairo_surface_destroy(draw_surface);
					XFreePixmap(dpy, pixmap);
					window_w = e.xconfigure.width;
					window_h = e.xconfigure.height;
					create_pixmap_surface(window_w, window_h, depth, visual);
				case Expose:
					clear_surface(draw_surface, draw_cr, bg_r, bg_g, bg_b);
					double translate_x = 0, translate_y = 0, scale_w = 1, scale_h = 1, scale_i_w = 1, scale_i_h = 1;
					double image_w_d = (double)image_w;
					double image_h_d = (double)image_h;
					double window_w_d = (double)window_w;
					double window_h_d = (double)window_h;
					if (image_w_d / image_h_d > window_w_d / window_h_d) {
						translate_y = window_h_d / 2.0 - window_w_d / 2.0;
						scale_w = scale_h = window_w_d / image_w_d;
						scale_i_w = scale_i_h = image_w_d / window_w_d;
					} else {
						translate_x = window_w_d / 2.0 - window_h_d / 2.0;
						scale_w = scale_h = window_h_d / image_h_d;
						scale_i_w = scale_i_h = image_h_d / window_h_d;
					}
					cairo_translate(draw_cr, translate_x, translate_y);
					cairo_scale(draw_cr, scale_w, scale_h);
					cairo_set_source_surface(draw_cr, image_surface, 0, 0);
					cairo_paint(draw_cr);
					cairo_scale(draw_cr, scale_i_w, scale_i_h);
					cairo_translate(draw_cr, -translate_x, -translate_y);
					cairo_surface_flush(draw_surface);
					XCopyArea(dpy, pixmap, win, gc, 0, 0, window_w, window_h, 0, 0);
					XFlush(dpy);
					break;
				case ClientMessage:
					if (e.xclient.message_type == wm_protocols &&
							e.xclient.data.l[0] == wm_delete_window) {
						running = false;
					}
					break;
			}
		}
		if (flag_hotreload) {
			if (stat(file, &st) != 0) die(file, strerror(errno), 1);
			if (st.st_mtime != prev_mtime) {
				prev_mtime = st.st_mtime;
				cairo_destroy(image_cr);
				cairo_surface_destroy(image_surface);
				ilDeleteImages(1, &image);
				image_cr = NULL;
				image_surface = NULL;
				image_init = false;
				if (!readfile(file, &is_stdin, &filename, 1024, &image_surface, &image_cr, &image, &image_w, &image_h)) die("Failed to read data", NULL, 1);
				image_init = true;
				XClearWindow(dpy, win);
			}
		}
    }
	die(NULL, NULL, 0);
	return 0;
}
