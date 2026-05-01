#include "audio/audio.h"
#include "audio/audio_backend.h" /* BACKEND_AUDIO_SDL, BACKEND_AUDIO_PSP */
#include "engine/math3d.h"
#include "engine/render3d.h"
#include "platform/platform.h"
#include "platform/renderer_backend.h" /* BACKEND_SDL, BACKEND_PSP_GU */
#include "ui/controls_guide.h"
#include "ui/fps_counter.h"
#include "ui/logger.h"
#include "ui/text_renderer.h"
#include <math.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <stdio.h>

PSP_MODULE_INFO("Forms3d", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* ---- PSP Home-button exit callback ---- */
static volatile bool g_running_flag = true;

static int exit_callback(int arg1, int arg2, void *common) {
  (void)arg1;
  (void)arg2;
  (void)common;
  g_running_flag = false;
  return 0;
}

static int callback_thread(SceSize args, void *argp) {
  (void)args;
  (void)argp;
  int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
  sceKernelRegisterExitCallback(cbid);
  sceKernelSleepThreadCB(); /* sleep forever, wake on callback */
  return 0;
}

static void setup_exit_callback(void) {
  int thid = sceKernelCreateThread("callback_thread", callback_thread,
                                   0x11, 0xFA0, 0, NULL);
  if (thid >= 0)
    sceKernelStartThread(thid, 0, NULL);
}

/* ---- Palette ---- */
static Color g_colors[] = {
    COLOR_RGB(0, 255, 128),   /* Vibrant Green  */
    COLOR_RGB(255, 100, 100), /* Soft Red       */
    COLOR_RGB(100, 150, 255), /* Sky Blue       */
    COLOR_RGB(255, 200, 50),  /* Golden Yellow  */
    COLOR_RGB(200, 100, 255), /* Purple         */
};
static int g_color_idx = 0;

/* ---- UI instances ---- */
static Logger g_sys_logger;
static Logger g_btn_logger;

/* ---- Backend switch helper ---- */
static void switch_backends(bool to_psp_native) {
  /* 1. Clear screen to black in current backend before teardown */
  platform_clear(COLOR_RGB(0, 0, 0));
  platform_present();

  /* 2. Tear down audio and platform */
  audio_stop_music();
  audio_shutdown();
  platform_shutdown();

  /* 3. Point to new vtables */
  if (to_psp_native) {
    platform_set_backend(&BACKEND_PSP_GU);
    audio_set_backend(&BACKEND_AUDIO_PSP);
  } else {
    platform_set_backend(&BACKEND_SDL);
    audio_set_backend(&BACKEND_AUDIO_SDL);
  }

  /* 4. Bring up the new backend */
  if (platform_init("PSP 3D Cube - Forms3d", 480, 272)) {
    /* Clear new backend immediately to avoid garbage in VRAM */
    platform_clear(COLOR_RGB(0, 0, 0));
    platform_present();

    if (platform_init_audio() && audio_init()) {
      audio_play_music("assets/music.wav");
    }
  }
}

int main(int argc, char *argv[]) {
  /* 0. Register Home-button exit callback */
  setup_exit_callback();

  /* 1. Fast PSP boot splash */
  pspDebugScreenInit();
  pspDebugScreenPrintf(
      "\n\n\n\n\n\n\n\n              Forms3d Engine - Booting...");

  /* 1. Platform init — start with SDL2 backend */
  platform_set_backend(&BACKEND_SDL);
  audio_set_backend(&BACKEND_AUDIO_SDL);

  if (!platform_init("PSP 3D Cube - Forms3d", 480, 272))
    return 1;

  /* 2. UI init */
  text_init();
  logger_init(&g_sys_logger, 10, 10, false, false, COLOR_RGB(255, 255, 255));
  logger_init(&g_btn_logger, 10, 260, false, true, COLOR_RGB(100, 255, 100));
  logger_set_auto_clear(&g_btn_logger, true, 3.0f);
  fps_init();

  logger_log(&g_sys_logger, "System Starting...");
  platform_clear(COLOR_RGB(20, 20, 25));
  logger_render(&g_sys_logger);
  platform_present();

  /* 3. Audio init */
  logger_log(&g_sys_logger, "Initializing Audio...");
  platform_clear(COLOR_RGB(20, 20, 25));
  logger_render(&g_sys_logger);
  platform_present();

  if (platform_init_audio() && audio_init()) {
    logger_log(&g_sys_logger, "Audio OK [%s]", audio_get_backend_name());
  } else {
    logger_log(&g_sys_logger, "Audio Init FAILED");
  }
  platform_clear(COLOR_RGB(20, 20, 25));
  logger_render(&g_sys_logger);
  platform_present();

  /* 4. Load initial mesh */
  logger_log(&g_sys_logger, "Backend: %s", platform_get_backend_name());
  Mesh current_mesh = mesh_create_cube(1.2f);
  ShapeType current_shape = SHAPE_CUBE;
  RenderMode current_mode = RENDER_WIREFRAME;

  platform_clear(COLOR_RGB(20, 20, 25));
  logger_render(&g_sys_logger);
  platform_present();

  logger_log(&g_sys_logger, "Ready");
  if (audio_play_music("assets/music.wav"))
    logger_log(&g_sys_logger, "Music: music.wav");

  /* 5. Matrices */
  Mat4 projection =
      mat4_perspective(45.0f * (M_PI / 180.0f), 480.0f / 272.0f, 0.1f, 100.0f);
  Mat4 view = mat4_translate(0, 0, -3.5f);

  InputState input = {0};
  bool running = true;
  g_running_flag = true; /* reset in case of re-entry */

  /* Trackball rotation state */
  Mat4 model_rot = mat4_identity();
  float velX = 0.012f;
  float velY = 0.018f;

  const float MANUAL_ACCEL = 0.003f;
  const float MAX_VEL = 0.07f;
  const float AUTO_VEL_X = 0.012f;
  const float AUTO_VEL_Y = 0.018f;
  const float AUTO_LERP = 0.015f;
  const float IDLE_TIMEOUT = 10.0f;

  float last_btn_time = platform_get_time() - IDLE_TIMEOUT;
  float last_frame_time = platform_get_time();
  bool screensaver = true;

  /* ---- Main loop ---- */
  while (running && g_running_flag) {
    float now = platform_get_time();
    float dt = now - last_frame_time;
    last_frame_time = now;

    logger_update(&g_sys_logger, dt);
    logger_update(&g_btn_logger, dt);

    platform_poll_input(&input);
    if (input.buttons_held & BN_EXIT || input.buttons_held & BN_SELECT)
      running = false;

    fps_update(dt);

    /* D-Pad: acceleration */
    bool btn = false;
    if (input.buttons_held & BN_UP) {
      velX -= MANUAL_ACCEL;
      btn = true;
    }
    if (input.buttons_held & BN_DOWN) {
      velX += MANUAL_ACCEL;
      btn = true;
    }
    if (input.buttons_held & BN_LEFT) {
      velY -= MANUAL_ACCEL;
      btn = true;
    }
    if (input.buttons_held & BN_RIGHT) {
      velY += MANUAL_ACCEL;
      btn = true;
    }

    if (btn) {
      if (velX > MAX_VEL)
        velX = MAX_VEL;
      if (velX < -MAX_VEL)
        velX = -MAX_VEL;
      if (velY > MAX_VEL)
        velY = MAX_VEL;
      if (velY < -MAX_VEL)
        velY = -MAX_VEL;
      last_btn_time = platform_get_time();
      screensaver = false;
    }

    /* Screensaver lerp */
    if (!screensaver && (platform_get_time() - last_btn_time) > IDLE_TIMEOUT) {
      screensaver = true;
      logger_log(&g_sys_logger, "Screensaver ON");
    }
    if (screensaver) {
      velX += (AUTO_VEL_X - velX) * AUTO_LERP;
      velY += (AUTO_VEL_Y - velY) * AUTO_LERP;
    }

    /* Trackball integration */
    if (velX != 0.0f)
      model_rot = mat4_multiply(mat4_rotate_x(velX), model_rot);
    if (velY != 0.0f)
      model_rot = mat4_multiply(mat4_rotate_y(velY), model_rot);

    /* Button logger */
    if (input.buttons_pressed & BN_UP)
      logger_log(&g_btn_logger, "Button: UP");
    if (input.buttons_pressed & BN_DOWN)
      logger_log(&g_btn_logger, "Button: DOWN");
    if (input.buttons_pressed & BN_LEFT)
      logger_log(&g_btn_logger, "Button: LEFT");
    if (input.buttons_pressed & BN_RIGHT)
      logger_log(&g_btn_logger, "Button: RIGHT");
    if (input.buttons_pressed & BN_TRIANGLE) {
      g_color_idx = (g_color_idx + 1) % 5;
      logger_log(&g_btn_logger, "Button: TRIANGLE");
      logger_log(&g_sys_logger, "Color changed: %d", g_color_idx + 1);
    }
    if (input.buttons_pressed & BN_CROSS) {
      logger_log(&g_btn_logger, "Button: CROSS");
      bool is_psp = (platform_get_backend_name()[0] == 'P'); /* "PSP GU" */
      switch_backends(!is_psp);
      logger_log(&g_sys_logger, "Backend: %s", platform_get_backend_name());
      logger_log(&g_sys_logger, "Audio:   %s", audio_get_backend_name());

      /* Sync time to avoid huge DT jump after switching clocks */
      last_frame_time = platform_get_time();
    }
    if (input.buttons_pressed & BN_START)
      logger_log(&g_btn_logger, "Button: START");
    if (input.buttons_pressed & BN_SELECT)
      logger_log(&g_btn_logger, "Button: SELECT");

    /* SQUARE — cycle shapes */
    if (input.buttons_pressed & BN_SQUARE) {
      current_shape = (current_shape + 1) % 3;
      logger_log(&g_btn_logger, "Button: SQUARE");
      if (current_shape == SHAPE_CUBE) {
        current_mesh = mesh_create_cube(1.2f);
        logger_log(&g_sys_logger, "Shape: Cube");
      }
      if (current_shape == SHAPE_PYRAMID) {
        current_mesh = mesh_create_pyramid(1.2f);
        logger_log(&g_sys_logger, "Shape: Pyramid");
      }
      if (current_shape == SHAPE_SPHERE) {
        current_mesh = mesh_create_sphere(0.8f, 12);
        logger_log(&g_sys_logger, "Shape: Sphere");
      }
    }

    /* CIRCLE — toggle wireframe / solid */
    if (input.buttons_pressed & BN_CIRCLE) {
      current_mode =
          (current_mode == RENDER_WIREFRAME) ? RENDER_SOLID : RENDER_WIREFRAME;
      logger_log(&g_btn_logger, "Button: CIRCLE");
      logger_log(&g_sys_logger, "Mode: %s",
                 current_mode == RENDER_SOLID ? "SOLID" : "WIRE");
    }

    /* ---- Render ---- */
    platform_clear(COLOR_RGB(45, 45, 48));
    mesh_render(&current_mesh, model_rot, view, projection,
                g_colors[g_color_idx], current_mode);

    /* UI */
    logger_render(&g_sys_logger);
    logger_render(&g_btn_logger);
    controls_render();
    fps_render();

    /* Backend indicator (Top-Right) */
    char backend_info[32];
    snprintf(backend_info, sizeof(backend_info), "BACKEND: %s",
             platform_get_backend_name());
    text_draw_string_right(470, 10, backend_info, COLOR_RGB(150, 255, 255));

    platform_present();
  }

  audio_shutdown();
  platform_shutdown();
  return 0;
}
