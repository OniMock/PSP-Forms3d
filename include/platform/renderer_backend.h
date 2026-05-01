#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

/**
 * @file renderer_backend.h
 * @brief Runtime-swappable renderer backend vtable.
 *
 * Each backend (SDL2, PSP GU) exports a const RendererBackend struct
 * filled with function pointers that implement the platform drawing API.
 * platform_core.c delegates every platform_* call to the active backend.
 */

#include "platform/platform.h" /* Color, InputState, Button */

typedef struct RendererBackend {
  const char *name;

  bool (*init)(const char *title, int w, int h);
  bool (*init_audio)(void);
  void (*shutdown)(void);

  void (*clear)(Color color);
  void (*draw_line)(int x1, int y1, int x2, int y2, Color color);
  void (*draw_triangle)(int x1, int y1, int x2, int y2, int x3, int y3,
                        Color color);
  void (*draw_pixel)(int x, int y, Color color);
  void (*present)(void);

  void (*poll_input)(InputState *state);
  float (*get_time)(void);
  int (*get_width)(void);
  int (*get_height)(void);
} RendererBackend;

/* Exported backends */
extern const RendererBackend BACKEND_SDL;
extern const RendererBackend BACKEND_PSP_GU;

#endif /* RENDERER_BACKEND_H */
