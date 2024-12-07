#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <err.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ncurses.h>
#include <term.h>
#undef buttons // conflicts with SDL

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "util.h"
#include "arg.h"
#include "image.h"
#include "term.h"

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

bool sigusr1 = false, sigusr2 = false;

void sigusr1_handler() {
	sigusr1 = true;
}

void sigusr2_handler() {
	sigusr2 = true;
}

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *term_surface = NULL;
SDL_Surface *surface = NULL;
SDL_Texture *texture = NULL;
char *title_default = NULL;

bool fetch_term_size(struct position *size) {
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		warn("ioctl");
		return false;
	}
	if (size) {
		size->x = w.ws_col;
		size->y = w.ws_row;
	}
	return true;
}

bool sdl_init = false, sdl_image_init = false, term_init = false;

// arguments
struct {
	char *title;
	bool stretch, hot_reload, sigusr1, sigusr2, position_set, size_set, background_set, terminal;
	SDL_Point position, size;
	SDL_Color background;
	enum bit_depth bit_depth;
	enum toggle_mode {
		TOGGLE_AUTO = 0,
		TOGGLE_OFF,
		TOGGLE_ON
	} unicode;
} options = {0}; // all false/NULL/0

void cleanup() {
	if (term_init) {
		term_init = false;
	}
	printf("\n");
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

bool should_reload = true;

static bool should_continue() {
	return !(sigusr1 || sigusr2 || should_reload);
}

int main(int argc, char *argv[]) {

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
	-p and -s will instead specify the image bounds on the terminal, -p cannot be set without -s\n\
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
		if (options.position_set && !options.size_set) {
			eprintf("Cannot set position without size being set in terminal mode, ignoring...\n");
			options.position_set = options.size_set = false;
		}
	} else if (options.unicode != TOGGLE_AUTO) {
		eprintf("Cannot specify unicode support without terminal mode, ignoring...\n");
		options.unicode = TOGGLE_AUTO;
	} else if (options.bit_depth != BIT_AUTO) {
		eprintf("Cannot specify bit depth without terminal mode, ignoring...\n");
		options.bit_depth = BIT_AUTO;
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
	}

	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	// set up signal handlers
	sa.sa_handler = sigusr1_handler;
	if (sigaction(SIGUSR1, &sa, NULL) == -1) err(1, "sigaction");
	sa.sa_handler = sigusr2_handler;
	if (sigaction(SIGUSR2, &sa, NULL) == -1) err(1, "sigaction");

	// variables for hot-reload
	struct stat st;                      // stat
	time_t prev_mtime = 0;               // previous modification date
	unsigned long long last_checked = 0; // the time at when we have last checked

	// terminal mode
	struct position term_size;
	bool term_should_render = true;

	// main loop
	bool running = true;
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

		if (options.terminal) {
			if (!term_init) {
				if (setupterm(NULL, STDOUT_FILENO, NULL) != OK) {
					eprintf("Failed to set up terminal\n");
					return 1;
				}

				if (options.title) printf("\x1b]0;%s\007", options.title); // print title

				int colors = tigetnum("colors");
				if (colors < 256) {
					if (options.bit_depth == BIT_AUTO) options.bit_depth = BIT_4;
					if (options.unicode == TOGGLE_AUTO) options.unicode = TOGGLE_OFF; // unicode is probably not supported, and it won't look good with 4-bit color
				} else if (options.bit_depth == BIT_AUTO) {
					// https://github.com/dankamongmen/notcurses/blob/master/src/lib/termdesc.c
					bool rgb = (tigetflag("RGB") > 0 || tigetflag("Tc") > 0);
					if (!rgb) {
						const char *cterm = getenv("COLORTERM");
						rgb = cterm && (strcmp(cterm, "truecolor") == 0 || strcmp(cterm, "24bit") == 0);
					}
					options.bit_depth = rgb ? BIT_24 : BIT_8;
				}
				if (options.unicode == TOGGLE_AUTO) options.unicode = TOGGLE_ON;
				term_init = true;
			}

			struct position old_size = term_size;
			if (!fetch_term_size(&term_size)) {
				eprintf("Failed to get terminal size\n");
				return 1;
			}

			// if size has changed
			if (!renderer || old_size.x != term_size.x || old_size.y != term_size.y) {
				term_should_render = true;

				// dsetroy the texture, renderer and surface
				if (texture) SDL_DestroyTexture(texture);
				texture = NULL;
				if (renderer) SDL_DestroyRenderer(renderer);
				renderer = NULL;
				if (term_surface) SDL_FreeSurface(term_surface);

				term_surface = SDL_CreateRGBSurface(0, term_size.x, term_size.y * (options.unicode == TOGGLE_ON ? 2 : 1), TERM_DEPTH, TERM_R_MASK, TERM_G_MASK, TERM_B_MASK, TERM_A_MASK);
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
		SDL_SetRenderDrawColor(renderer, options.background.r, options.background.g, options.background.b, 255);
		SDL_RenderClear(renderer);

		// get the size of the window
		SDL_Point window_size;
		if (window) {
			SDL_GetWindowSize(window, &window_size.x, &window_size.y);
		} else if (options.terminal) {
			if (options.size_set)
				window_size = (SDL_Point){options.size.x, options.size.y}; // specified size
			else
				window_size = (SDL_Point){term_surface->w, term_surface->h}; // whole terminal size
		} else {
			eprintf("Window is NULL\n");
			return 1;
		}

		// find the right scaling mode to fit the image in the window
		SDL_Rect rect;
		if (options.stretch) {
			// stretch image to window/terminal size
			rect = (SDL_Rect){.x = 0, .y = 0, .w = window_size.x, .h = window_size.y};
		} else {
			// fit image to window/terminal size
			unsigned int x_mul = options.terminal && options.unicode != TOGGLE_ON ? 2u : 1u;
			rect = get_fit_mode((SDL_Point){surface->w * (x_mul), surface->h}, window_size);
		}

		if (options.terminal) {
			if (options.position_set) {
				// offset by the correct position
				rect.x += options.position.x;
				rect.y += options.position.y;
			} else if (options.size_set) {
				// center the image in the terminal
				rect.x = ((int) term_size.x - rect.w) / 2;
				rect.y = ((int) term_size.y - rect.h) / 2;
			}
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
			if (render_image_to_terminal(term_surface, options.unicode == TOGGLE_ON, options.bit_depth, stdout, should_continue) == FAIL) {
				eprintf("Failed to render image to terminal\n");
				return 1;
			}
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
