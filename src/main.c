#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <err.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "./util.h"
#include "./arg.h"
#include "./image.h"

// long options with getopt
static struct option options_getopt[] = {
        {"help",       no_argument,       0, 'h'},
        {"version",    no_argument,       0, 'V'},
        {"title",      required_argument, 0, 't'},
        {"position",   required_argument, 0, 'p'},
        {"size",       required_argument, 0, 's'},
        {"background", required_argument, 0, 'b'},
        {"stretch",    no_argument,       0, 'S'},
        {"hotreload",  no_argument,       0, 'r'},
        {"sigusr1",    no_argument,       0, '1'},
        {"usr1",       no_argument,       0, '1'},
        {"sigusr2",    no_argument,       0, '2'},
        {"usr2",       no_argument,       0, '2'},
        {"terminal",   no_argument,       0, 'T'},
        {"term",       no_argument,       0, 'T'},
        {"unicode",    no_argument,       0, 'u'},
        {"nounicode",  no_argument,       0, 'U'},
        {"no-unicode", no_argument,       0, 'U'},
        {"4bit",       no_argument,       0, '4'},
        {"8bit",       no_argument,       0, '8'},
        {"24bit",      no_argument,       0, '6'},
        {0,            0,                 0, 0  }
};

bool sigusr1 = false, sigusr2 = false, sigwinch = false;

void sigusr1_handler() {
	sigusr1 = true;
}

void sigusr2_handler() {
	sigusr2 = true;
}

void sigwinch_handler() {
	sigwinch = true;
}

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *term_surface = NULL;
SDL_Surface *surface = NULL;
SDL_Texture *texture = NULL;
char *title_default = NULL;
bool sdl_init = false, sdl_image_init = false;

struct term_cursor {
	unsigned int x, y, width, height, scaled_width, scaled_height;
};

bool fetch_term_size(struct term_cursor *cursor) {
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		warn("ioctl");
		return false;
	}
	if (cursor) {
		cursor->width = w.ws_col;
		cursor->height = w.ws_row;
	}
	return true;
}

void cleanup() {
	printf("Cleaning up\n");
	if (texture) SDL_DestroyTexture(texture);
	if (renderer) SDL_DestroyRenderer(renderer);
	if (window) SDL_DestroyWindow(window);
	if (term_surface) SDL_FreeSurface(term_surface);
	if (surface) SDL_FreeSurface(surface);
	if (sdl_image_init) IMG_Quit();
	if (sdl_init) SDL_Quit();
	if (title_default) free(title_default);
	texture = NULL;
	renderer = NULL;
	window = NULL;
	term_surface = NULL;
	surface = NULL;
	sdl_image_init = false;
	sdl_init = false;
	title_default = NULL;
}

