#ifndef AUDIO_BACKEND_H
#define AUDIO_BACKEND_H

/**
 * @file audio_backend.h
 * @brief Runtime-swappable audio backend vtable.
 *
 * Each audio backend (SDL2_mixer, PSP native) exports a const AudioBackend
 * struct. audio_core.c delegates every audio_* call to the active backend.
 */

#include <stdbool.h>

typedef struct AudioBackend {
  const char *name;

  bool (*init)(void);
  void (*shutdown)(void);
  bool (*play_music)(const char *path);
  void (*stop_music)(void);
} AudioBackend;

/* Exported backends */
extern const AudioBackend BACKEND_AUDIO_SDL;
extern const AudioBackend BACKEND_AUDIO_PSP;

#endif /* AUDIO_BACKEND_H */
