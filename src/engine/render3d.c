#include "engine/render3d.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const Vec3 LIGHT_DIR = {0.577f, 0.577f, -0.577f};
#define AMBIENT 0.25f

Mesh mesh_create_cube(float size) {
  Mesh m = {0};
  float s = size / 2.0f;
  Vec3 verts[] = {{-s, -s, s},  {s, -s, s},  {s, s, s},  {-s, s, s},
                  {-s, -s, -s}, {s, -s, -s}, {s, s, -s}, {-s, s, -s}};
  for (int i = 0; i < 8; i++)
    m.vertices[i] = verts[i];
  m.vertex_count = 8;
  int edges[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                    {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
  for (int i = 0; i < 12; i++) {
    m.edges[i][0] = edges[i][0];
    m.edges[i][1] = edges[i][1];
  }
  m.edge_count = 12;
  int faces[][3] = {{0, 1, 2}, {0, 2, 3}, {5, 4, 7}, {5, 7, 6},
                    {4, 0, 3}, {4, 3, 7}, {1, 5, 6}, {1, 6, 2},
                    {3, 2, 6}, {3, 6, 7}, {4, 5, 1}, {4, 1, 0}};
  for (int i = 0; i < 12; i++) {
    m.faces[i].v[0] = faces[i][0];
    m.faces[i].v[1] = faces[i][1];
    m.faces[i].v[2] = faces[i][2];
  }
  m.face_count = 12;
  return m;
}

Mesh mesh_create_pyramid(float size) {
  Mesh m = {0};
  float s = size / 2.0f;
  Vec3 verts[] = {
      {0, s, 0}, {-s, -s, s}, {s, -s, s}, {s, -s, -s}, {-s, -s, -s}};
  for (int i = 0; i < 5; i++)
    m.vertices[i] = verts[i];
  m.vertex_count = 5;
  int edges[][2] = {{0, 1}, {0, 2}, {0, 3}, {0, 4},
                    {1, 2}, {2, 3}, {3, 4}, {4, 1}};
  for (int i = 0; i < 8; i++) {
    m.edges[i][0] = edges[i][0];
    m.edges[i][1] = edges[i][1];
  }
  m.edge_count = 8;
  int faces[][3] = {{0, 1, 2}, {0, 2, 3}, {0, 3, 4},
                    {0, 4, 1}, {1, 3, 2}, {1, 4, 3}};
  for (int i = 0; i < 6; i++) {
    m.faces[i].v[0] = faces[i][0];
    m.faces[i].v[1] = faces[i][1];
    m.faces[i].v[2] = faces[i][2];
  }
  m.face_count = 6;
  return m;
}

Mesh mesh_create_sphere(float radius, int segments) {
  Mesh m = {0};
  if (segments > 10)
    segments = 10;
  int rings = segments / 2;
  for (int r = 0; r <= rings; r++) {
    float phi = (float)M_PI * r / rings;
    for (int s = 0; s <= segments; s++) {
      float theta = 2.0f * (float)M_PI * s / segments;
      if (m.vertex_count < 128)
        m.vertices[m.vertex_count++] =
            (Vec3){radius * sinf(phi) * cosf(theta), radius * cosf(phi),
                   radius * sinf(phi) * sinf(theta)};
    }
  }
  for (int r = 0; r < rings; r++) {
    for (int s = 0; s < segments; s++) {
      int i0 = r * (segments + 1) + s, i1 = i0 + 1, i2 = i0 + (segments + 1),
          i3 = i2 + 1;
      if (m.edge_count < 250) {
        m.edges[m.edge_count][0] = i0;
        m.edges[m.edge_count++][1] = i1;
      }
      if (m.edge_count < 250) {
        m.edges[m.edge_count][0] = i0;
        m.edges[m.edge_count++][1] = i2;
      }
      if (m.face_count < 250) {
        m.faces[m.face_count].v[0] = i0;
        m.faces[m.face_count].v[1] = i1;
        m.faces[m.face_count++].v[2] = i2;
      }
      if (m.face_count < 250) {
        m.faces[m.face_count].v[0] = i1;
        m.faces[m.face_count].v[1] = i3;
        m.faces[m.face_count++].v[2] = i2;
      }
    }
  }
  return m;
}

void mesh_render(Mesh *mesh, Mat4 model, Mat4 view, Mat4 projection,
                 Color color, RenderMode mode) {
  Vec2 proj[128];
  Vec3 world[128];
  Mat4 mvp = mat4_multiply(projection, mat4_multiply(view, model));
  int sw = platform_get_width();
  int sh = platform_get_height();

  // Camera world position (from pure-translation view matrix:
  // translate(0,0,-d))
  Vec3 cam = {-view.m[0][3], -view.m[1][3], -view.m[2][3]};

  for (int i = 0; i < mesh->vertex_count; i++) {
    Vec3 v = mesh->vertices[i];
    world[i] = mat4_multiply_vec3(model, v);
    Vec4 p = mat4_multiply_vec4(mvp, (Vec4){v.x, v.y, v.z, 1.0f});
    if (p.w != 0.0f) {
      p.x /= p.w;
      p.y /= p.w;
      p.z /= p.w;
    }
    proj[i].x = (p.x + 1.0f) * 0.5f * sw;
    proj[i].y = (1.0f - p.y) * 0.5f * sh;
  }

  // ---- WIREFRAME ----
  if (mode == RENDER_WIREFRAME) {
    for (int i = 0; i < mesh->edge_count; i++) {
      int a = mesh->edges[i][0], b = mesh->edges[i][1];
      platform_draw_line(proj[a].x, proj[a].y, proj[b].x, proj[b].y, color);
    }
    return;
  }

  // ---- SOLID: single-pass with world-space back-face culling ----
  uint8_t base_r = color & 0xFF;
  uint8_t base_g = (color >> 8) & 0xFF;
  uint8_t base_b = (color >> 16) & 0xFF;

  for (int i = 0; i < mesh->face_count; i++) {
    int i0 = mesh->faces[i].v[0];
    int i1 = mesh->faces[i].v[1];
    int i2 = mesh->faces[i].v[2];

    // World-space face normal
    Vec3 e1 = vec3_sub(world[i1], world[i0]);
    Vec3 e2 = vec3_sub(world[i2], world[i0]);
    Vec3 raw = vec3_cross(e1, e2);
    float len2 = raw.x * raw.x + raw.y * raw.y + raw.z * raw.z;
    if (len2 < 1e-8f)
      continue; // skip degenerate (sphere poles)

    // Back-face culling: skip if face points away from camera
    Vec3 fc = {(world[i0].x + world[i1].x + world[i2].x) * 0.3333f,
               (world[i0].y + world[i1].y + world[i2].y) * 0.3333f,
               (world[i0].z + world[i1].z + world[i2].z) * 0.3333f};
    if (vec3_dot(raw, vec3_sub(cam, fc)) <= 0.0f)
      continue;

    // Flat shading: Lambertian + ambient
    Vec3 normal = vec3_mul(raw, 1.0f / sqrtf(len2));
    float diff = vec3_dot(normal, LIGHT_DIR);
    if (diff < 0.0f)
      diff = 0.0f;
    float intensity = AMBIENT + (1.0f - AMBIENT) * diff;

    uint8_t r = (uint8_t)(base_r * intensity);
    uint8_t g = (uint8_t)(base_g * intensity);
    uint8_t b = (uint8_t)(base_b * intensity);

    platform_draw_triangle((int)proj[i0].x, (int)proj[i0].y, (int)proj[i1].x,
                           (int)proj[i1].y, (int)proj[i2].x, (int)proj[i2].y,
                           COLOR_RGB(r, g, b));
  }
}
