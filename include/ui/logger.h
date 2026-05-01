#ifndef LOGGER_H
#define LOGGER_H

#include "platform/platform.h"

#define MAX_LOG_LINES 10
#define MAX_LOG_LENGTH 100

typedef struct {
    char lines[MAX_LOG_LINES][MAX_LOG_LENGTH];
    int count;
    int x, y;
    bool align_right;
    bool bottom_up;
    bool overwrite_mode;
    float timer;
    float duration;
    Color color;
} Logger;

/**
 * @brief Initialize a logger instance
 */
void logger_init(Logger* logger, int x, int y, bool align_right, bool bottom_up, Color color);

/**
 * @brief Set the logger to overwrite previous messages and clear after a duration
 */
void logger_set_auto_clear(Logger* logger, bool overwrite, float duration);

/**
 * @brief Update the logger (for timing)
 */
void logger_update(Logger* logger, float dt);

/**
 * @brief Log a message to a specific logger
 */
void logger_log(Logger* logger, const char* format, ...);

/**
 * @brief Render a specific logger
 */
void logger_render(Logger* logger);

#endif // LOGGER_H
