/*
COSC 3307 - Project: Interactive Fireworks Display
FireworksApp — main application class.
Owns the window, camera, shaders, scene geometry, and particle system.
Follows the professor's OpenGLApp pattern from T3/T4.
*/
#ifndef APP_H_
#define APP_H_

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>

#include <camera.h>
#include <particle.h>
#include <scene.h>

class FireworksApp {
public:
    FireworksApp();
    ~FireworksApp();

    // Creates the window, loads shaders, builds scene, runs the main loop.
    // Returns 0 on clean exit, 1 on error.
    int Init();

private:
    // ── Window ────────────────────────────────────────────────────────────
    GLFWwindow* window_;

    // ── Camera ────────────────────────────────────────────────────────────
    Camera*   camera_;
    glm::mat4 view_matrix_;
    glm::mat4 projection_matrix_;

    // ── Shaders ───────────────────────────────────────────────────────────
    GLuint ground_shader_;
    GLuint particle_shader_;
    GLuint skybox_shader_;
    GLuint object_shader_;   // Phong shader for all 3D scene objects

    // ── Ground / Sky ──────────────────────────────────────────────────────
    Geometry* ground_;
    Geometry* skybox_;
    GLuint    ground_tex_;
    glm::vec3 scene_light_pos_;   // matches moon world position

    // ── Fountain vase (3 stacked cylinders) ──────────────────────────────
    Geometry* vase_base_;    // wide flat base disc
    Geometry* vase_column_;  // narrow centre column
    Geometry* vase_bowl_;    // wide shallow bowl at the top

    // ── Moon (sphere, emissive — also the scene light source) ─────────────
    Geometry* moon_;

    // ── Road (two crossing strips) ────────────────────────────────────────
    Geometry* road_z_;   // runs along Z axis
    Geometry* road_x_;   // runs along X axis

    // ── Human figures (2 people: head sphere + body cylinder) ─────────────
    Geometry* human_head_;   // shared sphere geometry
    Geometry* human_body_;   // shared cylinder geometry
    Geometry* human_leg_;    // thin cylinder (reused for all 4 legs)

    // ── Rocket mesh (cone nose + cylinder body — drawn per active rocket) ──
    Geometry* rocket_body_;
    Geometry* rocket_nose_;

    // ── Particle system ───────────────────────────────────────────────────
    ParticleSystem* particle_sys_;
    GLuint particle_vao_;
    GLuint particle_vbo_;
    GLuint particle_ibo_;

    // ── Timing ────────────────────────────────────────────────────────────
    double last_time_;

    // ── Internal setup ────────────────────────────────────────────────────
    void InitParticleGPU();

    // ── Per-frame render ──────────────────────────────────────────────────
    void RenderSkybox();
    void RenderGround();
    void RenderParticles();
    void RenderObjects();   // fountain vase, road, humans, moon
    void RenderRockets();   // visible rocket mesh for active TRAIL rockets

    // Helper: set attribs + uniforms and draw one ObjectVertex geometry
    void DrawObject(Geometry* geom, const glm::mat4& worldMat, float shininess, float emissive = 0.0f);

    void PrintControls();

    // ── GLFW callbacks ────────────────────────────────────────────────────
    // Static so GLFW (a C library) can store them as plain function pointers.
    // They forward to instance_ to reach the app state.
    static FireworksApp* instance_;
    static void KeyCallback(GLFWwindow* win, int key, int scancode,
                            int action, int mods);
    static void ResizeCallback(GLFWwindow* win, int width, int height);
};

#endif // APP_H_
