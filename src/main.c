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
#include <math.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <IL/il.h>

#include "./util.h"
#include "./img.h"

#define BLOCK 1024
#define eprintf(...) fprintf(stderr, __VA_ARGS__)

XClassHint *class_hint = NULL;
XSizeHints *size_hint = NULL;
Window win;
bool win_init = false;
bool il_init = false;
ILuint image = 0;
bool image_init = false;
ILuint new_image = 0;
bool new_image_init = false;
Pixmap pixmap;
bool pixmap_init = false;
Pixmap alpha_pixmap;
bool alpha_pixmap_init = false;
XImage *alpha_pixmap_image = NULL;
XGCValues alpha_gc_values;
GC alpha_gc;
bool alpha_gc_init = false;
ILint image_bpp;
ILubyte *image_data = NULL;
cairo_surface_t *image_surface = NULL;
cairo_t *image_cr = NULL;
cairo_surface_t *draw_surface = NULL;
cairo_t *draw_cr = NULL;
Display *dpy = NULL;
bool exited = false;
bool running = false;

void die(const char *msg, const char *msg2, int exitcode) {
	running = false;
	if (!exited) {
		// print message
		if (msg2 && !msg) { msg = msg2; msg2 = NULL; }
		if (msg2) eprintf("%s: %s\n", msg, msg2);
		else if (msg) eprintf("%s\n", msg);

		// tidy everything up
		exited = true;
		if (image_cr) { cairo_destroy(image_cr); image_cr = NULL; }
		if (image_surface) { cairo_surface_destroy(image_surface); image_surface = NULL; }
		if (draw_surface) cairo_surface_flush(draw_surface);
		if (draw_cr) { cairo_destroy(draw_cr); draw_cr = NULL; }
		if (draw_surface) { cairo_surface_destroy(draw_surface); draw_surface = NULL; }
		if (image_surface) cairo_surface_flush(image_surface);
		if (image_cr) { cairo_destroy(image_cr); image_cr = NULL; }
		if (image_surface) { cairo_surface_destroy(image_surface); image_surface = NULL; }
		if (alpha_gc_init) { XFreeGC(dpy, alpha_gc); alpha_gc_init = false; }
		if (alpha_pixmap_image) { XDestroyImage(alpha_pixmap_image); alpha_pixmap_image = NULL; }
		if (alpha_pixmap_init) { XFreePixmap(dpy, alpha_pixmap); alpha_pixmap_init = false; }
		if (pixmap_init) { XFreePixmap(dpy, pixmap); pixmap_init = false; }
		if (new_image_init) { ilDeleteImages(1, &new_image); new_image_init = false;}
		if (image_init) { ilDeleteImages(1, &image); image_init = false; }
		if (class_hint) { XFree(class_hint); class_hint = NULL; }
		if (size_hint) { XFree(size_hint); size_hint = NULL; }
		if (win_init) { XDestroyWindow(dpy, win); win_init = false; }
		if (dpy) { XCloseDisplay(dpy); dpy = NULL; }
		if (il_init) { ilShutDown(); il_init = false; }
	}
	exit(exitcode);
}

void exit_handler() {
	if (!running) {
		die(NULL, NULL, 2);
		return;
	}
	running = false;
}

