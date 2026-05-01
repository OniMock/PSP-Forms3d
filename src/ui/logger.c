#include "ui/logger.h"
#include "ui/text_renderer.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void logger_init(Logger *logger, int x, int y, bool align_right, bool bottom_up,
                 Color color) {
  memset(logger, 0, sizeof(Logger));
  logger->x = x;
  logger->y = y;
  logger->align_right = align_right;
  logger->bottom_up = bottom_up;
  logger->color = color;
  logger->duration = 0; // Infinite by default
}

void logger_set_auto_clear(Logger *logger, bool overwrite, float duration) {
  logger->overwrite_mode = overwrite;
  logger->duration = duration;
}

void logger_update(Logger *logger, float dt) {
  if (logger->duration > 0 && logger->count > 0) {
    logger->timer -= dt;
    if (logger->timer <= 0) {
      logger->count = 0;
      memset(logger->lines, 0, sizeof(logger->lines));
    }
  }
}

void logger_log(Logger *logger, const char *format, ...) {
  char buffer[MAX_LOG_LENGTH];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, MAX_LOG_LENGTH, format, args);
  va_end(args);

  // Reset timer on new message
  logger->timer = logger->duration;

  if (logger->overwrite_mode) {
    strncpy(logger->lines[0], buffer, MAX_LOG_LENGTH);
    logger->count = 1;
    return;
  }

  if (logger->count < MAX_LOG_LINES) {
    strncpy(logger->lines[logger->count], buffer, MAX_LOG_LENGTH);
    logger->count++;
  } else {
    // Shift lines up
    for (int i = 0; i < MAX_LOG_LINES - 1; i++) {
      strncpy(logger->lines[i], logger->lines[i + 1], MAX_LOG_LENGTH);
    }
    strncpy(logger->lines[MAX_LOG_LINES - 1], buffer, MAX_LOG_LENGTH);
  }

  // Also print to console for debugging
  printf("[LOG] %s\n", buffer);
}

void logger_render(Logger *logger) {
  int ly = logger->y;
  for (int i = 0; i < logger->count; i++) {
    if (logger->align_right) {
      text_draw_string_right(logger->x, ly, logger->lines[i], logger->color);
    } else {
      text_draw_string(logger->x, ly, logger->lines[i], logger->color);
    }

    if (logger->bottom_up) {
      ly -= 10;
    } else {
      ly += 10;
    }
  }
}
