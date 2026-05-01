/**
 * @file platform_psp.c
 * @brief PSP GU (Graphics Utility) renderer backend — exports BACKEND_PSP_GU
 * vtable.
 *
 * Uses the PSP SDK natively:
 *   - pspgu / pspgum  for 2-D hardware-accelerated drawing
 *   - pspctrl          for input
 *   - psprtc           for timing
 *   - pspdisplay       for VBlank / buffer swap
 *
 * All rendering is done in 2-D transform mode: render3d.c already projects
 * vertices to screen space; we just fill triangles / draw lines via the GU
 * hardware rather than via SDL's software rasterizer.
 *
 * Frame lifecycle:
 *   psp_gu_clear()   → sceGuStart() + sceGuClear()      (opens the display
 * list) psp_gu_draw_*()  → append to g_vbuf (no GU call yet)
 *   psp_gu_present() → sceGuDrawArray() once per type, finish, sync, swap
 *
 * KEY FIX: We never call sceGuDrawArray inside draw_line/draw_triangle/
 * draw_pixel because the GU processes the list asynchronously. Instead,
 * vertices are accumulated in a large per-frame buffer and submitted all
 * at once in present(). This eliminates the CPU/GU data race that caused
 * rendering artifacts (streak lines, corrupted text) on real hardware.
 */

#include "platform/renderer_backend.h"

#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <string.h>

/* ---- VRAM layout ---- */
#define PSP_BUF_WIDTH  512
#define PSP_SCR_WIDTH  480
#define PSP_SCR_HEIGHT 272
#define FRAME_BYTES    (PSP_BUF_WIDTH * PSP_SCR_HEIGHT * 4) /* 32-bit colour */
#define VRAM_DRAW_BUF  ((void *)0x00000000)
#define VRAM_DISP_BUF  ((void *)FRAME_BYTES)
#define VRAM_DEPTH_BUF ((void *)(FRAME_BYTES * 2)) /* 16-bit depth */

/* Display list — 1 MB, DWORD-aligned in uncached RAM */
static unsigned int __attribute__((aligned(64))) g_dlist[262144];

static bool g_initialized = false;

/* ---- Vertex type for 2-D coloured primitives ---- */
typedef struct {
  unsigned int color; /* GU_COLOR_8888 == RGBA same layout as our Color */
  float x, y, z;
} __attribute__((aligned(16))) Vert2D;

/*
 * Large per-frame vertex accumulation buffer.
 * Worst case: sphere solid = ~200 faces * 3 verts = 600 triangles vertices
 *           + wireframe edges = ~250 * 2 = 500 line vertices
 *           + text pixels = lines * chars * 8*8 = ~1000 point vertices
 * We split into three sub-buffers (triangles, lines, points) that are
 * flushed in present() with a single draw call each.
 *
 * MAX chosen conservatively for PSP 1000 (24 MB RAM):
 *   3 * 2048 * 16 bytes = 96 KB — well within safe limits.
 */
#define MAX_TRI_VERTS   2048  /* triangles: multiple of 3 */
#define MAX_LINE_VERTS  2048  /* lines: multiple of 2    */
#define MAX_POINT_VERTS 4096  /* pixels for text          */

static Vert2D __attribute__((aligned(64))) g_tri_buf[MAX_TRI_VERTS];
static Vert2D __attribute__((aligned(64))) g_line_buf[MAX_LINE_VERTS];
static Vert2D __attribute__((aligned(64))) g_pt_buf[MAX_POINT_VERTS];

static int g_tri_count  = 0; /* in vertices, always multiple of 3 */
static int g_line_count = 0; /* in vertices, always multiple of 2 */
static int g_pt_count   = 0;

/* ---- Init / Shutdown ---- */

static bool psp_gu_init(const char *title, int w, int h) {
  (void)title;
  (void)w;
  (void)h;

  sceGuInit();

  sceGuStart(GU_DIRECT, g_dlist);

  sceGuDrawBuffer(GU_PSM_8888, VRAM_DRAW_BUF, PSP_BUF_WIDTH);
  sceGuDispBuffer(PSP_SCR_WIDTH, PSP_SCR_HEIGHT, VRAM_DISP_BUF, PSP_BUF_WIDTH);
  sceGuDepthBuffer(VRAM_DEPTH_BUF, PSP_BUF_WIDTH);

  sceGuOffset(2048 - (PSP_SCR_WIDTH / 2), 2048 - (PSP_SCR_HEIGHT / 2));
  sceGuViewport(2048, 2048, PSP_SCR_WIDTH, PSP_SCR_HEIGHT);

  /* 2-D mode: disable depth test — render3d already sorts by depth */
  sceGuDisable(GU_DEPTH_TEST);

  sceGuScissor(0, 0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT);
  sceGuEnable(GU_SCISSOR_TEST);

  sceGuShadeModel(GU_SMOOTH);
  sceGuDisable(GU_CULL_FACE);
  sceGuDisable(GU_TEXTURE_2D);
  sceGuDisable(GU_LIGHTING);

  sceGuFinish();
  sceGuSync(0, 0);

  sceDisplayWaitVblankStart();
  sceGuDisplay(GU_TRUE);

  /* Set up controller */
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

  g_initialized = true;
  return true;
}

static bool psp_gu_init_audio(void) {
  /* pspaudio channels are reserved per-use; nothing global to init here */
  return true;
}

static void psp_gu_shutdown(void) {
  if (!g_initialized)
    return;
  sceGuDisplay(GU_FALSE);
  sceGuTerm();
  g_initialized = false;
}

