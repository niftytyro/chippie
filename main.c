#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DISPLAY_SCALE_FACTOR 15;

const unsigned int WINDOW_WIDTH = 64 * DISPLAY_SCALE_FACTOR;
const unsigned int WINDOW_HEIGHT = 32 * DISPLAY_SCALE_FACTOR;

// in this implementation, programs start at 0x50
// (till that point, memory is taken up by pre-loaded fonts)
// and end at 0xF00 (remaining space is taken by the display)
const size_t PROGRAM_START = 80;
const size_t PROGRAM_END = 3840;

unsigned char registers[16];
uint16_t I = 0;
uint16_t stack[16];
unsigned char memory[4096] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

void read_rom(char path[]) {
  FILE *fptr;

  unsigned char command[2];
  size_t bytes_read = 0;

  fptr = fopen(path, "rb");

  while ((bytes_read = fread(command, 1, sizeof(command), fptr))) {
    printf("%x%x ", command[1], command[0]);
  }

  fclose(fptr);
}

struct Display_Struct {
  SDL_Renderer *renderer;
  SDL_Window *window;
};

typedef struct Display_Struct Display;

void log_sdl_error(char *msg) { printf("%s\nError:%s", msg, SDL_GetError()); }

Display setup_display() {
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

void draw_random_box(Display display) {
  bool quit = false;

  while (!quit) {

    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        quit = true;
      }
    }

    SDL_SetRenderDrawColor(display.renderer, 0, 0, 0, 255);
    SDL_RenderClear(display.renderer);

    SDL_Rect rect = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 10, 10};
    SDL_SetRenderDrawColor(display.renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(display.renderer, &rect);

    SDL_RenderPresent(display.renderer);
  }
}

int main() {
  read_rom("./test.ch8");

  Display display = setup_display();

  if (display.renderer == NULL || display.window == NULL) {
    return 1;
  }

  draw_random_box(display);

  SDL_DestroyRenderer(display.renderer);
  SDL_DestroyWindow(display.window);
  SDL_Quit();

  return 0;
}
