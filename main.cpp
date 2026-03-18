/*
COSC 3307 - Project: Interactive Fireworks Display
Sana Begum | Nipissing University

Scene: Night sky backdrop, Phong-shaded ground plane, particle-based fireworks.

--- Phase 1 (50% complete) ---
[x] GLFW window + GLEW + camera (identical setup to A2)
[x] Particle pool: rockets fly upward then explode (Starburst type)
[x] Physics: gravity, air drag, linear alpha fade, point-size shrink
[x] Rendering: dynamic VBO updated per frame, GL_POINTS with additive blending
[x] Circular glow shape via gl_PointCoord discard in fragment shader
[x] Ground plane with Phong shading (ambient + diffuse + specular)
[x] Night-blue background
[x] Camera controls (arrows, A/D roll, W/S move)
[x] Keyboard launch (SPACE / 1) + auto-launch on startup

--- Phase 2 (TODO) ---
[ ] Fountain, Rocket-with-trail, Cascade firework types
[ ] Particle glow texture (SOIL) with alpha channel
[ ] Ground texture (grass/concrete via SOIL)
[ ] Skybox sphere with gradient night texture
[ ] Dynamic point lights attached to active particle clusters
[ ] Multi-firework orchestration / choreography timing

Controls:
  Arrow Up/Down   : Pitch camera
  Arrow Left/Right: Yaw camera
  A / D           : Roll camera
  W / S           : Move forward / backward
  SPACE or 1      : Launch starburst firework
  Q               : Quit
*/

#include <iostream>
#include <stdexcept>
#include <string>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <camera.h>
#include <particle.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>

// -----------------------------------------------------------------------
// IMPORTANT: Change this path to point to Project_Fireworks/resource/ on YOUR machine.
// -----------------------------------------------------------------------
#define SHADER_DIRECTORY "C:/Users/sanam/OneDrive - Nipissing University/Desktop/3D_computer.graphics/Project_Fireworks/resource/"

#define PrintException(exception_object)\
    std::cerr << exception_object.what() << std::endl

// Window
const std::string window_title_g  = "COSC 3307 - Interactive Fireworks Display  [Phase 1]";
const unsigned int window_width_g  = 900;
const unsigned int window_height_g = 700;
const glm::vec3 background_g(0.01f, 0.01f, 0.06f); // deep night blue

// Camera globals
float camera_near_clip_g = 0.1f;
float camera_far_clip_g  = 3000.0f;
float camera_fov_g       = 60.0f;

// View and projection matrices
glm::mat4 view_matrix, projection_matrix;

// GLFW window and camera
GLFWwindow* window;
Camera*     camera;

// Particle system
ParticleSystem* particleSys;

// Static scene light (world space) — single ambient overhead light for ground
glm::vec3 scene_light_pos_g(0.0f, 200.0f, 100.0f);

// Delta-time tracking
double lastTime_g = 0.0;

// -----------------------------------------------------------------------
// Geometry handle — same typedef as A2
// -----------------------------------------------------------------------
typedef struct Geometry {
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLuint size;
} Geometry;

// Ground vertex: position + normal (texcoords added in Phase 2)
struct GroundVertex {
    glm::vec3 pos;    // offset  0 — 12 bytes
    glm::vec3 normal; // offset 12 — 12 bytes
};

// -----------------------------------------------------------------------
// Shader utilities — identical to A2
// -----------------------------------------------------------------------
std::string LoadTextFile(std::string filename) {
    std::ifstream f(filename.c_str());
    if (f.fail()) {
        throw(std::ios_base::failure("Error opening file: " + filename));
    }
    std::string content, line;
    while (std::getline(f, line)) {
        content += line + "\n";
    }
    f.close();
    return content;
}

