#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

bool audio_init();
void audio_shutdown();
bool audio_play_music(const char *path);
void audio_stop_music();

/* ---- Runtime backend switching ---- */
struct AudioBackend; /* forward declaration — see audio_backend.h */

/**
 * @brief Set the audio backend.
 * @param backend The audio backend to use.
 */
void audio_set_backend(const struct AudioBackend *backend);

/**
 * @brief Get the name of the current audio backend.
 * @return The name of the current audio backend.
 */
const char *audio_get_backend_name(void);

#endif // AUDIO_H