/* ---- Drawing primitives (CPU side only — no GU calls here) ---- */

/*
 * clear() opens the display list for a new frame AND resets the vertex
 * accumulators. Every subsequent draw_* call appends to the CPU buffers.
 * present() flushes those buffers to the GU in one batch.
 */
static void psp_gu_clear(Color color) {
  /* Reset accumulators */
  g_tri_count  = 0;
  g_line_count = 0;
  g_pt_count   = 0;

  sceGuStart(GU_DIRECT, g_dlist);
  sceGuClearColor(color);
  sceGuClearDepth(0);
  sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
}

static void psp_gu_draw_line(int x1, int y1, int x2, int y2, Color color) {
  if (g_line_count + 2 > MAX_LINE_VERTS)
    return;
  g_line_buf[g_line_count++] = (Vert2D){color, (float)x1, (float)y1, 0.0f};
  g_line_buf[g_line_count++] = (Vert2D){color, (float)x2, (float)y2, 0.0f};
}

static void psp_gu_draw_triangle(int x1, int y1, int x2, int y2, int x3,
                                 int y3, Color color) {
  if (g_tri_count + 3 > MAX_TRI_VERTS)
    return;
  g_tri_buf[g_tri_count++] = (Vert2D){color, (float)x1, (float)y1, 0.0f};
  g_tri_buf[g_tri_count++] = (Vert2D){color, (float)x2, (float)y2, 0.0f};
  g_tri_buf[g_tri_count++] = (Vert2D){color, (float)x3, (float)y3, 0.0f};
}

static void psp_gu_draw_pixel(int x, int y, Color color) {
  if (g_pt_count + 1 > MAX_POINT_VERTS)
    return;
  g_pt_buf[g_pt_count++] = (Vert2D){color, (float)x, (float)y, 0.0f};
}

static void psp_gu_present(void) {
  /* Flush triangles */
  if (g_tri_count > 0) {
    sceKernelDcacheWritebackRange(g_tri_buf, sizeof(Vert2D) * g_tri_count);
    sceGuDrawArray(GU_TRIANGLES,
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   g_tri_count, 0, g_tri_buf);
  }

  /* Flush lines */
  if (g_line_count > 0) {
    sceKernelDcacheWritebackRange(g_line_buf, sizeof(Vert2D) * g_line_count);
    sceGuDrawArray(GU_LINES,
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   g_line_count, 0, g_line_buf);
  }

  /* Flush pixels (text) */
  if (g_pt_count > 0) {
    sceKernelDcacheWritebackRange(g_pt_buf, sizeof(Vert2D) * g_pt_count);
    sceGuDrawArray(GU_POINTS,
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   g_pt_count, 0, g_pt_buf);
  }

  sceGuFinish();
  sceGuSync(0, 0);
  sceDisplayWaitVblankStart();
  sceGuSwapBuffers();
}

/* ---- Input ---- */

static void psp_gu_poll_input(InputState *state) {
  static uint32_t prev_buttons = 0;
  SceCtrlData pad;
  sceCtrlReadBufferPositive(&pad, 1);

  uint32_t current = 0;
  if (pad.Buttons & PSP_CTRL_UP)
    current |= BN_UP;
  if (pad.Buttons & PSP_CTRL_DOWN)
    current |= BN_DOWN;
  if (pad.Buttons & PSP_CTRL_LEFT)
    current |= BN_LEFT;
  if (pad.Buttons & PSP_CTRL_RIGHT)
    current |= BN_RIGHT;
  if (pad.Buttons & PSP_CTRL_CROSS)
    current |= BN_CROSS;
  if (pad.Buttons & PSP_CTRL_CIRCLE)
    current |= BN_CIRCLE;
  if (pad.Buttons & PSP_CTRL_SQUARE)
    current |= BN_SQUARE;
  if (pad.Buttons & PSP_CTRL_TRIANGLE)
    current |= BN_TRIANGLE;
  if (pad.Buttons & PSP_CTRL_START)
    current |= BN_START;
  if (pad.Buttons & PSP_CTRL_SELECT)
    current |= BN_SELECT;

  state->buttons_pressed = current & ~prev_buttons;
  state->buttons_held    = current;
  prev_buttons           = current;
}

/* ---- Timing ---- */

static float psp_gu_get_time(void) {
  static u64 start_tick = 0;
  static u32 resolution = 0;
  if (!start_tick) {
    sceRtcGetCurrentTick(&start_tick);
    resolution = sceRtcGetTickResolution();
  }
  u64 now;
  sceRtcGetCurrentTick(&now);
  return (float)(now - start_tick) / (float)resolution;
}

static int psp_gu_get_width(void)  { return PSP_SCR_WIDTH; }
static int psp_gu_get_height(void) { return PSP_SCR_HEIGHT; }

/* ---- Exported vtable ---- */

const RendererBackend BACKEND_PSP_GU = {
    .name        = "PSP GU",
    .init        = psp_gu_init,
    .init_audio  = psp_gu_init_audio,
    .shutdown    = psp_gu_shutdown,
    .clear       = psp_gu_clear,
    .draw_line   = psp_gu_draw_line,
    .draw_triangle = psp_gu_draw_triangle,
    .draw_pixel  = psp_gu_draw_pixel,
    .present     = psp_gu_present,
    .poll_input  = psp_gu_poll_input,
    .get_time    = psp_gu_get_time,
    .get_width   = psp_gu_get_width,
    .get_height  = psp_gu_get_height,
};
