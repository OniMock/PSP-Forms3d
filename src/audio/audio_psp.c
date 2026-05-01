/**
 * @file audio_psp.c
 * @brief PSP native audio backend — exports BACKEND_AUDIO_PSP vtable.
 *
 * Streams a WAV (PCM 16-bit stereo/mono) file using raw pspaudio channels.
 * A kernel thread handles the double-buffered output loop.
 *
 * Supports: PCM 16-bit, any sample rate, stereo or mono.
 * (The PSP hardware resamples internally on sceAudioOutputPannedBlocking.)
 */

#include "audio/audio_backend.h"

#include <pspaudio.h>
#include <pspkernel.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AUDIO_CHANNEL 0
#define AUDIO_SAMPLES 512 /* samples per output block */

/* ---- WAV header structures ---- */
typedef struct {
  char riff[4]; /* "RIFF" */
  uint32_t file_size;
  char wave[4]; /* "WAVE" */
} WavRiff;

typedef struct {
  uint16_t audio_format; /* 1 = PCM */
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
} WavFmt;

/* ---- Playback state ---- */
static FILE *g_file = NULL;
static int g_channel = -1;
static SceUID g_thread_id = -1;
static bool g_playing = false;
static long g_data_start = 0; /* file offset of the first PCM sample */
static int g_num_ch = 2;      /* 1 = mono, 2 = stereo */

/* Double-buffer: while one block is being played, the other is being filled */
static int16_t
    __attribute__((aligned(64))) g_buf[2][AUDIO_SAMPLES * 2]; /* max stereo */

/* ---- Audio streaming thread ---- */
static int psp_audio_thread(SceSize sz, void *arg) {
  (void)sz;
  (void)arg;
  int buf = 0;
  while (g_playing) {
    size_t bytes = AUDIO_SAMPLES * g_num_ch * sizeof(int16_t);
    size_t read = fread(g_buf[buf], 1, bytes, g_file);
    if (read == 0) {
      /* Loop: rewind to PCM data start */
      fseek(g_file, g_data_start, SEEK_SET);
      read = fread(g_buf[buf], 1, bytes, g_file);
    }
    /* Zero-pad short reads */
    if (read < bytes)
      memset((char *)g_buf[buf] + read, 0, bytes - read);

    sceAudioOutputPannedBlocking(g_channel, PSP_AUDIO_VOLUME_MAX,
                                 PSP_AUDIO_VOLUME_MAX, g_buf[buf]);
    buf ^= 1;
  }
  return 0;
}

/* ---- Backend functions ---- */

static bool psp_audio_init(void) { return true; /* channel reserved on play */ }

static void psp_audio_stop_internal(void) {
  if (!g_playing)
    return;
  g_playing = false;
  if (g_thread_id >= 0) {
    sceKernelWaitThreadEnd(g_thread_id, NULL);
    sceKernelDeleteThread(g_thread_id);
    g_thread_id = -1;
  }
  if (g_file) {
    fclose(g_file);
    g_file = NULL;
  }
  if (g_channel >= 0) {
    sceAudioChRelease(g_channel);
    g_channel = -1;
  }
}

static void psp_audio_shutdown(void) { psp_audio_stop_internal(); }

static bool psp_audio_play(const char *path) {
  psp_audio_stop_internal();

  g_file = fopen(path, "rb");
  if (!g_file)
    return false;

  /* --- Parse WAV header --- */
  WavRiff riff;
  fread(&riff, sizeof(WavRiff), 1, g_file);
  if (memcmp(riff.riff, "RIFF", 4) != 0 || memcmp(riff.wave, "WAVE", 4) != 0) {
    fclose(g_file);
    g_file = NULL;
    return false;
  }

  /* Scan chunks until we find "fmt " */
  char id[4];
  uint32_t chunk_size;
  WavFmt fmt = {0};
  bool got_fmt = false;

  while (fread(id, 4, 1, g_file) == 1 &&
         fread(&chunk_size, 4, 1, g_file) == 1) {
    if (memcmp(id, "fmt ", 4) == 0) {
      size_t read = chunk_size < sizeof(WavFmt) ? chunk_size : sizeof(WavFmt);
      fread(&fmt, 1, read, g_file);
      if (chunk_size > sizeof(WavFmt))
        fseek(g_file, chunk_size - sizeof(WavFmt), SEEK_CUR);
      got_fmt = true;
    } else if (memcmp(id, "data", 4) == 0) {
      g_data_start = ftell(g_file);
      break;
    } else {
      fseek(g_file, chunk_size, SEEK_CUR);
    }
  }

  if (!got_fmt || g_data_start == 0) {
    fclose(g_file);
    g_file = NULL;
    return false;
  }

  g_num_ch = fmt.num_channels;

  /* Reserve audio channel */
  int fmt_flag =
      (g_num_ch == 2) ? PSP_AUDIO_FORMAT_STEREO : PSP_AUDIO_FORMAT_MONO;
  g_channel = sceAudioChReserve(AUDIO_CHANNEL, AUDIO_SAMPLES, fmt_flag);
  if (g_channel < 0) {
    fclose(g_file);
    g_file = NULL;
    return false;
  }

  /* Start streaming thread */
  g_playing = true;
  g_thread_id = sceKernelCreateThread("psp_audio_stream", psp_audio_thread,
                                      0x12, 0x10000, 0, NULL);
  if (g_thread_id < 0) {
    g_playing = false;
    sceAudioChRelease(g_channel);
    g_channel = -1;
    fclose(g_file);
    g_file = NULL;
    return false;
  }
  sceKernelStartThread(g_thread_id, 0, NULL);
  return true;
}

static void psp_audio_stop(void) { psp_audio_stop_internal(); }

/* ---- Exported vtable ---- */

const AudioBackend BACKEND_AUDIO_PSP = {
    .name = "PSP Audio",
    .init = psp_audio_init,
    .shutdown = psp_audio_shutdown,
    .play_music = psp_audio_play,
    .stop_music = psp_audio_stop,
};
