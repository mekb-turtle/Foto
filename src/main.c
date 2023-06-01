#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdarg.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <IL/il.h>

#include "./main.h"
#include "./util.h"
#include "./img.h"

#ifndef EXEC
#define EXEC "foto"
#endif
#ifndef VERSION
#define VERSION "N/A"
#endif

// amount of bytes to read at a time, mainly for reading the image file
#define BLOCK 1024
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define FILE_CHECK_TIME 500

// Xlib window stuff
XClassHint *class_hint = NULL;
XSizeHints *size_hint = NULL;
Window win;
Display *dpy = NULL;
Visual *visual = NULL;
bool win_init = false;

// DevIL stuff (developer's image library)
bool il_init = false;
ILuint image = 0;
bool image_init = false;
ILint image_bpp;
ILubyte *image_data = NULL;

// window drawing
Pixmap window_pixmap;
bool window_pixmap_init = false;
cairo_surface_t *window_surface;
cairo_t *window_cr;

// image as a cairo surface
cairo_surface_t *image_surface;

// graphics context used for Xlib
GC gc;
bool gc_init = false;

// transparency stuff
Pixmap alpha_pixmap;
bool alpha_pixmap_init = false;
XImage *alpha_ximage = NULL;
GC alpha_gc;
bool alpha_gc_init = false;

// booleans
bool exited = false;
bool exiting = false;
bool running = false;

// signals that we receive
bool received_resize = false;
bool received_reload = false;

// clean up and exit function
void die(int exitcode, ...) {
	exiting = true;
	running = false;

	if (!exited) {
		exited = true;

		// print error message
		va_list args;
		bool first = true;
		char *arg;
		va_start(args, exitcode);

		while (true) {
			arg = va_arg(args, char*);
			if (!arg) break;

			// separate string args with ": ", e.g die(5, "hello", "world", NULL) will print "hello: world" and exit code 5
			if (!first) fputs(": ", stderr);
			fputs(arg, stderr);
			first = false;
		}
		if (!first) fputs("\n", stderr);

		va_end(args);

		// tidy everything up

		if (image_surface) { cairo_surface_destroy(image_surface); image_surface = NULL; }

		if (window_cr) { cairo_destroy(window_cr); window_cr = NULL; }
		if (window_surface) { cairo_surface_destroy(window_surface); window_surface = NULL; }

		if (alpha_gc_init) { XFreeGC(dpy, alpha_gc); alpha_gc_init = false; }
		if (alpha_ximage) { XDestroyImage(alpha_ximage); alpha_ximage = NULL; }
		if (alpha_pixmap_init) { XFreePixmap(dpy, alpha_pixmap); alpha_pixmap_init = false; }

		if (window_pixmap_init) { XFreePixmap(dpy, window_pixmap); window_pixmap_init = false; }

		if (gc_init) { XFreeGC(dpy, gc); gc_init = false; }
		
		if (image_init) { ilDeleteImages(1, &image); image_init = false; }

		if (class_hint) { XFree(class_hint); class_hint = NULL; }
		if (size_hint) { XFree(size_hint); size_hint = NULL; }
		if (win_init) { XDestroyWindow(dpy, win); win_init = false; }
		if (dpy) { XCloseDisplay(dpy); dpy = NULL; }

		if (il_init) { ilShutDown(); il_init = false; }
	}
	exit(exitcode);
}

// handler for exit signals
void exit_handler() {
	if (!exiting && !running) {
		die(3, NULL);
		return;
	}
	exiting = true;
}

void null_handler() {}

// handles SIGUSR1
void sigusr1_handler() {
	received_resize = true;
}

// handles SIGUSR2
void sigusr2_handler() {
	received_reload = true;
}

#define HELP_TEXT "Usage: "EXEC" <file>\n\
	-h, --help               : Shows this help text\n\
	-V, --version            : Shows the current version\n\
	-t, --title title        : Sets the window title\n\
	-c, --class class        : Sets the window class name\n\
	-p, --pos x,y            : Sets the window position\n\
	-s, --size w,h           : Sets the window size, defaults to the size of the image\n\
	-b, --bg r,g,b           : Sets the background colour (0-255)\n\
	-r, --hotreload          : Reloads image when it is modified, will not work with stdin\n\
	-B, --borderless         : Removes the border from the window\n\
	-u, --transparent        : Makes the transparency of the window match the image\n"

