/**
 * @file audio_core.c
 * @brief Audio system core — delegates to the active AudioBackend vtable.
 *
 * Mirrors the same pattern as platform_core.c but for audio.
 * Default backend is SDL2_mixer; main.c calls audio_set_backend() to switch.
 */

#include "audio/audio.h"
#include "audio/audio_backend.h"

static const AudioBackend *g_audio = &BACKEND_AUDIO_SDL;

void audio_set_backend(const struct AudioBackend *backend) {
  g_audio = (const AudioBackend *)backend;
}

const char *audio_get_backend_name(void) { return g_audio->name; }

bool audio_init(void) { return g_audio->init(); }
void audio_shutdown(void) { g_audio->shutdown(); }
bool audio_play_music(const char *path) { return g_audio->play_music(path); }
void audio_stop_music(void) { g_audio->stop_music(); }
