/*
COSC 3307 - Project: Interactive Fireworks Display
FireworksApp — implementation.
*/
#include <app.h>
#include <shader_utils.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <ctime>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ── Window / camera settings ──────────────────────────────────────────────────
static const unsigned int WIN_W    = 900;
static const unsigned int WIN_H    = 700;
static const float        CAM_FOV  = 60.0f;
static const float        CAM_NEAR = 0.1f;
static const float        CAM_FAR  = 5000.0f;

// Moon world-space position — also used as the scene light source
static const glm::vec3 MOON_POS(60.0f, 140.0f, -200.0f);

// ── CPU billboard buffers (static = data segment, not stack) ──────────────────
static ParticleVertex s_pVerts[MAX_PARTICLES * 4];
static unsigned int   s_pIdxs [MAX_PARTICLES * 6];

// ── Static GLFW callback instance ─────────────────────────────────────────────
FireworksApp* FireworksApp::instance_ = nullptr;

// ═════════════════════════════════════════════════════════════════════════════
// Constructor / Destructor
// ═════════════════════════════════════════════════════════════════════════════
FireworksApp::FireworksApp()
    : window_(nullptr), camera_(nullptr),
      ground_shader_(0), particle_shader_(0), skybox_shader_(0), object_shader_(0),
      ground_(nullptr), skybox_(nullptr), ground_tex_(0),
      scene_light_pos_(MOON_POS),
      vase_base_(nullptr), vase_column_(nullptr), vase_bowl_(nullptr),
      moon_(nullptr),
      road_z_(nullptr), road_x_(nullptr),
      human_head_(nullptr), human_body_(nullptr), human_leg_(nullptr),
      rocket_body_(nullptr), rocket_nose_(nullptr),
      particle_sys_(nullptr),
      particle_vao_(0), particle_vbo_(0), particle_ibo_(0),
      last_time_(0.0)
{
    instance_ = this;
}

FireworksApp::~FireworksApp() {
    delete camera_;
    delete particle_sys_;
    delete ground_;    delete skybox_;
    delete vase_base_; delete vase_column_; delete vase_bowl_;
    delete moon_;
    delete road_z_;    delete road_x_;
    delete human_head_; delete human_body_; delete human_leg_;
    delete rocket_body_; delete rocket_nose_;
}

// ═════════════════════════════════════════════════════════════════════════════
// InitParticleGPU
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::InitParticleGPU() {
    glGenVertexArrays(1, &particle_vao_);
    glBindVertexArray(particle_vao_);

    glGenBuffers(1, &particle_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(MAX_PARTICLES * 4 * sizeof(ParticleVertex)),
                 NULL, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &particle_ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particle_ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(MAX_PARTICLES * 6 * sizeof(unsigned int)),
                 NULL, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
}

// ═════════════════════════════════════════════════════════════════════════════
// DrawObject — helper used by RenderObjects and RenderRockets
// Sets up ObjectVertex attrib pointers, uploads world matrix + shininess,
// then issues one draw call. object_shader_ must already be active.
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::DrawObject(Geometry* geom, const glm::mat4& worldMat,
                               float shininess, float emissive)
{
    glBindVertexArray(geom->vao);
    glBindBuffer(GL_ARRAY_BUFFER,         geom->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ibo);

    // ObjectVertex layout: pos@0(12) | normal@12(12) | color@24(12) = 36 bytes
    GLint pos_att  = glGetAttribLocation(object_shader_, "vertex");
    GLint norm_att = glGetAttribLocation(object_shader_, "normal");
    GLint col_att  = glGetAttribLocation(object_shader_, "color");

    if (pos_att  >= 0) {
        glVertexAttribPointer(pos_att,  3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)0);
        glEnableVertexAttribArray(pos_att);
    }
    if (norm_att >= 0) {
        glVertexAttribPointer(norm_att, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)12);
        glEnableVertexAttribArray(norm_att);
    }
    if (col_att  >= 0) {
        glVertexAttribPointer(col_att,  3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)24);
        glEnableVertexAttribArray(col_att);
    }

    LoadShaderMatrix(object_shader_, worldMat,        "world_mat");
    LoadShaderFloat (object_shader_, shininess,        "shininess");
    LoadShaderFloat (object_shader_, emissive,         "emissive");

    glDrawElements(GL_TRIANGLES, geom->size, GL_UNSIGNED_INT, 0);
}

