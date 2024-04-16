#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <err.h>
#include <signal.h>
#include <sys/stat.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "./util.h"
#include "./arg.h"
#include "./image.h"

#define PROGRAM_NAME "foto"

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
		{"sigusr2",    no_argument,       0, '2'},
		{0, 0,                            0, 0}
};

bool resize_window = false, reload_image = false;

void sigusr1_handler() {
	resize_window = true;
}

void sigusr2_handler() {
	reload_image = true;
}

int main(int argc, char *argv[]) {
	// arguments
	struct {
		char *title;
		bool stretch, hot_reload, sigusr1, sigusr2, position_set, size_set, background_set;
		SDL_Point position, size;
		SDL_Color background;
	} options = {
			.title = NULL,
			.stretch = false, .hot_reload = false, .sigusr1 = false, .sigusr2 = false, .position_set = false, .size_set = false, .background_set = false,
	};

	bool invalid = false; // don't immediately exit when invalid argument, so we can still use --help
	int opt;
	long nums[3];

	// argument handling
	while ((opt = getopt_long(argc, argv, ":hVt:c:p:s:b:Sr12", options_getopt, NULL)) != -1) {
		if (opt == 'h') {
			// help text
			printf("Usage: %s [options_getopt] file\n\
-h --help: Shows help text\n\
-V --version: Shows the current version\n\
-t --title [title]: Sets the window title\n\
-p --position [x],[y]: Sets the window position\n\
-s --size [w],[h]: Sets the window size, defaults to the size of the image\n\
-b --background [r],[g],[b]: Sets the background colour (0-255)\n\
-S --stretch: Allows the image to stretch instead of fitting to the window\n\
-r --hotreload: Reloads image when it is modified, will not work with stdin\n\
-1 --sigusr1: Allows the SIGUSR1 signal to resize_window the window to the size of the image\n\
-2 --sigusr2: Allows the SIGUSR2 signal to reload_image the image on demand\n\
", PROGRAM_NAME);

			return 0;
		} else if (opt == 'V') {
			printf("Version: "VERSION"\n");
			return 0;
		} else if (!invalid) {
			switch (opt) {
				case 't':
					if (options.title) invalid = true;
					else options.title = optarg;
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
				default:
					invalid = true;
					break;
			}
		}
	}

	if (optind != argc - 1 || invalid || !argv[optind][0]) {
		errx(1, "Invalid usage, try --help");
		return 1;
	}

	int ret = 1;

	char *filename = argv[optind];

	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_Surface *surface = NULL;
	SDL_Texture *texture = NULL;
	char *title_default = NULL;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		warnx("Failed to initialize SDL: %s", SDL_GetError());
		goto clean;
	}

	if (!(IMG_Init(-1))) {
		// if it fails to load every image type
		// not any image type, because we don't want to require libavif if the image is png for example
		fprintf(stderr, "%s\n", IMG_GetError());
		warnx("Failed to initialize SDL image");
		goto clean;
	}

	FILE *fp = open_file(filename);

	if (!fp) goto clean;

	if (fp == stdin && options.hot_reload) {
		warnx("Cannot hot-reload with stdin");
		options.hot_reload = false;
	}

	surface = read_file(fp);

	close_file(fp);

	if (!surface) goto clean;

	if (!options.title) {
		// if title isn't set, set it to the last component of filename
		options.title = strrchr(filename, '/');
		if (!options.title) options.title = filename;
		else ++options.title;

		title_default = malloc(strlen(options.title) + 16);
		if (!title_default) {
			warn("malloc");
			goto clean;
		}

		sprintf(title_default, "%s - %s", options.title, PROGRAM_NAME);
		options.title = title_default;
	}

	// create window
	window = SDL_CreateWindow(
			options.title,
			options.position_set ? options.position.x : SDL_WINDOWPOS_UNDEFINED,
			options.position_set ? options.position.y : SDL_WINDOWPOS_UNDEFINED,
			options.size_set ? options.size.x : surface->w,
			options.size_set ? options.size.y : surface->h,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	if (options.position_set) {
		// set window position
		SDL_SetWindowPosition(window, options.position.x, options.position.y);
	}

	if (!window) {
		warnx("Failed to create window: %s", SDL_GetError());
		goto clean;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (!renderer) {
		warnx("Failed to create renderer: %s", SDL_GetError());
		goto clean;
	}

	// set up signal handlers
	signal(SIGUSR1, sigusr1_handler);
	signal(SIGUSR2, sigusr2_handler);

	// variables for hot-reload
	struct stat st; // stat
	time_t prev_mtime = 0; // previous modification date
	unsigned long long last_checked = 0; // the time at when we have last checked

	// main loop
	bool running = true, should_reload;
	while (running) {
		// from signal handler
		if (resize_window && options.sigusr1) {
			// resize the window to the size of the image
			resize_window = false;
			SDL_SetWindowSize(window, surface->w, surface->h);
		}

		// from signal handler
		should_reload = reload_image && options.sigusr2;

		if (!should_reload && options.hot_reload) {
			// get current time
			unsigned long long current_time = get_time();

			// if a second has passed since we last checked this
			if (current_time > last_checked + 1000) {
				// set time last checked
				last_checked = current_time;

				// stat the file
				if (stat(filename, &st) != 0) {
					warn("stat: %s", filename);
					goto clean;
				}

				// reload if modification date has changed
				if (st.st_mtime != prev_mtime) {
					prev_mtime = st.st_mtime;
					should_reload = true;
				}
			}
		}

		if (should_reload) {
			// reload the image
			reload_image = false;

			fp = open_file(filename);
			if (!fp) goto clean;
			SDL_Surface *new_surface = read_file(fp);
			close_file(fp);

			if (new_surface) {
				// destroy the old texture and surface
				if (texture) SDL_DestroyTexture(texture);
				texture = NULL;

				if (surface) SDL_FreeSurface(surface);
				surface = new_surface;
			} else {
				warnx("Failed to load updated image");
			}
		}

		// set background color
		SDL_SetRenderDrawColor(renderer, options.background.r, options.background.g, options.background.b, 255);
		SDL_RenderClear(renderer);

		// get the size of the window
		SDL_Point window_size;
		SDL_GetWindowSize(window, &window_size.x, &window_size.y);

		// find the right scaling mode to fit the image in the window
		SDL_Rect rect;
		if (options.stretch) rect = (SDL_Rect) {.x = 0, .y = 0, .w = window_size.x, .h = window_size.y};
		else rect = get_fit_mode((SDL_Point) {surface->w, surface->h}, window_size);

		if (!texture) {
			texture = SDL_CreateTextureFromSurface(renderer, surface);
			if (!texture) {
				warnx("Failed to create texture: %s", SDL_GetError());
				goto clean;
			}
		}

		// draw the image with the rectangle
		SDL_RenderCopy(renderer, texture, NULL, &rect);

		SDL_RenderPresent(renderer);

		SDL_Event event;
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT) {
				running = false;
			}
		}
	}

	ret = 0;

	// clean up
	clean:
	if (renderer) SDL_DestroyRenderer(renderer);
	if (window) SDL_DestroyWindow(window);
	if (texture) SDL_DestroyTexture(texture);
	if (surface) SDL_FreeSurface(surface);
	IMG_Quit();
	SDL_Quit();
	if (title_default) free(title_default);

	return ret;
}
