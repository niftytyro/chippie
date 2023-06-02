#include "display.h"
#include "interpreter.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <stdbool.h>
#include <stdio.h>

const int DISPLAY_SCALE_FACTOR = 15;

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
    } else if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
      case SDLK_1:
        key = 0x1;
        break;
      case SDLK_2:
        key = 0x2;
        break;
      case SDLK_3:
        key = 0x3;
        break;
      case SDLK_4:
        key = 0xC;
        break;
      case SDLK_q:
        key = 0x4;
        break;
      case SDLK_w:
        key = 0x5;
        break;
      case SDLK_e:
        key = 0x6;
        break;
      case SDLK_r:
        key = 0xD;
        break;
      case SDLK_a:
        key = 0x7;
        break;
      case SDLK_s:
        key = 0x8;
        break;
      case SDLK_d:
        key = 0x9;
        break;
      case SDLK_f:
        key = 0xE;
        break;
      case SDLK_z:
        key = 0xA;
        break;
      case SDLK_x:
        key = 0x0;
        break;
      case SDLK_c:
        key = 0xB;
        break;
      case SDLK_v:
        key = 0xF;
        break;
      default:
        break;
      }
    }
  }

  // The quitting SDL process is done at the end of the main function.
  return quit;
}

void draw_pixel(int x, int y, unsigned char bit, Display display) {
  SDL_Rect pixel = {x * DISPLAY_SCALE_FACTOR, y * DISPLAY_SCALE_FACTOR,
                    1 * DISPLAY_SCALE_FACTOR, 1 * DISPLAY_SCALE_FACTOR};
  if (bit) {
    SDL_SetRenderDrawColor(display.renderer, 255, 255, 255, 255);
  } else {
    SDL_SetRenderDrawColor(display.renderer, 0, 0, 0, 255);
  }
  SDL_RenderFillRect(display.renderer, &pixel);
}

void draw_byte(Display display, unsigned char byte, int x, int y) {
  for (int i = 0; i < 8; i++) {
    unsigned char pixel = byte & 1;
    draw_pixel(x + 7 - i, y, pixel, display);
    byte = byte >> 1;
  }
}

void draw(Display display, unsigned char *memory) {
  for (int i = 0; i < 256; i++) {
    draw_byte(display, memory[PROGRAM_END + i], 8 * (i % 8), i / 8);
  }
  SDL_RenderPresent(display.renderer);
}

void exit_display(Display display) {
  SDL_DestroyRenderer(display.renderer);
  SDL_DestroyWindow(display.window);

  display.renderer = NULL;
  display.window = NULL;
}