// ═════════════════════════════════════════════════════════════════════════════
// RenderSkybox
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::RenderSkybox() {
    glUseProgram(skybox_shader_);
    glBindVertexArray(skybox_->vao);
    glBindBuffer(GL_ARRAY_BUFFER,         skybox_->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox_->ibo);

    GLint pos_att = glGetAttribLocation(skybox_shader_, "vertex");
    glVertexAttribPointer(pos_att, 3, GL_FLOAT, GL_FALSE, sizeof(SkyVertex), 0);
    glEnableVertexAttribArray(pos_att);

    view_matrix_       = camera_->GetViewMatrix(NULL);
    projection_matrix_ = camera_->GetProjectionMatrix(NULL);
    LoadShaderMatrix(skybox_shader_, view_matrix_,       "view_mat");
    LoadShaderMatrix(skybox_shader_, projection_matrix_, "projection_mat");

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glDrawElements(GL_TRIANGLES, skybox_->size, GL_UNSIGNED_INT, 0);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

// ═════════════════════════════════════════════════════════════════════════════
// RenderGround
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::RenderGround() {
    glUseProgram(ground_shader_);
    glBindVertexArray(ground_->vao);
    glBindBuffer(GL_ARRAY_BUFFER,         ground_->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ground_->ibo);

    GLint pos_att = glGetAttribLocation(ground_shader_, "vertex");
    glVertexAttribPointer(pos_att,  3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), 0);
    glEnableVertexAttribArray(pos_att);

    GLint norm_att = glGetAttribLocation(ground_shader_, "normal");
    glVertexAttribPointer(norm_att, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)12);
    glEnableVertexAttribArray(norm_att);

    GLint uv_att = glGetAttribLocation(ground_shader_, "texcoord");
    if (uv_att >= 0) {
        glVertexAttribPointer(uv_att, 2, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), (void*)24);
        glEnableVertexAttribArray(uv_att);
    }

    glm::mat4 world = glm::mat4(1.0f);
    view_matrix_       = camera_->GetViewMatrix(NULL);
    projection_matrix_ = camera_->GetProjectionMatrix(NULL);
    LoadShaderMatrix(ground_shader_, world,              "world_mat");
    LoadShaderMatrix(ground_shader_, view_matrix_,       "view_mat");
    LoadShaderMatrix(ground_shader_, projection_matrix_, "projection_mat");

    glm::vec3 lightView = glm::vec3(view_matrix_ * glm::vec4(scene_light_pos_, 1.0f));
    LoadShaderVec3(ground_shader_, lightView,                       "lightPosView");
    LoadShaderVec3(ground_shader_, glm::vec3(0.07f, 0.16f, 0.05f), "ground_color");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ground_tex_);
    LoadShaderInt(ground_shader_, 0, "groundTex");

    glm::vec3 dynLights[MAX_DYN_LIGHTS];
    int numLights = particle_sys_->GetActiveLightPositions(dynLights);
    LoadShaderInt(ground_shader_, numLights, "numDynLights");
    if (numLights > 0) {
        glm::vec3 dynLightsView[MAX_DYN_LIGHTS];
        for (int i = 0; i < numLights; i++)
            dynLightsView[i] = glm::vec3(view_matrix_ * glm::vec4(dynLights[i], 1.0f));
        GLint loc = glGetUniformLocation(ground_shader_, "dynLightsView");
        if (loc >= 0)
            glUniform3fv(loc, numLights, glm::value_ptr(dynLightsView[0]));
    }

    glDrawElements(GL_TRIANGLES, ground_->size, GL_UNSIGNED_INT, 0);
}

