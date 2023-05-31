#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <SDL2/SDL_render.h>

struct Display_Struct {
  SDL_Renderer *renderer;
  SDL_Window *window;
};

typedef struct Display_Struct Display;

// function declarations
Display boot_display();

void draw_random_box(Display display);

void redraw_frame(Display display);

bool handle_sdl_event();

void clear_screen(Display display);

void log_sdl_error(char *msg);

void exit_display(Display display);

#endif