// checks if all flags in flag are in possible_flags exactly once
// returns false if an unknown flag appears or a flag appears more than once
// if you know a better way to do this please make an issue
bool argch(char *flag, char *possible_flags) {
	int len = strlen(possible_flags);
	char *_possible_flags = malloc(len); // create a backup of it so we can edit it
	if (!_possible_flags) die(errno, "malloc", strerror(errno), NULL);
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

// creates pixmaps and cairo surfaces for drawing the window
void create_window_drawing(struct vector2 window_size, unsigned int depth, bool transparent) {
	// destroy/free stuff we have already initialised

	if (window_cr) { cairo_destroy(window_cr); window_cr = NULL; }
	if (window_surface) { cairo_surface_destroy(window_surface); window_surface = NULL; }

	if (window_pixmap_init) {
		XFreePixmap(dpy, window_pixmap);
		window_pixmap_init = false;
	}

	if (gc_init) {
		XFreeGC(dpy, gc);
		gc_init = false;
	}

	// initialise stuff

	window_pixmap = XCreatePixmap(dpy, win, window_size.x, window_size.y, depth);
	if (!window_pixmap) die(1, "Failed to create window pixmap", NULL);
	window_pixmap_init = true;

	gc = XCreateGC(dpy, window_pixmap, 0, NULL);
	gc_init = true;
	
	window_surface = cairo_xlib_surface_create(dpy, window_pixmap, visual, window_size.x, window_size.y);
	if (!window_surface) die(1, "Failed to create window surface", NULL);

	window_cr = cairo_create(window_surface);
	if (!window_cr) die(1, "Failed to create window context", NULL);

	// only for transparency
	if (transparent) {
		if (alpha_gc_init) {
			XFreeGC(dpy, alpha_gc);
			alpha_gc_init = false;
		}

		if (alpha_pixmap_init) {
			XFreePixmap(dpy, alpha_pixmap);
			alpha_pixmap_init = false;
		}

		if (alpha_ximage) {
			XDestroyImage(alpha_ximage);
			alpha_ximage = NULL;
		}

		alpha_pixmap = XCreatePixmap(dpy, win, window_size.x, window_size.y, 1);
		if (!alpha_pixmap) die(1, "Failed to create alpha pixmap", NULL);
		alpha_pixmap_init = true;

		alpha_ximage = XGetImage(dpy, alpha_pixmap, 0, 0, window_size.x, window_size.y, AllPlanes, ZPixmap);

		alpha_gc = XCreateGC(dpy, alpha_pixmap, 0, NULL);
		if (!alpha_gc) die(1, "Failed to create alpha GC", NULL);
	}
}

// creates cairo surface for the image
void create_image_surface(struct vector2 image_size) {
	if (image_surface) { cairo_surface_destroy(image_surface); image_surface = NULL; }

	image_surface = cairo_image_surface_create_for_data(image_data, CAIRO_FORMAT_ARGB32, image_size.x, image_size.y,
			cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, image_size.x));
	if (!image_surface) die(1, "Failed to create image surface", NULL);
}

struct render_args {
	struct transform transform;
	struct transform *old_transform;
	struct vector2 image_size;
	struct vector2 window_size;
	struct color bg;
	bool transparent;
};

