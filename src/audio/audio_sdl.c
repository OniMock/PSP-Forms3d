/**
 * @file audio_sdl.c
 * @brief SDL2_mixer audio backend — exports BACKEND_AUDIO_SDL vtable.
 */

#include "audio/audio_backend.h"
#include <SDL2/SDL_mixer.h>
#include <stdio.h>

static Mix_Music *g_music = NULL;

static bool sdl_audio_init(void) {
  /* Doubled buffer size from 2048 to 4096 to prevent stuttering during heavy
   * software rendering */
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
    printf("SDL_mixer Error: %s\n", Mix_GetError());
    return false;
  }
  return true;
}

static void sdl_audio_shutdown(void) {
  if (g_music) {
    Mix_HaltMusic();
    Mix_FreeMusic(g_music);
    g_music = NULL;
  }
  Mix_CloseAudio();
}

static bool sdl_audio_play(const char *path) {
  if (g_music) {
    Mix_HaltMusic();
    Mix_FreeMusic(g_music);
  }
  g_music = Mix_LoadMUS(path);
  if (!g_music) {
    printf("Mix_LoadMUS error: %s\n", Mix_GetError());
    return false;
  }
  if (Mix_PlayMusic(g_music, -1) == -1) {
    printf("Mix_PlayMusic error: %s\n", Mix_GetError());
    return false;
  }
  return true;
}

static void sdl_audio_stop(void) { Mix_HaltMusic(); }

const AudioBackend BACKEND_AUDIO_SDL = {
    .name = "SDL2_mixer",
    .init = sdl_audio_init,
    .shutdown = sdl_audio_shutdown,
    .play_music = sdl_audio_play,
    .stop_music = sdl_audio_stop,
};