int usage(char *argv0) {
	eprintf("Usage: %s <file>\n\
	-t, --title title        : set window title\n\
	-c, --class class        : set window class name\n\
	-p, --pos x y            : set window position, default to center\n\
	-s, --size w h           : set window size, default to image size\n\
	-b, --bg r g b           : set background colour where there is transparency\n\
	-h, --hotreload          : reload image when it is changed, will not work with stdin\n\
	-B, --borderless         : remove the border from the window\n\
	-u, --transparent        : window's transparency will match the image\n", argv0);
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

void create_pixmap_surface(struct vector2 size, unsigned int depth, Visual *visual, bool transparent, GC gc) {
	pixmap = XCreatePixmap(dpy, win, size.x, size.y, depth);
	if (!pixmap) die("Failed to create pixmap", NULL, 1);
	pixmap_init = true;

	if (transparent) {
		alpha_pixmap = XCreatePixmap(dpy, win, size.x, size.y, 1);
		if (!alpha_pixmap) die("Failed to create alpha pixmap", NULL, 1);
		alpha_pixmap_init = true;

		alpha_pixmap_image = XGetImage(dpy, alpha_pixmap, 0, 0, size.x, size.y, AllPlanes, ZPixmap);
		if (!alpha_pixmap_image) die("Failed to get image of alpha pixmap", NULL, 1);

		alpha_gc = XCreateGC(dpy, alpha_pixmap, 0, &alpha_gc_values);
		alpha_gc_init = true;
	}

	draw_surface = cairo_xlib_surface_create(dpy, pixmap, visual, size.x, size.y);
	if (!draw_surface) die("Failed to create cairo surface", NULL, 1);

	draw_cr = cairo_create(draw_surface);
	if (!draw_cr) die("Failed to create cairo context", NULL, 1);
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
	struct vector2 window_pos, window_size;
	struct color bg = { 0, 0, 0 };
	bool set_pos = false, set_size = false, set_bg = false;

	{
	char flag_set_title = 0, flag_set_class = 0, flag_set_pos = 0, flag_set_size = 0, flag_set_bg = 0;
	char *str_x = NULL, *str_y = NULL, *str_w = NULL, *str_h = NULL;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; else // finish flags/options

			// if we try to put a flag/option before finishing the last option 
			if (flag_set_title) invalid; else
			if (flag_set_class) invalid; else
			if (flag_set_pos) invalid; else
			if (flag_set_size) invalid; else
			if (flag_set_bg) invalid; else

			if (argv[i][1] != '-' && !argch(argv[i]+1, "tcpsbhBu")) invalid;
			argoption('t', "--title", title, flag_set_title);
			argoption('c', "--class", class, flag_set_class);
			argoption('p', "--pos", str_x || str_y, flag_set_pos);
			argoption('s', "--size", str_w || str_h, flag_set_size);
			argoption('b', "--bg", set_bg, flag_set_bg);
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
			if (!to_int(str_x, &window_pos.x, true, false)) invalid;
		} else if (flag_set_pos == 2) {
			set_pos = true;
			flag_set_pos = 0;
			char *str_y = argv[i]; // set y of pos
			if (!to_int(str_y, &window_pos.y, true, false)) invalid;

		} else if (flag_set_size == 1) {
			++flag_set_size;
			char *str_w = argv[i]; // set width of size
			if (!to_int(str_w, &window_size.x, true, false)) invalid;
		} else if (flag_set_size == 2) {
			set_size = true;
			flag_set_size = 0;
			char *str_h = argv[i]; // set height of size
			if (!to_int(str_h, &window_size.y, true, false)) invalid;

		} else if (flag_set_bg == 1) {
			++flag_set_bg;
			char *str_r = argv[i]; // set r of bg
			int bg_;
			if (!to_int(str_r, &bg_, true, false)) invalid;
			bg.r = bg_;
		} else if (flag_set_bg == 2) {
			++flag_set_bg;
			char *str_g = argv[i]; // set g of bg
			int bg_;
			if (!to_int(str_g, &bg_, true, false)) invalid;
			bg.g = bg_;
		} else if (flag_set_bg == 3) {
			set_bg = true;
			flag_set_bg = 0;
			char *str_b = argv[i]; // set b of bg
			int bg_;
			if (!to_int(str_b, &bg_, true, false)) invalid;
			bg.b = bg_;
		} else if (!file) {
			file = argv[i];
		} else {
			invalid;
		}
	}

	if (flag_set_title || flag_set_pos || flag_set_size || flag_set_bg) invalid; // if we didn't finish an option
	if (!file) invalid; // if file wasn't specified
	}

	signal(SIGHUP, exit_handler);
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGUSR1, exit_handler);
	signal(SIGUSR2, exit_handler);
	signal(SIGTERM, exit_handler);

	ilInit();
	il_init = true;

	struct vector2 image_size;

	char *filename = NULL;
	bool is_stdin;
	image_init = readfilesurface(die, file, &is_stdin, &filename, BLOCK, &image_data, &image_surface, &image_cr, &image, &image_size, &image_bpp, true, bg);
	if (!image_init) die("Failed to read data", NULL, 1);

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
		window_size.x = image_size.x,
		window_size.y = image_size.y;
	}

	dpy = XOpenDisplay(NULL);
	if (!dpy) die("Cannot open display", NULL, 1);

	int screen = DefaultScreen(dpy);
	Window root = DefaultRootWindow(dpy);
	Visual *visual = DefaultVisual(dpy, screen);
	unsigned int depth = DefaultDepth(dpy, screen);
	GC gc = DefaultGC(dpy, screen);

	win = XCreateSimpleWindow(dpy, root, 0, 0, window_size.x, window_size.y, 0,
			BlackPixel(dpy, screen), WhitePixel(dpy, screen));
	if (!win) die("Cannot create window", NULL, 1);

	XSetWindowAttributes attrs;
	unsigned long attrs_flags = CWBackPixel;
	attrs.background_pixel = color_to_long(bg);
	if (flag_borderless) {
		attrs.override_redirect = True;
		attrs_flags |= CWOverrideRedirect;
	}
	XChangeWindowAttributes(dpy, win, attrs_flags, &attrs);

	Atom wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);

	XSelectInput(dpy, win, ExposureMask | StructureNotifyMask | SubstructureNotifyMask);
	XMapWindow(dpy, win);
	XStoreName(dpy, win, title);

	win_init = true;

	class_hint = XAllocClassHint();
	if (!class_hint) die("Failed to allocate class hint", NULL, 1);
	XGetClassHint(dpy, win, class_hint);
	class_hint->res_name = class;
	class_hint->res_class = class;
	XSetClassHint(dpy, win, class_hint);

	if (set_pos) XMoveWindow(dpy, win, window_pos.x, window_pos.y);

	size_hint = XAllocSizeHints();
	if (!size_hint) die("Failed to allocate size hint", NULL, 1);
	XSetWMNormalHints(dpy, win, size_hint);

	create_pixmap_surface(window_size, depth, visual, flag_transparent, gc);

	bool first_resize = true;
	bool first_expose = true;
	bool hotreload = false;
    XEvent e;

	struct transform transform, old_transform;
	letterboxing(window_size, image_size, &transform);

	unsigned long long file_last_checked = 0;
	unsigned long long file_checked = 0;

	running = true;
	while (running) {
		while (running && XPending(dpy)) {
			if (!first_expose)
				XNextEvent(dpy, &e);
			else
				e.type = Expose;
			switch (e.type) {
				case ConfigureNotify:
					if (first_resize) {
						first_resize = false;
						if (set_pos) XMoveWindow(dpy, win, window_pos.x, window_pos.y);
						if (set_size && (e.xconfigure.width != window_size.x || e.xconfigure.height != window_size.y)) {
							eprintf("Failed to resize window or did not resize to the correct size. Using a tiling window manager?\n");
						}
					}
					if (e.xconfigure.width == window_size.x && e.xconfigure.height == window_size.y) break;
					cairo_destroy(draw_cr);
					cairo_surface_destroy(draw_surface);
					if (flag_transparent) {
						XDestroyImage(alpha_pixmap_image);
						alpha_pixmap_image = NULL;
						XFreeGC(dpy, alpha_gc);
						alpha_gc_init = false;
						XFreePixmap(dpy, alpha_pixmap);
						alpha_pixmap_init = false;
					}
					XFreePixmap(dpy, pixmap);
					pixmap_init = false;
					window_size.x = e.xconfigure.width;
					window_size.y = e.xconfigure.height;
					create_pixmap_surface(window_size, depth, visual, flag_transparent, gc);
					letterboxing(window_size, image_size, &transform);
				case Expose:
					if (first_expose) {
						first_expose = false;
						letterboxing(window_size, image_size, &transform);
					}
					if (!transformcmp(old_transform, transform)) {
						old_transform = transform;
						clear_surface(draw_surface, draw_cr, bg.r, bg.g, bg.b);
						cairo_translate(draw_cr, transform.translate.x, transform.translate.y);
						cairo_scale(draw_cr, transform.scale, transform.scale);
						cairo_set_source_surface(draw_cr, image_surface, 0, 0);
						cairo_paint(draw_cr);
						cairo_scale(draw_cr, transform.scale_inv, transform.scale_inv);
						cairo_translate(draw_cr, -transform.translate.x, -transform.translate.y);
						cairo_surface_flush(draw_surface);
						if (flag_transparent) {
							bool is_visible;
							struct vector2 image_pos;
							ILubyte pixel;
							for (double y = 0; y < window_size.y; ++y)
								for (double x = 0; x < window_size.x; ++x) {
									if (x <  transform.translate.x                    || y <  transform.translate.y ||
										x >= transform.translate.x + transform.size.x || y >= transform.translate.y + transform.size.y) is_visible = false;
									else {
										image_pos.x = round((x - transform.translate.x) * transform.scale_inv);
										image_pos.y = round((y - transform.translate.y) * transform.scale_inv);
										pixel = image_data[(image_pos.y * image_size.x + image_pos.x) * image_bpp + 3];
										is_visible = pixel & 0x80;
									}
									XPutPixel(alpha_pixmap_image, x, y, is_visible ? 1 : 0);
								}

							XPutImage(dpy, alpha_pixmap, alpha_gc, alpha_pixmap_image, 0, 0, 0, 0, window_size.x, window_size.y);
						}
					}
					XCopyArea(dpy, pixmap, win, gc, 0, 0, window_size.x, window_size.y, 0, 0);
					if (flag_transparent) XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, alpha_pixmap, ShapeSet);
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
		if (!is_stdin && flag_hotreload && running) {
			file_checked = get_time();
			if (file_checked > file_last_checked + 500) {
				file_last_checked = file_checked;
				if (stat(file, &st) != 0) die(file, strerror(errno), 1);
				if (st.st_mtime != prev_mtime) {
					prev_mtime = st.st_mtime;
					hotreload = true;
				}
			}
		}
		if (hotreload) {
			hotreload = false;
			cairo_surface_flush(image_surface);
			cairo_destroy(image_cr);
			cairo_surface_destroy(image_surface);
			ilDeleteImages(1, &image);
			image_cr = NULL;
			image_surface = NULL;
			image_init = false;
			if (!readfilesurface(die, file, &is_stdin, &filename, BLOCK, &image_data, &image_surface, &image_cr, &image, &image_size, &image_bpp, true, bg)) die("Failed to read data", NULL, 1);
			letterboxing(window_size, image_size, &transform);
			image_init = true;
			XClearWindow(dpy, win);
		}
    }
	die(NULL, NULL, 0);
	return 0;
}
