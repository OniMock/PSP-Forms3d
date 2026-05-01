#include "ui/fps_counter.h"
#include "ui/text_renderer.h"
#include <stdio.h>

static float g_fps = 0;
static int g_frames = 0;
static float g_accumulator = 0;

void fps_init() {
  g_fps = 0;
  g_frames = 0;
  g_accumulator = 0;
}

void fps_update(float dt) {
  g_accumulator += dt;
  g_frames++;
  if (g_accumulator >= 1.0f) {
    g_fps = (float)g_frames / g_accumulator;
    g_frames = 0;
    g_accumulator = 0;
  }
}

void fps_render() {
  char buffer[20];
  sprintf(buffer, "FPS: %.2f", g_fps);

  // Bottom center alignment
  int x = platform_get_width() / 2;
  int y = platform_get_height() - 15;

  // Vibrant green for FPS
  Color fpsColor = COLOR_RGB(0, 255, 128);
  text_draw_string_center(x, y, buffer, fpsColor);
}
