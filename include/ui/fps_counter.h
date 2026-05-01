#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include "platform/platform.h"

/**
 * @brief Initialize the FPS counter
 */
void fps_init();

/**
 * @brief Update FPS calculation
 * @param dt Time since last frame
 */
void fps_update(float dt);

/**
 * @brief Render the FPS counter
 */
void fps_render();

#endif // FPS_COUNTER_H
