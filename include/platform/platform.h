#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Platform color representation (R8G8B8A8)
 */
typedef uint32_t Color;

#define COLOR_RGBA(r, g, b, a)                                                 \
  ((Color)((r) | ((g) << 8) | ((b) << 16) | ((a) << 24)))
#define COLOR_RGB(r, g, b) COLOR_RGBA(r, g, b, 255)

/**
 * @brief Input button flags
 */
typedef enum {
  BN_UP = 1 << 0,
  BN_DOWN = 1 << 1,
  BN_LEFT = 1 << 2,
  BN_RIGHT = 1 << 3,
  BN_CROSS = 1 << 4,
  BN_CIRCLE = 1 << 5,
  BN_SQUARE = 1 << 6,
  BN_TRIANGLE = 1 << 7,
  BN_START = 1 << 8,
  BN_SELECT = 1 << 9,
  BN_EXIT = 1 << 10
} Button;

typedef struct {
  uint32_t buttons_held;
  uint32_t buttons_pressed;
} InputState;

/**
 * @brief Platform Abstraction Layer (PAL) Interface
 * @param title Title of the window
 * @param width Width of the window
 * @param height Height of the window
 * @return True if the platform was initialized successfully
 */
bool platform_init(const char *title, int width, int height);
/**
 * @brief Initialize audio
 * @return True if audio was initialized successfully
 */
bool platform_init_audio();
/**
 * @brief Shutdown platform
 */
void platform_shutdown();
/**
 * @brief Clear the screen
 * @param color Color to clear the screen with
 */
void platform_clear(Color color);
/**
 * @brief Draw a line
 * @param x1 Start X coordinate
 * @param y1 Start Y coordinate
 * @param x2 End X coordinate
 * @param y2 End Y coordinate
 * @param color Color of the line
 */
void platform_draw_line(int x1, int y1, int x2, int y2, Color color);
/**
 * @brief Draw a triangle
 * @param x1 Vertex 1 X coordinate
 * @param y1 Vertex 1 Y coordinate
 * @param x2 Vertex 2 X coordinate
 * @param y2 Vertex 2 Y coordinate
 * @param x3 Vertex 3 X coordinate
 * @param y3 Vertex 3 Y coordinate
 * @param color Color of the triangle
 */
void platform_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3,
                            Color color);
/**
 * @brief Draw a pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color of the pixel
 */
void platform_draw_pixel(int x, int y, Color color);
/**
 * @brief Present the screen
 */
void platform_present();
/**
 * @brief Poll for input
 * @param state Input state
 */
void platform_poll_input(InputState *state);
/**
 * @brief Get the time
 * @return Time in seconds
 */
float platform_get_time();
/**
 * @brief Get the width
 * @return Width of the window
 */
int platform_get_width();
/**
 * @brief Get the height
 * @return Height of the window
 */
int platform_get_height();

/* ---- Runtime backend switching ---- */
struct RendererBackend; /* forward declaration — see renderer_backend.h */

/**
 * @brief Switch the active renderer backend.
 *        Caller must shutdown the old backend and init the new one.
 */
void platform_set_backend(const struct RendererBackend *backend);

/**
 * @brief Return the name of the active backend ("SDL2", "PSP GU", …)
 */
const char *platform_get_backend_name(void);

#endif // PLATFORM_H