// ═════════════════════════════════════════════════════════════════════════════
// RenderObjects — fountain vase, road, human figures, moon
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::RenderObjects() {
    glUseProgram(object_shader_);

    // Shared uniforms for all objects
    view_matrix_       = camera_->GetViewMatrix(NULL);
    projection_matrix_ = camera_->GetProjectionMatrix(NULL);
    LoadShaderMatrix(object_shader_, view_matrix_,       "view_mat");
    LoadShaderMatrix(object_shader_, projection_matrix_, "projection_mat");

    glm::vec3 lightView = glm::vec3(view_matrix_ * glm::vec4(scene_light_pos_, 1.0f));
    LoadShaderVec3(object_shader_, lightView, "lightPosView");

    // Pass explosion lights to objects too
    glm::vec3 dynLights[MAX_DYN_LIGHTS];
    int numLights = particle_sys_->GetActiveLightPositions(dynLights);
    LoadShaderInt(object_shader_, numLights, "numDynLights");
    if (numLights > 0) {
        glm::vec3 dynLightsView[MAX_DYN_LIGHTS];
        for (int i = 0; i < numLights; i++)
            dynLightsView[i] = glm::vec3(view_matrix_ * glm::vec4(dynLights[i], 1.0f));
        GLint loc = glGetUniformLocation(object_shader_, "dynLightsView");
        if (loc >= 0)
            glUniform3fv(loc, numLights, glm::value_ptr(dynLightsView[0]));
    }

    // ── Road (two crossing black asphalt strips) ──────────────────────────
    // Road along Z axis
    DrawObject(road_z_, glm::mat4(1.0f), 8.0f);
    // Road along X axis: rotate 90° around Y to make it run along X
    glm::mat4 roadRot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(90.0f), glm::vec3(0,1,0));
    DrawObject(road_x_, roadRot, 8.0f);

    // ── Fountain vase — stone-coloured, centred at (0,0,0) ───────────────
    // Base disc: wide and flat on the ground
    DrawObject(vase_base_,   glm::mat4(1.0f), 32.0f);
    // Column: narrow pillar rising from the base (starts at top of base = y+3)
    glm::mat4 colWorld = glm::translate(glm::mat4(1.0f), glm::vec3(0, 3.0f, 0));
    DrawObject(vase_column_, colWorld, 32.0f);
    // Bowl: wide shallow basin at the top of the column (base+column = 3+12 = y+15)
    glm::mat4 bowlWorld = glm::translate(glm::mat4(1.0f), glm::vec3(0, 15.0f, 0));
    DrawObject(vase_bowl_, bowlWorld, 48.0f);

    // ── Human figure 1 — standing near fountain ───────────────────────────
    // Legs: h=6 (y 0→6), Body: h=9 starts at y=6, Head: r=2 centre at y=6+9+2=17
    glm::vec3 h1(22.0f, 0.0f, 18.0f);
    DrawObject(human_body_, glm::translate(glm::mat4(1.0f), h1 + glm::vec3(0, 6.0f, 0)), 24.0f);
    DrawObject(human_head_, glm::translate(glm::mat4(1.0f), h1 + glm::vec3(0, 17.0f, 0)), 16.0f);
    DrawObject(human_leg_,  glm::translate(glm::mat4(1.0f), h1 + glm::vec3(-1.0f, 0, 0)), 16.0f);
    DrawObject(human_leg_,  glm::translate(glm::mat4(1.0f), h1 + glm::vec3( 1.0f, 0, 0)), 16.0f);

    // ── Human figure 2 — standing on the road ────────────────────────────
    glm::vec3 h2(-28.0f, 0.0f, -12.0f);
    DrawObject(human_body_, glm::translate(glm::mat4(1.0f), h2 + glm::vec3(0, 6.0f, 0)), 24.0f);
    DrawObject(human_head_, glm::translate(glm::mat4(1.0f), h2 + glm::vec3(0, 17.0f, 0)), 16.0f);
    DrawObject(human_leg_,  glm::translate(glm::mat4(1.0f), h2 + glm::vec3(-1.0f, 0, 0)), 16.0f);
    DrawObject(human_leg_,  glm::translate(glm::mat4(1.0f), h2 + glm::vec3( 1.0f, 0, 0)), 16.0f);

    // ── Moon sphere — self-illuminated (emissive=1), in the sky ──────────
    glm::mat4 moonWorld = glm::translate(glm::mat4(1.0f), MOON_POS);
    DrawObject(moon_, moonWorld, 8.0f, 1.0f);  // emissive=1 → skips Phong, just glows
}

