/*
COSC 3307 - Project: Interactive Fireworks Display
Scene geometry — GPU buffers for the ground plane and skybox sphere,
plus the procedural ground texture.
*/
#ifndef SCENE_H_
#define SCENE_H_

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// ── GPU geometry handle ─────────────────────────────────────────────────────
// Same typedef as used in the professor's tutorials (T3/T4).
typedef struct Geometry {
    GLuint vbo;   // vertex buffer
    GLuint ibo;   // index buffer
    GLuint vao;   // vertex array object
    GLuint size;  // number of indices to draw
} Geometry;

// ── Vertex layouts ──────────────────────────────────────────────────────────

// Ground plane vertex: position (12) + normal (12) + UV (8) = 32 bytes
struct GroundVertex {
    glm::vec3 pos;     // offset  0
    glm::vec3 normal;  // offset 12
    glm::vec2 uv;      // offset 24
};

// Skybox vertex: position only — used as direction for gradient + star lookup
struct SkyVertex {
    glm::vec3 pos;
};

// ── 3D object vertex layout ─────────────────────────────────────────────────
// Used by all Phong-shaded scene objects (fountain, rocket, moon, road, human).
// Layout: pos(12) + normal(12) + color(12) = 36 bytes — all plain floats.
struct ObjectVertex {
    glm::vec3 pos;     // offset  0
    glm::vec3 normal;  // offset 12
    glm::vec3 color;   // offset 24
};

// ── Scene creation functions ────────────────────────────────────────────────

// Generate a 128x128 procedural grass texture on the GPU and return its ID.
GLuint CreateGroundTexture();

// Build a quad at y=0 covering [-halfSize, halfSize] on X and Z axes.
Geometry* CreateGroundPlane(float halfSize = 300.0f);

// Build an inside-out sphere (skybox). Large radius keeps it behind all geometry.
Geometry* CreateSkyboxSphere(float radius = 2000.0f);

// ── 3D Primitive builders ───────────────────────────────────────────────────

// Upright cylinder — base at y=0, top at y=height, given colour.
Geometry* CreateCylinder(float radius, float height, int slices, glm::vec3 color);

// Upright cone — base at y=0, tip at y=height.
Geometry* CreateCone(float radius, float height, int slices, glm::vec3 color);

// UV sphere centred at origin.
Geometry* CreateSphere(float radius, int stacks, int slices, glm::vec3 color);

// Flat horizontal quad (road strip).
// Runs along the Z axis, centred on the X axis.
// halfW = half-width on X, halfL = half-length on Z, yOffset = height above ground.
Geometry* CreateRoadStrip(float halfW, float halfL, float yOffset, glm::vec3 color);

#endif // SCENE_H_
