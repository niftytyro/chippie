#include "display.h"
#include <SDL2/SDL_mixer.h>

Mix_Chunk *beep = NULL;

int init_audio() {
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    log_sdl_error("Failed to initialize audio.");
    return 1;
  }

  beep = Mix_LoadWAV("../assets/mixkit-arcade-retro-changing-tab-206.wav");
  if (beep == NULL) {
    log_sdl_error("Failed to load the beep.");
    return 1;
  }

  return 0;
}

void play_sound() { Mix_PlayChannel(-1, beep, 0); }

void exit_audio() {
  Mix_FreeChunk(beep);
  beep = NULL;

  Mix_Quit();
}
