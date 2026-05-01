#include "engine/math3d.h"
#include <string.h>

Vec3 vec3_add(Vec3 a, Vec3 b) {
  return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 vec3_sub(Vec3 a, Vec3 b) {
  return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 vec3_mul(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }

float vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 vec3_cross(Vec3 a, Vec3 b) {
  return (Vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x};
}

Vec3 vec3_normalize(Vec3 v) {
  float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  if (len < 0.0001f)
    return v;
  return (Vec3){v.x / len, v.y / len, v.z / len};
}

Mat4 mat4_identity() {
  Mat4 res = {0};
  res.m[0][0] = 1.0f;
  res.m[1][1] = 1.0f;
  res.m[2][2] = 1.0f;
  res.m[3][3] = 1.0f;
  return res;
}

Mat4 mat4_multiply(Mat4 a, Mat4 b) {
  Mat4 res = {0};
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        res.m[i][j] += a.m[i][k] * b.m[k][j];
      }
    }
  }
  return res;
}

Mat4 mat4_rotate_x(float angle) {
  Mat4 res = mat4_identity();
  float s = sinf(angle);
  float c = cosf(angle);
  res.m[1][1] = c;
  res.m[1][2] = -s;
  res.m[2][1] = s;
  res.m[2][2] = c;
  return res;
}

Mat4 mat4_rotate_y(float angle) {
  Mat4 res = mat4_identity();
  float s = sinf(angle);
  float c = cosf(angle);
  res.m[0][0] = c;
  res.m[0][2] = s;
  res.m[2][0] = -s;
  res.m[2][2] = c;
  return res;
}

Mat4 mat4_rotate_z(float angle) {
  Mat4 res = mat4_identity();
  float s = sinf(angle);
  float c = cosf(angle);
  res.m[0][0] = c;
  res.m[0][1] = -s;
  res.m[1][0] = s;
  res.m[1][1] = c;
  return res;
}

Mat4 mat4_translate(float x, float y, float z) {
  Mat4 res = mat4_identity();
  res.m[0][3] = x;
  res.m[1][3] = y;
  res.m[2][3] = z;
  return res;
}

Mat4 mat4_perspective(float fov, float aspect, float near, float far) {
  Mat4 res = {0};
  float tanHalfFov = tanf(fov / 2.0f);
  res.m[0][0] = 1.0f / (aspect * tanHalfFov);
  res.m[1][1] = 1.0f / tanHalfFov;
  res.m[2][2] = -(far + near) / (far - near);
  res.m[2][3] = -(2.0f * far * near) / (far - near);
  res.m[3][2] = -1.0f;
  return res;
}

Vec3 mat4_multiply_vec3(Mat4 m, Vec3 v) {
  float w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3];
  return (Vec3){
      (m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3]) / w,
      (m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3]) / w,
      (m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3]) / w};
}

Vec4 mat4_multiply_vec4(Mat4 m, Vec4 v) {
  return (Vec4){
      m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w,
      m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w,
      m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w,
      m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w};
}