// render/expose function
void render(struct render_args args) {
	// if transformation is different
	if (!transformcmp(*args.old_transform, args.transform)) {
		// set transformation
		*args.old_transform = args.transform;

		// clear surface
		cairo_set_source_rgb(window_cr, (double)args.bg.r/255, (double)args.bg.g/255, (double)args.bg.b/255);
		cairo_paint(window_cr);

		// transformation
		cairo_translate(window_cr, args.transform.translate.x, args.transform.translate.y);
		cairo_scale(window_cr, args.transform.scale, args.transform.scale);

		// draw image
		cairo_set_source_surface(window_cr, image_surface, 0, 0);
		cairo_paint(window_cr);

		// undo transformation
		cairo_scale(window_cr, args.transform.scale_inv, args.transform.scale_inv);
		cairo_translate(window_cr, -args.transform.translate.x, -args.transform.translate.y);

		// flush
		cairo_surface_flush(window_surface);

		if (args.transparent) {
			bool is_visible;
			struct vector2 image_pos;
			for (double y = 0; y < args.window_size.y; ++y) {
				for (double x = 0; x < args.window_size.x; ++x) {
					if (x <  args.transform.translate.x                    || y <  args.transform.translate.y ||
						x >= args.transform.translate.x + args.transform.size.x || y >= args.transform.translate.y + args.transform.size.y) is_visible = false;
					else {
						image_pos.x = (int)((x - args.transform.translate.x) * args.transform.scale_inv + 0.5);
						image_pos.y = (int)((y - args.transform.translate.y) * args.transform.scale_inv + 0.5);
						is_visible = image_data[(image_pos.y * args.image_size.x + image_pos.x) * image_bpp + 3] & 0x80;
					}
					XPutPixel(alpha_ximage, x, y, is_visible ? 1 : 0);
				}
			}
			XPutImage(dpy, alpha_pixmap, alpha_gc, alpha_ximage, 0, 0, 0, 0, args.window_size.x, args.window_size.y);
		}
	}

	// set transparent shape
	if (args.transparent) XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, alpha_pixmap, ShapeSet);

	// set pixmap to window
	XCopyArea(dpy, window_pixmap, win, gc, 0, 0, args.window_size.x, args.window_size.y, 0, 0);

	// set window background
	XSetWindowBackgroundPixmap(dpy, win, window_pixmap);

	// flush display
	XFlush(dpy);
}

