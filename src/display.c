#include "display.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <stdbool.h>

#define DISPLAY_SCALE_FACTOR 15;

const unsigned int WINDOW_WIDTH = 64 * DISPLAY_SCALE_FACTOR;
const unsigned int WINDOW_HEIGHT = 32 * DISPLAY_SCALE_FACTOR;

void log_sdl_error(char *msg) { printf("%s\nError:%s", msg, SDL_GetError()); }

void clear_screen(Display display) {
  SDL_SetRenderDrawColor(display.renderer, 0, 0, 0, 255);
  SDL_RenderClear(display.renderer);
}

Display boot_display() {
  Display display;
  display.renderer = NULL;
  display.window = NULL;
  int sdl_init_failure = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  if (sdl_init_failure) {
    log_sdl_error("Failed to initialize sdl.");
    return display;
  }

  display.window = SDL_CreateWindow("Chippie", 100, 100, WINDOW_WIDTH,
                                    WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

  if (display.window == NULL) {
    log_sdl_error("Something went wrong while creating window.");
    SDL_Quit();
    return display;
  }

  display.renderer = SDL_CreateRenderer(
      display.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (display.renderer == NULL) {
    log_sdl_error("Something went wrong while creating renderer.");
    SDL_DestroyWindow(display.window);
    SDL_Quit();
    return display;
  }

  return display;
}

bool handle_sdl_event() {
  SDL_Event event;
  bool quit = false;

  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT) {
      quit = true;
    }
  }

  // The quitting SDL process is done at the end of the main function.
  return quit;
}

void redraw_frame(Display display) { SDL_RenderPresent(display.renderer); }

void draw_random_box(Display display) {
  bool quit = false;

  while (!quit) {

    quit = handle_sdl_event();

    clear_screen(display);

    /* SDL_Rect rect = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 10, 10}; */
    /* SDL_SetRenderDrawColor(display.renderer, 255, 255, 255, 255); */
    /* SDL_RenderFillRect(display.renderer, &rect); */

    redraw_frame(display);
  }
}

void exit_display(Display display) {
  SDL_DestroyRenderer(display.renderer);
  SDL_DestroyWindow(display.window);
  SDL_Quit();
}