GLuint LoadShaders(std::string shaderName) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    std::string vert_src = LoadTextFile(std::string(SHADER_DIRECTORY) + shaderName + ".vert");
    std::string frag_src = LoadTextFile(std::string(SHADER_DIRECTORY) + shaderName + ".frag");

    const char* cv = vert_src.c_str();
    glShaderSource(vs, 1, &cv, NULL);
    glCompileShader(vs);
    GLint status;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[512]; glGetShaderInfoLog(vs, 512, NULL, buf);
        throw(std::ios_base::failure(std::string("Vertex shader error (") + shaderName + "): " + buf));
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* cf = frag_src.c_str();
    glShaderSource(fs, 1, &cf, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[512]; glGetShaderInfoLog(fs, 512, NULL, buf);
        throw(std::ios_base::failure(std::string("Fragment shader error (") + shaderName + "): " + buf));
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[512]; glGetProgramInfoLog(prog, 512, NULL, buf);
        throw(std::ios_base::failure(std::string("Shader link error (") + shaderName + "): " + buf));
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void LoadShaderMatrix(GLuint shader, glm::mat4 matrix, std::string name) {
    GLint loc = glGetUniformLocation(shader, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
}

void LoadShaderVec3(GLuint shader, glm::vec3 v, std::string name) {
    GLint loc = glGetUniformLocation(shader, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(v));
}

void LoadShaderFloat(GLuint shader, float val, std::string name) {
    GLint loc = glGetUniformLocation(shader, name.c_str());
    glUniform1f(loc, val);
}

void LoadShaderInt(GLuint shader, int val, std::string name) {
    GLint loc = glGetUniformLocation(shader, name.c_str());
    glUniform1i(loc, val);
}

// -----------------------------------------------------------------------
// CreateGroundPlane — large quad at y = 0, normal pointing up
// -----------------------------------------------------------------------
Geometry* CreateGroundPlane(float halfSize = 300.0f) {
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    GroundVertex verts[4] = {
        { glm::vec3(-halfSize, 0.0f, -halfSize), up },
        { glm::vec3( halfSize, 0.0f, -halfSize), up },
        { glm::vec3( halfSize, 0.0f,  halfSize), up },
        { glm::vec3(-halfSize, 0.0f,  halfSize), up },
    };
    unsigned int idx[6] = { 0, 1, 2,  0, 2, 3 };

    Geometry* geom = new Geometry();
    geom->size = 6;

    glGenVertexArrays(1, &geom->vao);
    glBindVertexArray(geom->vao);

    glGenBuffers(1, &geom->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glGenBuffers(1, &geom->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    return geom;
}

// -----------------------------------------------------------------------
// RenderGround — Phong-shaded ground plane, same style as A2 Render()
// -----------------------------------------------------------------------
void RenderGround(Geometry* geom, GLuint shader) {
    glUseProgram(shader);
    glBindVertexArray(geom->vao);
    glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ibo);

    // GroundVertex layout: pos(vec3 @ 0) | normal(vec3 @ 12)
    GLint pos_att = glGetAttribLocation(shader, "vertex");
    glVertexAttribPointer(pos_att, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), 0);
    glEnableVertexAttribArray(pos_att);

    GLint norm_att = glGetAttribLocation(shader, "normal");
    glVertexAttribPointer(norm_att, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)12);
    glEnableVertexAttribArray(norm_att);

    glm::mat4 world = glm::mat4(1.0f);
    view_matrix       = camera->GetViewMatrix(NULL);
    projection_matrix = camera->GetProjectionMatrix(NULL);

    LoadShaderMatrix(shader, world,             "world_mat");
    LoadShaderMatrix(shader, view_matrix,       "view_mat");
    LoadShaderMatrix(shader, projection_matrix, "projection_mat");

    // Transform light to view space before passing (same approach as A2)
    glm::vec3 lightPosView = glm::vec3(view_matrix * glm::vec4(scene_light_pos_g, 1.0f));
    LoadShaderVec3(shader, lightPosView, "lightPosView");

    // Dark green — nighttime grass
    LoadShaderVec3(shader, glm::vec3(0.07f, 0.16f, 0.05f), "ground_color");

    glDrawElements(GL_TRIANGLES, geom->size, GL_UNSIGNED_INT, 0);
}

// -----------------------------------------------------------------------
// Particle GPU resources — pre-allocated dynamic VBO
// -----------------------------------------------------------------------
GLuint particle_vao_g;
GLuint particle_vbo_g;

void InitParticleGPU() {
    glGenVertexArrays(1, &particle_vao_g);
    glBindVertexArray(particle_vao_g);

    glGenBuffers(1, &particle_vbo_g);
    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo_g);
    // Allocate max capacity; data is written by glBufferSubData each frame
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(ParticleVertex), NULL, GL_DYNAMIC_DRAW);
}