int main(int argc, char *argv[]) {
	// macros for argument handling
#define invalid return eprintf("Invalid usage, see --help\n"), 2
	// if the argument matches our option
#define argcmp(flag, shortflag, longflag) \
		(strcmp(flag, longflag) == 0 || (flag[1] != '-' && strchr(flag+1, shortflag) != NULL))
	// set a flag if argument matches our option, fail if already set
#define argflag(shortflag, longflag, set, set2) \
	int set2 = 0; \
	if (argcmp(argv[i], shortflag, longflag)) { \
		if (set) invalid; \
		set2 = 1; \
		set = 1; \
	}
	// set an option if argument matches our option, fail if already set
#define argoption(shortflag, longflag, failcondition, set) \
	if (argcmp(argv[i], shortflag, longflag)) { \
		if (failcondition) invalid; \
		set = 1; \
	}

	// return help text if ran with no arguments
	if (argc <= 1) {
		fputs(HELP_TEXT, stderr);
		return 2;
	}

	// flags
	bool flag_done = false, flag_hotreload = false, flag_borderless = false, flag_transparent = false,
		 flag_sigusr1 = false, flag_sigusr2 = false;
	// options
	char *file = NULL, *title = NULL, *class = NULL;
	struct vector2 window_pos;
	struct render_args render_args;
	struct color bg = { 0, 0, 0 };
	// if an option has been set
	bool set_pos = false, set_size = false, set_bg = false;

	{
	// flags for if we are about to set an option
	char flag_set_title = 0, flag_set_class = 0, flag_set_pos = 0, flag_set_size = 0, flag_set_bg = 0;
	char *str_x = NULL, *str_y = NULL, *str_w = NULL, *str_h = NULL;

	for (int i = 1; i < argc; ++i) {
		// if argument starts with - and has characters after that
		if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') {
				// if argument is --
				flag_done = 1; // finish flags/options
			} else {
				// if we try to put a flag/option before finishing the last option
				if (flag_set_title || flag_set_class || flag_set_pos || flag_set_size || flag_set_bg) invalid;

				if (argv[i][1] != '-' && !argch(argv[i]+1, "hVtcpsbrBu12")) invalid;

				if (argcmp(argv[i], 'h', "--help")) {
					fputs(HELP_TEXT, stdout);
					return 0;
				}
				if (argcmp(argv[i], 'V', "--version")) {
					printf("%s %s\n", EXEC, VERSION);
					return 0;
				}
				argoption('t', "--title", title, flag_set_title);
				argoption('c', "--class", class, flag_set_class);
				argoption('p', "--pos", str_x || str_y, flag_set_pos);
				argoption('s', "--size", str_w || str_h, flag_set_size);
				argoption('b', "--bg", set_bg, flag_set_bg);
				argflag('r', "--hotreload", flag_hotreload, flag_set_hotreload);
				argflag('B', "--borderless", flag_borderless, flag_set_borderless);
				argflag('u', "--transparent", flag_transparent, flag_set_transparent);
				argflag('1', "--sigusr1", flag_sigusr1, flag_set_sigusr1);
				argflag('2', "--sigusr2", flag_sigusr2, flag_set_sigusr2);

				// prevent setting multiple options at once or invalid flag
				char total = flag_set_title + flag_set_class + flag_set_pos + flag_set_size + flag_set_bg;
				if (total > 1 || (total == 0 && !(flag_set_hotreload || flag_set_borderless || flag_set_transparent || flag_set_sigusr1 || flag_set_sigusr2)))
					invalid;
			}
		} else if (flag_set_title) {
			flag_set_title = 0;
			title = argv[i]; // set title option
		} else if (flag_set_class) {
			flag_set_class = 0;
			class = argv[i]; // set class option
		} else if (flag_set_pos) {
			set_pos = true;
			flag_set_pos = 0;
			if (!multi_to_int(argv[i], true, false, &window_pos.x, &window_pos.y, NULL)) invalid;
		} else if (flag_set_size) {
			set_size = true;
			flag_set_size = 0;
			if (!multi_to_int(argv[i], false, false, &render_args.window_size.x, &render_args.window_size.y, NULL)) invalid;
		} else if (flag_set_bg) {
			set_bg = true;
			flag_set_bg = 0;
			int r, g, b;
			if (!multi_to_int(argv[i], true, true, &r, &g, &b, NULL)) invalid;
			bg.r = r;
			bg.g = g;
			bg.b = b;
		} else if (!file) {
			file = argv[i];
		} else {
			invalid;
		}
	}

	if (flag_set_title || flag_set_class || flag_set_pos || flag_set_size || flag_set_bg) invalid; // if we didn't finish an option
	if (!file) invalid; // if file wasn't specified
	}

	// handle all signals
	signal(SIGHUP, exit_handler);
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGUSR1, flag_sigusr1 ? sigusr1_handler : null_handler);
	signal(SIGUSR2, flag_sigusr2 ? sigusr2_handler : null_handler);
	signal(SIGTERM, exit_handler);

	// initialise DevIL
	ilInit();
	il_init = true;

	char *filename = NULL;
	bool is_stdin;
	image_init = false;
	// read the image
	image_init = readimage(file, &is_stdin, &filename, BLOCK, &image_data, &image, &render_args.image_size, &image_bpp, true, bg);
	if (!image_init) die(1, "Failed to read data", NULL);

	struct stat st;
	time_t prev_mtime = 0;
	if (flag_hotreload) {
		if (is_stdin) die(1, "Cannot use hotreload with stdin", NULL);
		if (stat(file, &st) != 0) die(1, file, strerror(errno), NULL);
		prev_mtime = st.st_mtime;
	}

	if (!title) {
		size_t size = strlen(EXEC) + strlen(filename) + 16;
		title = malloc(size);
		if (!title) die(errno, "malloc", strerror(errno), NULL);
		snprintf(title, size, "%s: %s", EXEC, filename);
	}
	if (!class) class = EXEC;

	if (!set_size) {
		// image size by default
		render_args.window_size.x = render_args.image_size.x,
		render_args.window_size.y = render_args.image_size.y;
	}

	// some WMs/compositors fail to resize the window if it's too small
	if (render_args.window_size.x < 10 || render_args.window_size.y < 10) {
		if (render_args.window_size.x < 10) render_args.window_size.x = 10;
		if (render_args.window_size.y < 10) render_args.window_size.y = 10;
		eprintf("Window size too small\n");
	}

	// open X display
	dpy = XOpenDisplay(NULL);
	if (!dpy) die(1, "Cannot open display", NULL);

	// get a bunch of stuff
	int screen = DefaultScreen(dpy);
	Window root = DefaultRootWindow(dpy);
	visual = DefaultVisual(dpy, screen);
	unsigned int depth = DefaultDepth(dpy, screen);

	// main window
	win = XCreateSimpleWindow(dpy, root, 0, 0, render_args.window_size.x, render_args.window_size.y, 0,
			BlackPixel(dpy, screen), WhitePixel(dpy, screen));
	if (!win) die(1, "Cannot create window", NULL);

	// window attributes
	XSetWindowAttributes attrs;
	unsigned long attrs_flags = CWBackPixel;
	attrs.background_pixel = color_to_long(bg);
	if (flag_borderless) {
		attrs.override_redirect = True;
		attrs_flags |= CWOverrideRedirect;
	}
	XChangeWindowAttributes(dpy, win, attrs_flags, &attrs);

	// we want to handle close window event
	Atom wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);

	// we want to handle some events
	XSelectInput(dpy, win, ExposureMask | StructureNotifyMask | SubstructureNotifyMask);

	// set window title
	XStoreName(dpy, win, title);

	// set class
	class_hint = XAllocClassHint();
	if (!class_hint) die(1, "Failed to allocate class hint", NULL);
	XGetClassHint(dpy, win, class_hint);
	class_hint->res_name = class;
	class_hint->res_class = class;
	XSetClassHint(dpy, win, class_hint);

	// makes class hint work better somehow
	size_hint = XAllocSizeHints();
	if (!size_hint) die(1, "Failed to allocate size hint", NULL);
	XSetWMNormalHints(dpy, win, size_hint);

	// show window
	XMapWindow(dpy, win);

	win_init = true;

	// create image surface and window stuff
	create_image_surface(render_args.image_size);
	create_window_drawing(render_args.window_size, depth, flag_transparent);

	// move window to the position
	if (set_pos) XMoveWindow(dpy, win, window_pos.x, window_pos.y);

	// booleans
	bool resize = true, hotreload = false;
	struct transform old_transform;
	render_args.old_transform = &old_transform;
	
	// event we are handling
    XEvent event;

	// image transformation so it can fit to the window size
	letterboxing(render_args.window_size, render_args.image_size, &render_args.transform);

	// for hotreloading
	unsigned long long file_last_checked = 0;
	unsigned long long file_checked = 0;

	received_resize = received_reload = false;
	running = true;
	while (running && !exiting) {
		if (received_resize) {
			// resize window to the image size
			received_resize = false;
			render_args.window_size.x = render_args.image_size.x,
			render_args.window_size.y = render_args.image_size.y;
			XResizeWindow(dpy, win, render_args.window_size.x, render_args.window_size.y);
			create_window_drawing(render_args.window_size, depth, flag_transparent);
			letterboxing(render_args.window_size, render_args.image_size, &render_args.transform);
			render(render_args);
		}
		if (received_reload) {
			// force reload image
			received_reload = false;
			file_checked = get_time();
			hotreload = true;
		}
		if (!is_stdin && flag_hotreload && running && !hotreload) {
			file_checked = get_time();
			// delay after file was last checked
			if (file_checked > file_last_checked + FILE_CHECK_TIME || file_checked < file_last_checked) {
				file_last_checked = file_checked;
				if (stat(file, &st) != 0) die(1, file, strerror(errno), NULL);
				if (st.st_mtime != prev_mtime) {
					prev_mtime = st.st_mtime;
					hotreload = true;
				}
			}
		}
		if (hotreload) {
			// TODO: wait until image is done loading
			received_reload = false;
			hotreload = false;
			ilDeleteImages(1, &image);
			image_init = false;
			image_init = readimage(file, &is_stdin, &filename, BLOCK, &image_data, &image, &render_args.image_size, &image_bpp, true, bg);
			if (!image_init) die(1, "Failed to read data", NULL);
			create_image_surface(render_args.image_size);
			letterboxing(render_args.window_size, render_args.image_size, &render_args.transform);
			render(render_args);
		}
		while (running && XPending(dpy)) {
			XNextEvent(dpy, &event);
			switch (event.type) {
				case ConfigureNotify:
					// window resize
					if (resize) {
						resize = false;
						if (set_pos) XMoveWindow(dpy, win, window_pos.x, window_pos.y);
					}
					// don't recreate if size hasn't changed
					if (event.xconfigure.width == render_args.window_size.x && event.xconfigure.height == render_args.window_size.y) break;
					render_args.window_size.x = event.xconfigure.width;
					render_args.window_size.y = event.xconfigure.height;
					create_window_drawing(render_args.window_size, depth, flag_transparent);
					letterboxing(render_args.window_size, render_args.image_size, &render_args.transform);
					// fall to render window
				case Expose:
					// render window
					render(render_args);
					break;
				case ClientMessage:
					// close window
					if (event.xclient.message_type == wm_protocols &&
							event.xclient.data.l[0] == wm_delete_window) {
						running = false;
					}
					break;
			}
		}
		if (exiting) break;
    }
	die(0, NULL);
	return 0;
}