// ═════════════════════════════════════════════════════════════════════════════
// RenderRockets — visible 3D rocket mesh for each active TRAIL rocket.
// Demonstrates Phong shading (ambient + diffuse + specular) on a 3D object
// clearly visible as the rocket flies — key 3.
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::RenderRockets() {
    ParticleSystem::RocketInfo rockets[MAX_ROCKETS];
    int count = particle_sys_->GetActiveTrailRockets(rockets, MAX_ROCKETS);
    if (count == 0) return;

    glUseProgram(object_shader_);
    view_matrix_       = camera_->GetViewMatrix(NULL);
    projection_matrix_ = camera_->GetProjectionMatrix(NULL);
    LoadShaderMatrix(object_shader_, view_matrix_,       "view_mat");
    LoadShaderMatrix(object_shader_, projection_matrix_, "projection_mat");

    glm::vec3 lightView = glm::vec3(view_matrix_ * glm::vec4(scene_light_pos_, 1.0f));
    LoadShaderVec3(object_shader_, lightView, "lightPosView");

    glm::vec3 dynLights[MAX_DYN_LIGHTS];
    int numLights = particle_sys_->GetActiveLightPositions(dynLights);
    LoadShaderInt(object_shader_, numLights, "numDynLights");
    if (numLights > 0) {
        glm::vec3 dynLightsView[MAX_DYN_LIGHTS];
        for (int i = 0; i < numLights; i++)
            dynLightsView[i] = glm::vec3(view_matrix_ * glm::vec4(dynLights[i], 1.0f));
        GLint loc = glGetUniformLocation(object_shader_, "dynLightsView");
        if (loc >= 0)
            glUniform3fv(loc, numLights, glm::value_ptr(dynLightsView[0]));
    }

    for (int i = 0; i < count; i++) {
        glm::vec3 pos = rockets[i].position;
        glm::vec3 vel = rockets[i].velocity;

        // Build rotation matrix: align cylinder Y-axis to velocity direction
        glm::mat4 rotation(1.0f);
        float vlen = glm::length(vel);
        if (vlen > 0.5f) {
            glm::vec3 dir  = vel / vlen;
            glm::vec3 defUp(0.0f, 1.0f, 0.0f);
            glm::vec3 axis = glm::cross(defUp, dir);
            float     alen = glm::length(axis);
            if (alen > 0.001f) {
                float angle = acosf(glm::clamp(glm::dot(defUp, dir), -1.0f, 1.0f));
                rotation    = glm::rotate(glm::mat4(1.0f), angle, axis / alen);
            }
        }

        // Place rocket: base of body at rocket position
        glm::mat4 base = glm::translate(glm::mat4(1.0f), pos) * rotation;

        // Body cylinder (metallic dark gray, high shininess = clear specular)
        DrawObject(rocket_body_, base, 128.0f);

        // Nose cone on top of body (bright red, medium shininess)
        glm::mat4 noseWorld = base * glm::translate(glm::mat4(1.0f), glm::vec3(0, 12.0f, 0));
        DrawObject(rocket_nose_, noseWorld, 64.0f);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RenderParticles
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::RenderParticles() {
    view_matrix_       = camera_->GetViewMatrix(NULL);
    projection_matrix_ = camera_->GetProjectionMatrix(NULL);

    glm::vec3 camRight(view_matrix_[0][0], view_matrix_[1][0], view_matrix_[2][0]);
    glm::vec3 camUp   (view_matrix_[0][1], view_matrix_[1][1], view_matrix_[2][1]);

    int count = particle_sys_->BuildBillboardMesh(s_pVerts, s_pIdxs, camRight, camUp);
    if (count == 0) return;

    glUseProgram(particle_shader_);
    glBindVertexArray(particle_vao_);

    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    (GLsizeiptr)(count * 4 * sizeof(ParticleVertex)), s_pVerts);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particle_ibo_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                    (GLsizeiptr)(count * 6 * sizeof(unsigned int)), s_pIdxs);

    GLint pos_att = glGetAttribLocation(particle_shader_, "vertex");
    if (pos_att >= 0) {
        glVertexAttribPointer(pos_att, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)0);
        glEnableVertexAttribArray(pos_att);
    }
    GLint col_att = glGetAttribLocation(particle_shader_, "color");
    if (col_att >= 0) {
        glVertexAttribPointer(col_att, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)12);
        glEnableVertexAttribArray(col_att);
    }
    GLint uv_att = glGetAttribLocation(particle_shader_, "texcoord");
    if (uv_att >= 0) {
        glVertexAttribPointer(uv_att, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)28);
        glEnableVertexAttribArray(uv_att);
    }

    LoadShaderMatrix(particle_shader_, view_matrix_,       "view_mat");
    LoadShaderMatrix(particle_shader_, projection_matrix_, "projection_mat");

    glDrawElements(GL_TRIANGLES, count * 6, GL_UNSIGNED_INT, 0);
}