// -----------------------------------------------------------------------
// RenderParticles — upload updated data and draw with GL_POINTS
// -----------------------------------------------------------------------
void RenderParticles(GLuint shader) {
    if (particleSys->ActiveCount() == 0) return;

    glUseProgram(shader);
    glBindVertexArray(particle_vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo_g);

    // Stream updated particle positions/colours to GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        particleSys->ActiveCount() * sizeof(ParticleVertex),
        particleSys->GetVertices().data());

    // ParticleVertex layout: position(vec3 @ 0) | color(vec4 @ 12) | size(float @ 28)
    GLint pos_att = glGetAttribLocation(shader, "vertex");
    glVertexAttribPointer(pos_att, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), 0);
    glEnableVertexAttribArray(pos_att);

    GLint col_att = glGetAttribLocation(shader, "color");
    glVertexAttribPointer(col_att, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)12);
    glEnableVertexAttribArray(col_att);

    GLint sz_att = glGetAttribLocation(shader, "size");
    glVertexAttribPointer(sz_att, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)28);
    glEnableVertexAttribArray(sz_att);

    glm::mat4 world = glm::mat4(1.0f);
    view_matrix       = camera->GetViewMatrix(NULL);
    projection_matrix = camera->GetProjectionMatrix(NULL);

    LoadShaderMatrix(shader, world,             "world_mat");
    LoadShaderMatrix(shader, view_matrix,       "view_mat");
    LoadShaderMatrix(shader, projection_matrix, "projection_mat");

    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, particleSys->ActiveCount());
}

// -----------------------------------------------------------------------
// Keyboard callback — mirrors A2 style
// -----------------------------------------------------------------------
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE) return;

    // Camera controls — 1 degree increments
    if (key == GLFW_KEY_UP)    camera->Pitch( 1.0f);
    if (key == GLFW_KEY_DOWN)  camera->Pitch(-1.0f);
    if (key == GLFW_KEY_LEFT)  camera->Yaw(  -1.0f);
    if (key == GLFW_KEY_RIGHT) camera->Yaw(   1.0f);
    if (key == GLFW_KEY_A)     camera->Roll( -1.0f);
    if (key == GLFW_KEY_D)     camera->Roll(  1.0f);
    if (key == GLFW_KEY_W)     camera->MoveForward(5.0f);
    if (key == GLFW_KEY_S)     camera->MoveBackward(5.0f);

    // Launch starburst firework at a random XZ launch point
    if (key == GLFW_KEY_SPACE || key == GLFW_KEY_1) {
        float x = (float)(rand() % 120) - 60.0f;
        float z = (float)(rand() % 120) - 60.0f;
        particleSys->Launch(glm::vec3(x, 0.0f, z), FIREWORK_STARBURST);
        std::cout << "Starburst launched at (" << x << ", 0, " << z << ")\n";
    }

    // Quit
    if (key == GLFW_KEY_Q) glfwSetWindowShouldClose(window, true);
}

void ResizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void PrintControls() {
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL:   " << glGetString(GL_VERSION)  << "\n";
    std::cout << "GLSL:     " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "\n=== Interactive Fireworks Display — Phase 1 (50%) ===\n";
    std::cout << "Arrow Up/Down    : Pitch camera\n";
    std::cout << "Arrow Left/Right : Yaw camera\n";
    std::cout << "A / D            : Roll camera\n";
    std::cout << "W / S            : Move forward / backward\n";
    std::cout << "SPACE or 1       : Launch starburst firework\n";
    std::cout << "Q                : Quit\n";
    std::cout << "\nPhase 2 (coming soon): 2=Fountain  3=Rocket  4=Cascade\n";
    std::cout << "                       Textures, skybox, dynamic lighting\n\n";
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------
int main(void) {
    srand((unsigned int)time(NULL));

    try {
        // --- GLFW initialisation ---
        if (!glfwInit()) {
            throw(std::runtime_error("Could not initialize GLFW"));
        }

        window = glfwCreateWindow(window_width_g, window_height_g,
                                  window_title_g.c_str(), NULL, NULL);
        if (!window) {
            glfwTerminate();
            throw(std::runtime_error("Could not create GLFW window"));
        }

        glfwMakeContextCurrent(window);

        // --- GLEW initialisation ---
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            throw(std::runtime_error(std::string("GLEW init failed: ") +
                  (const char*)glewGetErrorString(err)));
        }

        // --- OpenGL state ---
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);

        // --- Camera — positioned in front of the display area, looking inward ---
        camera = new Camera();
        camera->SetCamera(
            glm::vec3(  0.0f, 80.0f, 250.0f),  // eye
            glm::vec3(  0.0f, 60.0f,   0.0f),  // look-at
            glm::vec3(  0.0f,  1.0f,   0.0f)); // up
        camera->SetPerspectiveView(camera_fov_g,
            (float)window_width_g / (float)window_height_g,
            camera_near_clip_g, camera_far_clip_g);

        // --- Load shaders ---
        GLuint groundShader   = LoadShaders("ground");
        GLuint particleShader = LoadShaders("particle");

        // --- Create geometry ---
        Geometry* ground = CreateGroundPlane(300.0f);

        // --- Particle system ---
        particleSys = new ParticleSystem();
        InitParticleGPU();

        // --- GLFW callbacks ---
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetFramebufferSizeCallback(window, ResizeCallback);

        PrintControls();

        // Auto-launch a few fireworks so the scene is active from the start
        for (int i = 0; i < 4; i++) {
            float x = (float)(rand() % 100) - 50.0f;
            float z = (float)(rand() % 100) - 50.0f;
            particleSys->Launch(glm::vec3(x, 0.0f, z), FIREWORK_STARBURST);
        }

        lastTime_g = glfwGetTime();

        // -----------------------------------------------------------------------
        // Main render loop
        // -----------------------------------------------------------------------
        while (!glfwWindowShouldClose(window)) {

            // Delta time
            double now = glfwGetTime();
            float  dt  = (float)(now - lastTime_g);
            lastTime_g = now;
            if (dt > 0.05f) dt = 0.05f; // cap at 50 ms to prevent physics blow-up

            // Advance simulation
            particleSys->Update(dt);

            // Clear
            glClearColor(background_g.r, background_g.g, background_g.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // --- Ground (opaque — depth writes ON, blending OFF) ---
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            RenderGround(ground, groundShader);

            // --- Particles (transparent — additive blending, depth writes OFF) ---
            // Additive blending: src*srcAlpha + dst*1  =>  bright glowing particles
            // depth writes off so particles don't occlude each other
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            RenderParticles(particleShader);
            glDepthMask(GL_TRUE);

            glfwPollEvents();
            glfwSwapBuffers(window);
        }

        // Cleanup
        delete camera;
        delete particleSys;
        delete ground;
    }
    catch (std::exception& e) {
        PrintException(e);
    }

    glfwTerminate();
    return 0;
}
