/**
 * @file platform_sdl.c
 * @brief SDL2 renderer backend — exports BACKEND_SDL vtable.
 *
 * All functions are file-static. Only BACKEND_SDL is exported.
 * platform_core.c calls these via the vtable when SDL2 is the active backend.
 */

#include "platform/renderer_backend.h"
#include <SDL2/SDL.h>
#include <stdio.h>

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Joystick *g_joystick = NULL;
static int g_width = 480;
static int g_height = 272;

/* ---- Init / Shutdown ---- */

static bool sdl_init(const char *title, int width, int height) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0) {
    printf("SDL Init Error: %s\n", SDL_GetError());
    return false;
  }
  g_width = width;
  g_height = height;

  g_window =
      SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       width, height, SDL_WINDOW_SHOWN);
  if (!g_window)
    return false;

  g_renderer = SDL_CreateRenderer(
      g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!g_renderer)
    return false;

  if (SDL_NumJoysticks() > 0)
    g_joystick = SDL_JoystickOpen(0);

  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
  SDL_RenderClear(g_renderer);
  SDL_RenderPresent(g_renderer);
  return true;
}

static bool sdl_init_audio(void) {
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    return false;
  return true;
}

static void sdl_shutdown(void) {
  if (g_joystick) {
    SDL_JoystickClose(g_joystick);
    g_joystick = NULL;
  }
  if (g_renderer) {
    SDL_DestroyRenderer(g_renderer);
    g_renderer = NULL;
  }
  if (g_window) {
    SDL_DestroyWindow(g_window);
    g_window = NULL;
  }
  SDL_Quit();
}

/* ---- Drawing ---- */

static void sdl_clear(Color color) {
  SDL_SetRenderDrawColor(g_renderer, color & 0xFF, (color >> 8) & 0xFF,
                         (color >> 16) & 0xFF, (color >> 24) & 0xFF);
  SDL_RenderClear(g_renderer);
}

static void sdl_draw_line(int x1, int y1, int x2, int y2, Color color) {
  SDL_SetRenderDrawColor(g_renderer, color & 0xFF, (color >> 8) & 0xFF,
                         (color >> 16) & 0xFF, (color >> 24) & 0xFF);
  SDL_RenderDrawLine(g_renderer, x1, y1, x2, y2);
}

static void sdl_draw_pixel(int x, int y, Color color) {
  SDL_SetRenderDrawColor(g_renderer, color & 0xFF, (color >> 8) & 0xFF,
                         (color >> 16) & 0xFF, (color >> 24) & 0xFF);
  SDL_RenderDrawPoint(g_renderer, x, y);
}

static void sdl_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3,
                              Color color) {
  SDL_SetRenderDrawColor(g_renderer, color & 0xFF, (color >> 8) & 0xFF,
                         (color >> 16) & 0xFF, (color >> 24) & 0xFF);

  /* Sort vertices by y ascending */
  if (y1 > y2) {
    int t;
    t = y1;
    y1 = y2;
    y2 = t;
    t = x1;
    x1 = x2;
    x2 = t;
  }
  if (y1 > y3) {
    int t;
    t = y1;
    y1 = y3;
    y3 = t;
    t = x1;
    x1 = x3;
    x3 = t;
  }
  if (y2 > y3) {
    int t;
    t = y2;
    y2 = y3;
    y3 = t;
    t = x2;
    x2 = x3;
    x3 = t;
  }
  if (y3 == y1)
    return;

  for (int y = y1; y <= y3; y++) {
    if (y < 0 || y >= g_height)
      continue;

    float t_ac = (float)(y - y1) / (float)(y3 - y1);
    int lx = x1 + (int)((float)(x3 - x1) * t_ac + 0.5f);

    int sx;
    if (y <= y2 && y2 > y1) {
      float t = (float)(y - y1) / (float)(y2 - y1);
      sx = x1 + (int)((float)(x2 - x1) * t + 0.5f);
    } else {
      int dy_bc = y3 - y2;
      if (dy_bc == 0)
        sx = x2;
      else {
        float t = (float)(y - y2) / (float)dy_bc;
        sx = x2 + (int)((float)(x3 - x2) * t + 0.5f);
      }
    }

    int xa = lx < sx ? lx : sx;
    int xb = lx < sx ? sx : lx;
    if (xa < 0)
      xa = 0;
    if (xb >= g_width)
      xb = g_width - 1;
    if (xa <= xb)
      SDL_RenderDrawLine(g_renderer, xa, y, xb, y);
  }
}