// ═════════════════════════════════════════════════════════════════════════════
// PrintControls
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::PrintControls() {
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL:   " << glGetString(GL_VERSION)  << "\n";
    std::cout << "\n=== Interactive Fireworks Display ===\n";
    std::cout << "1/SPACE : Starburst    2: Fountain    3: Trail rocket (Phong demo)    4: Cascade\n";
    std::cout << "Arrows  : Camera pitch/yaw    A/D: Roll    W/S: Move    Q: Quit\n\n";
}

// ═════════════════════════════════════════════════════════════════════════════
// GLFW Callbacks
// ═════════════════════════════════════════════════════════════════════════════
void FireworksApp::KeyCallback(GLFWwindow* win, int key, int scancode,
                                int action, int mods)
{
    if (action == GLFW_RELEASE) return;

    if (key == GLFW_KEY_UP)    instance_->camera_->Pitch( 1.0f);
    if (key == GLFW_KEY_DOWN)  instance_->camera_->Pitch(-1.0f);
    if (key == GLFW_KEY_LEFT)  instance_->camera_->Yaw(  -1.0f);
    if (key == GLFW_KEY_RIGHT) instance_->camera_->Yaw(   1.0f);
    if (key == GLFW_KEY_A)     instance_->camera_->Roll( -1.0f);
    if (key == GLFW_KEY_D)     instance_->camera_->Roll(  1.0f);
    if (key == GLFW_KEY_W)     instance_->camera_->MoveForward(5.0f);
    if (key == GLFW_KEY_S)     instance_->camera_->MoveBackward(5.0f);

    // Random launch position for all firework types
    float x = (float)(rand() % 140) - 70.0f;
    float z = (float)(rand() % 80)  - 80.0f;

    if (key == GLFW_KEY_SPACE || key == GLFW_KEY_1)
        instance_->particle_sys_->Launch(glm::vec3(x, 0.0f, z), FIREWORK_STARBURST);

    // Fountain sparks emerge from bowl top — fountain stays at scene centre
    if (key == GLFW_KEY_2)
        instance_->particle_sys_->Launch(glm::vec3(0.0f, 19.0f, 0.0f), FIREWORK_FOUNTAIN);

    if (key == GLFW_KEY_3)
        instance_->particle_sys_->Launch(glm::vec3(x, 0.0f, z), FIREWORK_TRAIL);
    if (key == GLFW_KEY_4)
        instance_->particle_sys_->Launch(glm::vec3(x, 0.0f, z), FIREWORK_CASCADE);

    if (key == GLFW_KEY_Q) glfwSetWindowShouldClose(win, true);
}

void FireworksApp::ResizeCallback(GLFWwindow* win, int width, int height) {
    glViewport(0, 0, width, height);
}

