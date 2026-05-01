/**
 * @file platform_core.c
 * @brief Platform Abstraction Layer core — delegates to the active
 * RendererBackend.
 *
 * This is the ONLY translation unit that implements platform_*.
 * All other code calls platform_* and this file forwards each call
 * to the currently active backend vtable pointer.
 */

#include "platform/platform.h"
#include "platform/renderer_backend.h"

/* Default backend is SDL2; main.c can call platform_set_backend() to switch. */
static const RendererBackend *g_backend = &BACKEND_SDL;

void platform_set_backend(const struct RendererBackend *backend) {
  g_backend = (const RendererBackend *)backend;
}

const char *platform_get_backend_name(void) { return g_backend->name; }

/* ---- Delegate all platform_* calls to active backend ---- */

bool platform_init(const char *title, int w, int h) {
  return g_backend->init(title, w, h);
}

bool platform_init_audio(void) { return g_backend->init_audio(); }

void platform_shutdown(void) { g_backend->shutdown(); }

void platform_clear(Color color) { g_backend->clear(color); }

void platform_draw_line(int x1, int y1, int x2, int y2, Color color) {
  g_backend->draw_line(x1, y1, x2, y2, color);
}

void platform_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3,
                            Color color) {
  g_backend->draw_triangle(x1, y1, x2, y2, x3, y3, color);
}

void platform_draw_pixel(int x, int y, Color color) {
  g_backend->draw_pixel(x, y, color);
}

void platform_present(void) { g_backend->present(); }

void platform_poll_input(InputState *state) { g_backend->poll_input(state); }

float platform_get_time(void) { return g_backend->get_time(); }

int platform_get_width(void) { return g_backend->get_width(); }

int platform_get_height(void) { return g_backend->get_height(); }