static void sdl_present(void) { SDL_RenderPresent(g_renderer); }

/* ---- Input ---- */

static void sdl_poll_input(InputState *state) {
  SDL_Event e;
  static uint32_t current_buttons = 0;
  state->buttons_pressed = 0;

  while (SDL_PollEvent(&e) != 0) {
    uint32_t bit = 0;
    bool down = false;

    if (e.type == SDL_QUIT) {
      bit = BN_EXIT;
      down = true;
    } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
      down = (e.type == SDL_KEYDOWN);
      switch (e.key.keysym.scancode) {
      case SDL_SCANCODE_UP:
        bit = BN_UP;
        break;
      case SDL_SCANCODE_DOWN:
        bit = BN_DOWN;
        break;
      case SDL_SCANCODE_LEFT:
        bit = BN_LEFT;
        break;
      case SDL_SCANCODE_RIGHT:
        bit = BN_RIGHT;
        break;
      case SDL_SCANCODE_X:
        bit = BN_CROSS;
        break;
      case SDL_SCANCODE_C:
        bit = BN_CIRCLE;
        break;
      case SDL_SCANCODE_S:
        bit = BN_SQUARE;
        break;
      case SDL_SCANCODE_T:
        bit = BN_TRIANGLE;
        break;
      case SDL_SCANCODE_RETURN:
        bit = BN_START;
        break;
      case SDL_SCANCODE_ESCAPE:
        bit = BN_SELECT;
        break;
      default:
        break;
      }
    } else if (e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP) {
      down = (e.type == SDL_JOYBUTTONDOWN);
      switch (e.jbutton.button) {
      case 8:
        bit = BN_UP;
        break;
      case 6:
        bit = BN_DOWN;
        break;
      case 7:
        bit = BN_LEFT;
        break;
      case 9:
        bit = BN_RIGHT;
        break;
      case 2:
        bit = BN_CROSS;
        break;
      case 1:
        bit = BN_CIRCLE;
        break;
      case 0:
        bit = BN_SQUARE;
        break;
      case 3:
        bit = BN_TRIANGLE;
        break;
      case 11:
        bit = BN_START;
        break;
      case 10:
        bit = BN_SELECT;
        break;
      default:
        break;
      }
    }

    if (bit) {
      if (down) {
        if (!(current_buttons & bit))
          state->buttons_pressed |= bit;
        current_buttons |= bit;
      } else {
        current_buttons &= ~bit;
      }
    }
  }
  state->buttons_held = current_buttons;
}

/* ---- Time / Screen ---- */

static float sdl_get_time(void) { return SDL_GetTicks() / 1000.0f; }
static int sdl_get_width(void) { return g_width; }
static int sdl_get_height(void) { return g_height; }

/* ---- Exported vtable ---- */

const RendererBackend BACKEND_SDL = {
    .name = "SDL2",
    .init = sdl_init,
    .init_audio = sdl_init_audio,
    .shutdown = sdl_shutdown,
    .clear = sdl_clear,
    .draw_line = sdl_draw_line,
    .draw_triangle = sdl_draw_triangle,
    .draw_pixel = sdl_draw_pixel,
    .present = sdl_present,
    .poll_input = sdl_poll_input,
    .get_time = sdl_get_time,
    .get_width = sdl_get_width,
    .get_height = sdl_get_height,
};