// ═════════════════════════════════════════════════════════════════════════════
// Init — create window, load all assets, run main loop
// ═════════════════════════════════════════════════════════════════════════════
int FireworksApp::Init() {
    srand((unsigned int)time(NULL));

    try {
        if (!glfwInit())
            throw std::runtime_error("Could not initialize GLFW");

        window_ = glfwCreateWindow(WIN_W, WIN_H,
                    "COSC 3307 - Interactive Fireworks Display  [Phase 2]",
                    NULL, NULL);
        if (!window_) { glfwTerminate(); throw std::runtime_error("GLFW window failed"); }
        glfwMakeContextCurrent(window_);

        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK)
            throw std::runtime_error(std::string("GLEW: ") + (const char*)glewGetErrorString(err));

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);

        // ── Camera ────────────────────────────────────────────────────────
        camera_ = new Camera();
        camera_->SetCamera(
            glm::vec3(0.0f, 80.0f, 120.0f),
            glm::vec3(0.0f, 60.0f,   0.0f),
            glm::vec3(0.0f,  1.0f,   0.0f));
        camera_->SetPerspectiveView(CAM_FOV,
            (float)WIN_W / (float)WIN_H, CAM_NEAR, CAM_FAR);

        // ── Shaders ───────────────────────────────────────────────────────
        ground_shader_   = LoadShaders("ground");
        particle_shader_ = LoadShaders("particle");
        skybox_shader_   = LoadShaders("skybox");
        object_shader_   = LoadShaders("object");   // Phong for all 3D objects

        // ── Ground / sky ──────────────────────────────────────────────────
        ground_     = CreateGroundPlane(300.0f);
        skybox_     = CreateSkyboxSphere(2000.0f);
        ground_tex_ = CreateGroundTexture();

        // ── Road — dark asphalt strips, y=0.02 to avoid z-fighting ───────
        glm::vec3 asphalt(0.14f, 0.14f, 0.16f);
        road_z_ = CreateRoadStrip(12.0f, 300.0f, 0.15f, asphalt);
        road_x_ = CreateRoadStrip(12.0f, 300.0f, 0.15f, asphalt);  // rotated 90° in render

        // ── Fountain vase — stone / light marble colour ───────────────────
        glm::vec3 stone(0.72f, 0.70f, 0.65f);
        vase_base_   = CreateCylinder(10.0f,  3.0f, 24, stone);  // 2x: wide base disc
        vase_column_ = CreateCylinder( 2.8f, 12.0f, 16, stone);  // 2x: narrow column
        vase_bowl_   = CreateCylinder( 8.4f,  4.0f, 24, stone);  // 2x: shallow bowl at top

        // ── Moon — bright pale-yellow sphere high in the sky ─────────────
        moon_ = CreateSphere(20.0f, 16, 24, glm::vec3(0.96f, 0.96f, 0.82f));

        // ── Human figures (2x scale) ──────────────────────────────────────
        // Head: sphere, skin tone
        human_head_ = CreateSphere(2.0f, 10, 14, glm::vec3(0.82f, 0.62f, 0.44f));
        // Body: cylinder, blue clothing
        human_body_ = CreateCylinder(1.4f, 9.0f, 12, glm::vec3(0.20f, 0.30f, 0.72f));
        // Legs: dark cylinder (shared for all 4 legs across both figures)
        human_leg_  = CreateCylinder(0.6f, 6.0f, 10, glm::vec3(0.12f, 0.10f, 0.12f));

        // ── Rocket mesh (trail rocket visual) ────────────────────────────
        // Body: dark metallic gray — high shininess for clear specular highlight
        rocket_body_ = CreateCylinder(3.0f, 12.0f, 16, glm::vec3(0.30f, 0.30f, 0.36f));
        // Nose cone: bright red — medium shininess
        rocket_nose_ = CreateCone(   3.0f,  7.5f, 16, glm::vec3(0.88f, 0.10f, 0.10f));

        // ── Particle system ───────────────────────────────────────────────
        particle_sys_ = new ParticleSystem();
        InitParticleGPU();

        glfwSetKeyCallback(window_, KeyCallback);
        glfwSetFramebufferSizeCallback(window_, ResizeCallback);
        PrintControls();
        last_time_ = glfwGetTime();

        // ── Main loop ─────────────────────────────────────────────────────
        while (!glfwWindowShouldClose(window_)) {
            double now = glfwGetTime();
            float  dt  = (float)(now - last_time_);
            last_time_ = now;
            if (dt > 0.05f) dt = 0.05f;

            particle_sys_->Update(dt);

            glClearColor(0.01f, 0.01f, 0.06f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // 1. Skybox (depth writes off — always behind everything)
            RenderSkybox();

            // 2. Ground plane (opaque, depth write on)
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            RenderGround();

            // 3. Opaque 3D scene objects (road, vase, moon, humans)
            RenderObjects();

            // 4. Rocket meshes — visible 3D rocket for key 3 (Phong demo)
            RenderRockets();

            // 5. Particle sparks — additive blending, no depth write
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            RenderParticles();
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);

            glfwPollEvents();
            glfwSwapBuffers(window_);
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwTerminate();
    return 0;
}
