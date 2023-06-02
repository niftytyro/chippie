#include "audio.h"
#include "display.h"
#include "interpreter.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int init_SDL() {
  int sdl_init_failure = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  if (sdl_init_failure) {
    log_sdl_error("Failed to initialize sdl.");
    return 1;
  }

  return 0;
}

int main() {
  int sdl_init_failure = init_SDL();

  if (sdl_init_failure) {
    return 1;
  }

  Display display = boot_display();

  if (display.renderer == NULL || display.window == NULL) {
    return 1;
  }

  int sdl_audio_failure = init_audio();

  if (sdl_audio_failure) {
    return 1;
  }

  read_rom_into_memory("../roms/connect 4.ch8");
  execute_rom(display);

  exit_audio();
  exit_display(display);

  SDL_Quit();

  return 0;
}