int main(int argc, char *argv[]) {
	// arguments
	struct {
		char *title;
		bool stretch, hot_reload, sigusr1, sigusr2, position_set, size_set, background_set, terminal;
		SDL_Point position, size;
		SDL_Color background;
		enum {
			BIT_AUTO = 0,
			BIT_4,
			BIT_8,
			BIT_24
		} bit_depth;
		enum toggle_mode {
			TOGGLE_AUTO = 0,
			TOGGLE_OFF,
			TOGGLE_ON
		} unicode;
	} options = {0}; // all false/NULL/0

	bool invalid = false; // don't immediately exit when invalid argument, so we can still use --help
	int opt;
	long nums[3];

	// argument handling
	while ((opt = getopt_long(argc, argv, ":hVt:c:p:s:b:Sr12TuU486", options_getopt, NULL)) != -1) {
		if (opt == 'h') {
			// help text
			printf("Usage: %s [options_getopt] file\n\
\n\
-h --help: Shows help text\n\
-V --version: Shows the current version\n\
\n\
-t --title [title]: Sets the window/terminal title\n\
-p --position [x],[y]: Sets the window position\n\
-s --size [w],[h]: Sets the window size, defaults to the size of the image\n\
-b --background [r],[g],[b]: Sets the background colour (0-255), defaults to black or terminal background\n\
-S --stretch: Allows the image to stretch instead of fitting to the window\n\
\n\
-r --hotreload: Reloads image when it is modified, will not work with stdin\n\
-1 --usr1 --sigusr1: Allows the SIGUSR1 signal to resize the window to the size of the image, incompatible with -T\n\
-2 --usr2 --sigusr2: Allows the SIGUSR2 signal to reload the image on demand\n\
\n\
-T --term --terminal: Shows the image in the terminal instead of on screen\n\
	Highly experimental! Not functional yet\n\
	Uses software renderer, which may be slower\n\
	-p and -s will instead specify the image bounds on the terminal, these cannot be set separately\n\
\n\
-u --unicode: Force enable unicode support for -T\n\
-U --no-unicode: Disables unicode support for -T\n\
-4 --4bit: Force 4-bit colour depth (0-15)\n\
-8 --8bit: Force 8-bit colour depth (16-255)\n\
-6 --24bit: Force 24-bit colour depth (true color)\n\
	Defaults to whatever the terminal supports\n\
\n\
",
			       PROJECT_NAME);

			return 0;
		} else if (opt == 'V') {
			printf("%s %s\n", PROJECT_NAME, PROJECT_VERSION);
#ifdef PROJECT_URL
			printf("See more at %s\n", PROJECT_URL);
#endif
			return 0;
		} else if (!invalid) {
			switch (opt) {
				case 't':
					if (options.title) invalid = true;
					else
						options.title = optarg;
					break;
				case 'p':
					if (options.position_set) break;
					if (parse_num_array(optarg, nums, 2))
						if (to_point(nums[0], nums[1], &options.position)) {
							options.position_set = true;
							break;
						}
					invalid = true;
					break;
				case 's':
					if (options.size_set) break;
					if (parse_num_array(optarg, nums, 2))
						if (to_point(nums[0], nums[1], &options.size)) {
							options.size_set = true;
							break;
						}
					invalid = true;
					break;
				case 'b':
					if (options.background_set) break;
					if (parse_num_array(optarg, nums, 3))
						if (to_color(nums[0], nums[1], nums[2], &options.background)) {
							options.background_set = true;
							break;
						}
					invalid = true;
					break;
				case 'S':
					if (options.stretch) invalid = true;
					options.stretch = true;
					break;
				case 'r':
					if (options.hot_reload) invalid = true;
					options.hot_reload = true;
					break;
				case '1':
					if (options.sigusr1) invalid = true;
					options.sigusr1 = true;
					break;
				case '2':
					if (options.sigusr2) invalid = true;
					options.sigusr2 = true;
					break;
				case 'T':
					if (options.terminal) invalid = true;
					options.terminal = true;
					break;
				case 'u':
				case 'U':
					if (options.unicode != TOGGLE_AUTO) invalid = true;
					options.unicode = opt == 'u' ? TOGGLE_ON : TOGGLE_OFF;
					break;
				case '4':
				case '8':
				case '6':
					if (options.bit_depth != BIT_AUTO) invalid = true;
					options.bit_depth = opt == '4' ? BIT_4 : opt == '8' ? BIT_8
					                                                    : BIT_24;
					break;
				default:
					invalid = true;
					break;
			}
		}
	}

	if (optind != argc - 1 || invalid || !argv[optind][0]) {
		eprintf("Invalid usage, try --help\n");
		return 1;
	}

	if (options.terminal) {
		if (options.sigusr1) {
			eprintf("Cannot use SIGUSR1 in terminal mode, ignoring...\n");
			options.sigusr1 = false;
		}
		if (options.position_set != options.size_set) {
			eprintf("Cannot set position or size seperately in terminal mode, ignoring...\n");
			options.position_set = options.size_set = false;
		}
		if ((options.position_set || options.size_set) && options.stretch) {
			eprintf("Cannot set position or size, while stretch is enabled in terminal mode, ignoring stretch...\n");
			options.stretch = false;
		}
	} else if (options.unicode != TOGGLE_AUTO) {
		eprintf("Cannot specify unicode support without terminal mode, ignoring...\n");
		options.unicode = TOGGLE_AUTO;
	}

	char *filename = argv[optind];

	atexit(cleanup);

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		eprintf("Failed to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	sdl_init = true;

	if (!(IMG_Init(-1))) {
		// if it fails to load every image type
		// not any image type, because we don't want to require libavif if the image is png for example
		eprintf("Failed to initialize SDL image: %s\n", IMG_GetError());
		return 1;
	}

	sdl_image_init = true;

	FILE *fp = open_file(filename);
	if (!fp) return 1;

	if (fp == stdin && options.hot_reload) {
		eprintf("Cannot hot-reload with stdin, ignoring...\n");
		options.hot_reload = false;
	}

	surface = read_file(fp);
	close_file(fp);
	if (!surface) return 1;

	if (!options.title && !options.terminal) {
		// if title isn't set, set it to the last component of filename, except in terminal mode
		options.title = strrchr(filename, '/');
		if (!options.title) options.title = filename;
		else
			++options.title;

		title_default = malloc(strlen(options.title) + 16);
		if (!title_default) err(1, "malloc");

		sprintf(title_default, "%s - %s", options.title, PROJECT_NAME);
		options.title = title_default;
	}

	if (!options.terminal) {
		// create window
		window = SDL_CreateWindow(
		        options.title,
		        options.position_set ? options.position.x : (int) SDL_WINDOWPOS_UNDEFINED,
		        options.position_set ? options.position.y : (int) SDL_WINDOWPOS_UNDEFINED,
		        options.size_set ? options.size.x : surface->w,
		        options.size_set ? options.size.y : surface->h,
		        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

		if (options.position_set) {
			// set window position
			SDL_SetWindowPosition(window, options.position.x, options.position.y);
		}

		if (!window) {
			eprintf("Failed to create window: %s\n", SDL_GetError());
			return 1;
		}

		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

		if (!renderer) {
			eprintf("Failed to create renderer: %s\n", SDL_GetError());
			return 1;
		}
	} else if (options.title) {
		// print title
		printf("\033]0;%s\007", options.title);
	}

	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	// set up signal handlers
	sa.sa_handler = sigusr1_handler;
	if (sigaction(SIGUSR1, &sa, NULL) == -1) err(1, "sigaction");
	sa.sa_handler = sigusr2_handler;
	if (sigaction(SIGUSR2, &sa, NULL) == -1) err(1, "sigaction");
	sa.sa_handler = sigwinch_handler;
	if (sigaction(SIGWINCH, &sa, NULL) == -1) err(1, "sigaction");

	// variables for hot-reload
	struct stat st;                      // stat
	time_t prev_mtime = 0;               // previous modification date
	unsigned long long last_checked = 0; // the time at when we have last checked

	// terminal mode
	struct term_cursor cursor;
	bool term_should_render = true;

	// main loop
	bool running = true, should_reload;
	while (running) {
		// from signal handler
		if (window && sigusr1 && options.sigusr1) {
			// resize the window to the size of the image
			sigusr1 = false;
			SDL_SetWindowSize(window, surface->w, surface->h);
		}

		// from signal handler
		should_reload = sigusr2 && options.sigusr2;

		if (!should_reload && options.hot_reload) {
			// get current time
			unsigned long long current_time = get_time();

			// if a second has passed since we last checked this
			if (current_time > last_checked + 1000) {
				// set time last checked
				last_checked = current_time;

				// stat the file
				if (stat(filename, &st) != 0) err(1, "hot-reload: %s", filename);

				// reload if modification date has changed
				if (st.st_mtime != prev_mtime) {
					prev_mtime = st.st_mtime;
					should_reload = true;
				}
			}
		}

		if (should_reload) {
			term_should_render = true;
			sigusr2 = false;

			// reload the image
			fp = open_file(filename);
			if (!fp) return 1;
			SDL_Surface *new_surface = read_file(fp);
			close_file(fp);

			// TODO: load new image in a separate thread and switch to it when done
			if (new_surface) {
				// destroy the old texture and surface
				if (texture) SDL_DestroyTexture(texture);
				texture = NULL;

				if (surface) SDL_FreeSurface(surface);
				surface = new_surface;
			} else {
				eprintf("Failed to load updated image\n");
			}
		}

		if ((sigwinch || !renderer) && options.terminal) {
			sigwinch = false;
			struct term_cursor old_cursor = cursor;
			if (!fetch_term_size(&cursor)) {
				eprintf("Failed to get terminal size\n");
				return 1;
			}
			// if size has changed
			if (!renderer || old_cursor.width != cursor.width || old_cursor.height != cursor.height) {
				term_should_render = true;
				options.unicode = TOGGLE_ON; // TODO: check if terminal supports unicode and set if auto

				// dsetroy the renderer and surface
				if (renderer) SDL_DestroyRenderer(renderer);
				renderer = NULL;
				if (term_surface) SDL_FreeSurface(term_surface);

				cursor.scaled_width = cursor.width;
				cursor.scaled_height = cursor.height * (options.unicode == TOGGLE_ON ? 2 : 1);

				term_surface = SDL_CreateRGBSurface(0, cursor.scaled_width, cursor.scaled_height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
				if (!term_surface) {
					eprintf("Failed to create terminal surface: %s\n", SDL_GetError());
					return 1;
				}

				renderer = SDL_CreateSoftwareRenderer(term_surface);
				if (!renderer) {
					eprintf("Failed to create software renderer: %s\n", SDL_GetError());
					return 1;
				}
			}
		}

		// set background color
		if (!options.terminal || options.background_set)
			SDL_SetRenderDrawColor(renderer, options.background.r, options.background.g, options.background.b, 255);
		SDL_RenderClear(renderer);

		// get the size of the window
		SDL_Point window_size;
		if (window)
			SDL_GetWindowSize(window, &window_size.x, &window_size.y);
		else if (options.terminal)
			window_size = (SDL_Point){cursor.scaled_width, cursor.scaled_height};
		else {
			eprintf("Window is NULL\n");
			return 1;
		}

		// find the right scaling mode to fit the image in the window
		SDL_Rect rect;
		if (options.terminal && options.position_set) {
			// set exact bounds of image in terminal
		} else if (options.stretch) {
			// stretch image to window/terminal size
			rect = (SDL_Rect){.x = 0, .y = 0, .w = window_size.x, .h = window_size.y};
		} else {
			// fit image to window/terminal size
			rect = get_fit_mode((SDL_Point){surface->w, surface->h}, window_size);
		}

		if (!texture) {
			texture = SDL_CreateTextureFromSurface(renderer, surface);
			if (!texture) {
				eprintf("Failed to create texture: %s\n", SDL_GetError());
				return 1;
			}
		}

		// draw the image with the rectangle
		SDL_RenderCopy(renderer, texture, NULL, &rect);

		SDL_RenderPresent(renderer);

		if (term_should_render && options.terminal) {
			term_should_render = false;
			// TODO: draw the surface to the terminal
			printf("render\n");
			// testing
			/*
			if (IMG_SavePNG(term_surface, "test.png") != 0) {
				eprintf("Failed to save image: %s\n", IMG_GetError());
				return 1;
			}
			*/
			/*
			4-bit: 16 colors
			8-bit: 16-231: R=(n-16)/36, G=((n-16)%36)/6, B=(n-16)%6
			24-bit: 232-255: RGB=(n-232)*10+8
			*/
		}

		SDL_Event event;
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT) {
				running = false;
			}
		}
	}

	return 0;
}
