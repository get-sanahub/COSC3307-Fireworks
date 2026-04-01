/*
COSC 3307 - Project: Interactive Fireworks Display
Sana Begum | Nipissing University

Scene: Textured ground plane, particle-based fireworks, dynamic lighting.

--- Phase 1 ---
[x] GLFW window + GLEW + camera (identical setup to A2)
[x] Particle pool: rockets fly upward then explode (Starburst type)
[x] Physics: gravity, air drag, linear alpha fade, point-size shrink
[x] Rendering: dynamic VBO updated per frame, GL_POINTS with additive blending
[x] Circular glow shape via gl_PointCoord in fragment shader
[x] Ground plane with Phong shading (ambient + diffuse + specular)
[x] Night-blue background
[x] Camera controls (arrows, A/D roll, W/S move)
[x] Keyboard launch (SPACE / 1) + auto-launch on startup

--- Phase 2 ---
[x] Fountain firework type (key 2): continuous narrow-cone ground emitter
[x] Rocket-with-Trail firework type (key 3): rocket emits trail sparks, then starburst
[x] Cascade firework type (key 4): burst spawns secondary mini-rockets
[x] Procedural ground texture (dark-green grass grid, generated in C++)
[x] Procedural particle glow texture (64x64 circular alpha gradient)
[x] Dynamic point lights: active explosion positions light the ground (up to 8)
[x] Multi-firework choreography: auto-launch timer cycles all four types
[ ] Skybox sphere (night-sky gradient + star field) -- in progress

Controls:
  Arrow Up/Down   : Pitch camera
  Arrow Left/Right: Yaw camera
  A / D           : Roll camera
  W / S           : Move forward / backward
  SPACE or 1      : Launch starburst firework
  2               : Launch fountain firework
  3               : Launch rocket-with-trail firework
  4               : Launch cascade firework
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
#include <cmath>

// -----------------------------------------------------------------------
// IMPORTANT: Change this path to point to Project_Fireworks/resource/ on YOUR machine.
// -----------------------------------------------------------------------
#define SHADER_DIRECTORY "C:/Users/sanam/OneDrive - Nipissing University/Desktop/3D_computer.graphics/Project_Fireworks/resource/"

#define PrintException(exception_object)\
    std::cerr << exception_object.what() << std::endl

// Window
const std::string window_title_g  = "COSC 3307 - Interactive Fireworks Display";
const unsigned int window_width_g  = 900;
const unsigned int window_height_g = 700;
const glm::vec3 background_g(0.01f, 0.01f, 0.06f); // deep night blue

// Camera globals
float camera_near_clip_g = 0.1f;
float camera_far_clip_g  = 3000.0f;
float camera_fov_g       = 60.0f;

// View and projection matrices (updated each frame)
glm::mat4 view_matrix, projection_matrix;

// GLFW window and camera
GLFWwindow* window;
Camera*     camera;

// Particle system
ParticleSystem* particleSys;

// Static scene light (world space)
glm::vec3 scene_light_pos_g(0.0f, 200.0f, 100.0f);

// Delta-time tracking
double lastTime_g = 0.0;

// Procedural GPU textures
GLuint ground_tex_g        = 0;
GLuint particle_glow_tex_g = 0;

// Choreography state
double choreo_next_time_g = 2.0;
int    choreo_idx_g       = 0;

// -----------------------------------------------------------------------
// Geometry handle — same typedef as A2
// -----------------------------------------------------------------------
typedef struct Geometry {
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLuint size;
} Geometry;

// Ground vertex: position + normal + UV texcoord
struct GroundVertex {
    glm::vec3 pos;     // offset  0 — 12 bytes
    glm::vec3 normal;  // offset 12 — 12 bytes
    glm::vec2 uv;      // offset 24 —  8 bytes
};                     // total  32 bytes

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
// CreateGroundTexture
// 128x128 procedural dark-green grass — generated entirely in C++.
// -----------------------------------------------------------------------
GLuint CreateGroundTexture() {
    const int W = 128, H = 128;
    unsigned char pixels[W * H * 3];

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int i = (y * W + x) * 3;
            float nx = (float)x / (float)W;
            float ny = (float)y / (float)H;
            float v  = 0.5f + 0.5f * sinf(nx * 37.0f) * cosf(ny * 31.0f);
            bool onGrid = (x % 16 == 0) || (y % 16 == 0);
            if (onGrid) {
                pixels[i+0] = 28;
                pixels[i+1] = 62;
                pixels[i+2] = 20;
            } else {
                pixels[i+0] = (unsigned char)(10  + v * 22);
                pixels[i+1] = (unsigned char)(32  + v * 52);
                pixels[i+2] = (unsigned char)(8   + v * 18);
            }
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

// -----------------------------------------------------------------------
// CreateParticleGlowTexture
// 64x64 RGBA circular glow (bright centre, transparent edges).
// -----------------------------------------------------------------------
GLuint CreateParticleGlowTexture() {
    const int W = 64, H = 64;
    unsigned char pixels[W * H * 4];
    float cx = W * 0.5f, cy = H * 0.5f;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int   i    = (y * W + x) * 4;
            float dx   = (x - cx) / cx;
            float dy   = (y - cy) / cy;
            float dist = sqrtf(dx*dx + dy*dy);
            float t    = 1.0f - glm::clamp(dist, 0.0f, 1.0f);
            float v    = t * t;
            pixels[i+0] = 255;
            pixels[i+1] = 255;
            pixels[i+2] = 255;
            pixels[i+3] = (unsigned char)(v * 255.0f);
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// -----------------------------------------------------------------------
// CreateGroundPlane — large quad at y = 0, normal pointing up
// Vertex now includes UV texcoords (tiled 20x across the plane)
// -----------------------------------------------------------------------
Geometry* CreateGroundPlane(float halfSize = 300.0f) {
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const float     tileCount = 20.0f;

    GroundVertex verts[4] = {
        { glm::vec3(-halfSize, 0.0f, -halfSize), up, glm::vec2(0.0f,       0.0f)      },
        { glm::vec3( halfSize, 0.0f, -halfSize), up, glm::vec2(tileCount,  0.0f)      },
        { glm::vec3( halfSize, 0.0f,  halfSize), up, glm::vec2(tileCount,  tileCount) },
        { glm::vec3(-halfSize, 0.0f,  halfSize), up, glm::vec2(0.0f,       tileCount) },
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
// RenderGround — Phong-shaded textured ground plane
// Binds procedural texture + uploads dynamic firework lights
// -----------------------------------------------------------------------
void RenderGround(Geometry* geom, GLuint shader) {
    glUseProgram(shader);
    glBindVertexArray(geom->vao);
    glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ibo);

    // GroundVertex layout (32 bytes): pos @ 0, normal @ 12, uv @ 24
    GLint pos_att = glGetAttribLocation(shader, "vertex");
    glVertexAttribPointer(pos_att, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), 0);
    glEnableVertexAttribArray(pos_att);

    GLint norm_att = glGetAttribLocation(shader, "normal");
    glVertexAttribPointer(norm_att, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)12);
    glEnableVertexAttribArray(norm_att);

    GLint uv_att = glGetAttribLocation(shader, "texcoord");
    if (uv_att >= 0) {
        glVertexAttribPointer(uv_att, 2, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)24);
        glEnableVertexAttribArray(uv_att);
    }

    glm::mat4 world = glm::mat4(1.0f);
    view_matrix       = camera->GetViewMatrix(NULL);
    projection_matrix = camera->GetProjectionMatrix(NULL);

    LoadShaderMatrix(shader, world,             "world_mat");
    LoadShaderMatrix(shader, view_matrix,       "view_mat");
    LoadShaderMatrix(shader, projection_matrix, "projection_mat");

    glm::vec3 lightPosView = glm::vec3(view_matrix * glm::vec4(scene_light_pos_g, 1.0f));
    LoadShaderVec3(shader, lightPosView, "lightPosView");
    LoadShaderVec3(shader, glm::vec3(0.07f, 0.16f, 0.05f), "ground_color");

    // Ground texture on unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ground_tex_g);
    LoadShaderInt(shader, 0, "groundTex");

    // Dynamic point lights from active firework explosions
    glm::vec3 dynLights[MAX_DYN_LIGHTS];
    int numLights = particleSys->GetActiveLightPositions(dynLights);

    LoadShaderInt(shader, numLights, "numDynLights");
    if (numLights > 0) {
        glm::vec3 dynLightsView[MAX_DYN_LIGHTS];
        for (int i = 0; i < numLights; i++) {
            dynLightsView[i] = glm::vec3(view_matrix * glm::vec4(dynLights[i], 1.0f));
        }
        GLint loc = glGetUniformLocation(shader, "dynLightsView");
        if (loc >= 0) {
            glUniform3fv(loc, numLights, glm::value_ptr(dynLightsView[0]));
        }
    }

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
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(ParticleVertex), NULL, GL_DYNAMIC_DRAW);
}

// -----------------------------------------------------------------------
// RenderParticles — upload updated data and draw with GL_POINTS
// Binds procedural glow texture
// -----------------------------------------------------------------------
void RenderParticles(GLuint shader) {
    if (particleSys->ActiveCount() == 0) return;

    glUseProgram(shader);
    glBindVertexArray(particle_vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo_g);

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

    // Bind procedural glow texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particle_glow_tex_g);
    LoadShaderInt(shader, 0, "glowTex");

    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, particleSys->ActiveCount());
}

// -----------------------------------------------------------------------
// UpdateChoreography — auto-launches mixed fireworks on a timer
// -----------------------------------------------------------------------
void UpdateChoreography(double now) {
    if (now < choreo_next_time_g) return;

    FireworkType seq[5] = {
        FIREWORK_STARBURST, FIREWORK_FOUNTAIN,
        FIREWORK_TRAIL,     FIREWORK_CASCADE,
        FIREWORK_STARBURST
    };

    float x = (float)(rand() % 180) - 90.0f;
    float z = (float)(rand() % 180) - 90.0f;
    int   s = choreo_idx_g % 5;

    particleSys->Launch(glm::vec3(x, 0.0f, z), seq[s]);

    if (s == 4) {
        float x2 = (float)(rand() % 180) - 90.0f;
        float z2 = (float)(rand() % 180) - 90.0f;
        particleSys->Launch(glm::vec3(x2, 0.0f, z2), FIREWORK_CASCADE);
    }

    choreo_idx_g++;
    choreo_next_time_g = now + 2.0 + (double)(rand() % 200) / 100.0;
}

// -----------------------------------------------------------------------
// Keyboard callback — mirrors A2 style
// -----------------------------------------------------------------------
void KeyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE) return;

    if (key == GLFW_KEY_UP)    camera->Pitch( 1.0f);
    if (key == GLFW_KEY_DOWN)  camera->Pitch(-1.0f);
    if (key == GLFW_KEY_LEFT)  camera->Yaw(  -1.0f);
    if (key == GLFW_KEY_RIGHT) camera->Yaw(   1.0f);
    if (key == GLFW_KEY_A)     camera->Roll( -1.0f);
    if (key == GLFW_KEY_D)     camera->Roll(  1.0f);
    if (key == GLFW_KEY_W)     camera->MoveForward(5.0f);
    if (key == GLFW_KEY_S)     camera->MoveBackward(5.0f);

    float x = (float)(rand() % 160) - 80.0f;
    float z = (float)(rand() % 160) - 80.0f;

    if (key == GLFW_KEY_SPACE || key == GLFW_KEY_1) {
        particleSys->Launch(glm::vec3(x, 0.0f, z), FIREWORK_STARBURST);
        std::cout << "Starburst at (" << x << ", 0, " << z << ")\n";
    }
    if (key == GLFW_KEY_2) {
        particleSys->Launch(glm::vec3(x, 0.0f, z), FIREWORK_FOUNTAIN);
        std::cout << "Fountain at  (" << x << ", 0, " << z << ")\n";
    }
    if (key == GLFW_KEY_3) {
        particleSys->Launch(glm::vec3(x, 0.0f, z), FIREWORK_TRAIL);
        std::cout << "Trail rocket at (" << x << ", 0, " << z << ")\n";
    }
    if (key == GLFW_KEY_4) {
        particleSys->Launch(glm::vec3(x, 0.0f, z), FIREWORK_CASCADE);
        std::cout << "Cascade at   (" << x << ", 0, " << z << ")\n";
    }

    if (key == GLFW_KEY_Q) glfwSetWindowShouldClose(win, true);
}

void ResizeCallback(GLFWwindow* win, int width, int height) {
    glViewport(0, 0, width, height);
}

void PrintControls() {
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL:   " << glGetString(GL_VERSION)  << "\n";
    std::cout << "GLSL:     " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "\n=== Interactive Fireworks Display ===\n";
    std::cout << "Arrow Up/Down    : Pitch camera\n";
    std::cout << "Arrow Left/Right : Yaw camera\n";
    std::cout << "A / D            : Roll camera\n";
    std::cout << "W / S            : Move forward / backward\n";
    std::cout << "SPACE or 1       : Launch starburst firework\n";
    std::cout << "2                : Launch fountain firework\n";
    std::cout << "3                : Launch rocket-with-trail firework\n";
    std::cout << "4                : Launch cascade firework\n";
    std::cout << "Q                : Quit\n\n";
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------
int main(void) {
    srand((unsigned int)time(NULL));

    try {
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

        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            throw(std::runtime_error(std::string("GLEW init failed: ") +
                  (const char*)glewGetErrorString(err)));
        }

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);
        glEnable(GL_PROGRAM_POINT_SIZE);

        camera = new Camera();
        camera->SetCamera(
            glm::vec3(  0.0f, 80.0f, 250.0f),
            glm::vec3(  0.0f, 60.0f,   0.0f),
            glm::vec3(  0.0f,  1.0f,   0.0f));
        camera->SetPerspectiveView(camera_fov_g,
            (float)window_width_g / (float)window_height_g,
            camera_near_clip_g, camera_far_clip_g);

        GLuint groundShader   = LoadShaders("ground");
        GLuint particleShader = LoadShaders("particle");

        Geometry* ground = CreateGroundPlane(300.0f);

        ground_tex_g        = CreateGroundTexture();
        particle_glow_tex_g = CreateParticleGlowTexture();

        particleSys = new ParticleSystem();
        InitParticleGPU();

        glfwSetKeyCallback(window, KeyCallback);
        glfwSetFramebufferSizeCallback(window, ResizeCallback);

        PrintControls();

        // Auto-launch a variety of fireworks so the scene is active immediately
        float startPos[4][2] = { {-50.0f,-30.0f}, {50.0f,-30.0f}, {0.0f,40.0f}, {-30.0f,60.0f} };
        FireworkType startTypes[4] = { FIREWORK_STARBURST, FIREWORK_CASCADE,
                                       FIREWORK_TRAIL,     FIREWORK_FOUNTAIN };
        for (int i = 0; i < 4; i++) {
            particleSys->Launch(glm::vec3(startPos[i][0], 0.0f, startPos[i][1]), startTypes[i]);
        }

        lastTime_g         = glfwGetTime();
        choreo_next_time_g = lastTime_g + 3.0;

        while (!glfwWindowShouldClose(window)) {

            double now = glfwGetTime();
            float  dt  = (float)(now - lastTime_g);
            lastTime_g = now;
            if (dt > 0.05f) dt = 0.05f;

            particleSys->Update(dt);
            UpdateChoreography(now);

            glClearColor(background_g.r, background_g.g, background_g.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Ground (opaque)
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            RenderGround(ground, groundShader);

            // Particles (additive blending)
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            RenderParticles(particleShader);
            glDepthMask(GL_TRUE);

            glfwPollEvents();
            glfwSwapBuffers(window);
        }

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
