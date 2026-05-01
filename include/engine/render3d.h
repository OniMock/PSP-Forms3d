#ifndef RENDER3D_H
#define RENDER3D_H

#include "math3d.h"
#include "platform/platform.h"

typedef enum { RENDER_WIREFRAME, RENDER_SOLID } RenderMode;

typedef enum { SHAPE_CUBE, SHAPE_PYRAMID, SHAPE_SPHERE } ShapeType;

typedef struct {
  int v[3];    // Vertex indices
  float depth; // Used for sorting
} Face;

typedef struct {
  Vec3 vertices[128]; // Max vertices
  int vertex_count;
  int edges[256][2]; // Max edges
  int edge_count;
  Face faces[256]; // Max faces
  int face_count;
} Mesh;

/**
 * @brief Create a cube mesh
 */
Mesh mesh_create_cube(float size);
/**
 * @brief Create a pyramid mesh
 */
Mesh mesh_create_pyramid(float size);
/**
 * @brief Create a sphere mesh
 */
Mesh mesh_create_sphere(float radius, int segments);

/**
 * @brief Render a mesh
 * @param mesh Mesh to render
 * @param model Model matrix
 * @param view View matrix
 * @param projection Projection matrix
 * @param color Color of the mesh
 * @param mode Render mode
 */
void mesh_render(Mesh *mesh, Mat4 model, Mat4 view, Mat4 projection,
                 Color color, RenderMode mode);

#endif // RENDER3D_H
