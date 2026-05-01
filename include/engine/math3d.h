#ifndef MATH3D_H
#define MATH3D_H

#include <math.h>

typedef struct {
  float x, y;
} Vec2;

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  float x, y, z, w;
} Vec4;

typedef struct {
  float m[4][4];
} Mat4;

/**
 * @brief Add two vectors
 * @param a First vector
 * @param b Second vector
 * @return Resultant vector
 */
Vec3 vec3_add(Vec3 a, Vec3 b);

/**
 * @brief Subtract two vectors
 * @param a First vector
 * @param b Second vector
 * @return Resultant vector
 */
Vec3 vec3_sub(Vec3 a, Vec3 b);

/**
 * @brief Multiply vector by scalar
 * @param v Vector
 * @param s Scalar
 * @return Resultant vector
 */
Vec3 vec3_mul(Vec3 v, float s);

/**
 * @brief Dot product of two vectors
 * @param a First vector
 * @param b Second vector
 * @return Dot product
 */
float vec3_dot(Vec3 a, Vec3 b);

/**
 * @brief Cross product of two vectors
 * @param a First vector
 * @param b Second vector
 * @return Cross product
 */
Vec3 vec3_cross(Vec3 a, Vec3 b);

/**
 * @brief Normalize a vector
 * @param v Vector
 * @return Normalized vector
 */
Vec3 vec3_normalize(Vec3 v);

/**
 * @brief Create an identity matrix
 * @return Identity matrix
 */
Mat4 mat4_identity();

/**
 * @brief Multiply two matrices
 * @param a First matrix
 * @param b Second matrix
 * @return Resultant matrix
 */
Mat4 mat4_multiply(Mat4 a, Mat4 b);

/**
 * @brief Rotate a matrix around the X axis
 * @param angle Angle in radians
 * @return Rotated matrix
 */
Mat4 mat4_rotate_x(float angle);
/**
 * @brief Rotate a matrix around the Y axis
 * @param angle Angle in radians
 * @return Rotated matrix
 */
Mat4 mat4_rotate_y(float angle);
/**
 * @brief Rotate a matrix around the Z axis
 * @param angle Angle in radians
 * @return Rotated matrix
 */
Mat4 mat4_rotate_z(float angle);
/**
 * @brief Translate a matrix
 * @param x Translation on X axis
 * @param y Translation on Y axis
 * @param z Translation on Z axis
 * @return Translated matrix
 */
Mat4 mat4_translate(float x, float y, float z);
/**
 * @brief Create a perspective matrix
 * @param fov Field of view
 * @param aspect Aspect ratio
 * @param near Near clipping plane
 * @param far Far clipping plane
 * @return Perspective matrix
 */
Mat4 mat4_perspective(float fov, float aspect, float near, float far);
/**
 * @brief Multiply a matrix by a vector
 * @param m Matrix
 * @param v Vector
 * @return Resultant vector
 */
Vec3 mat4_multiply_vec3(Mat4 m, Vec3 v);
/**
 * @brief Multiply a matrix by a vector
 * @param m Matrix
 * @param v Vector
 * @return Resultant vector
 */
Vec4 mat4_multiply_vec4(Mat4 m, Vec4 v);

#endif // MATH3D_H
