/*
COSC 3307 - Project: Interactive Fireworks Display
Particle system - header

Phase 1: Starburst firework.
Phase 2: Fountain (continuous ground emitter), Rocket-with-Trail, Cascade (multi-burst),
         plus dynamic point-light positions exposed to main for ground shading.
*/
#ifndef PARTICLE_H_
#define PARTICLE_H_

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// -----------------------------------------------------------------------
// Pool sizes
// -----------------------------------------------------------------------
static const int MAX_PARTICLES  = 5000;
static const int MAX_ROCKETS    = 64;   // larger pool: cascade spawns sub-rockets
static const int MAX_DYN_LIGHTS = 8;   // max firework positions sent to ground shader

// -----------------------------------------------------------------------
// Firework types
// -----------------------------------------------------------------------
enum FireworkType {
    FIREWORK_STARBURST = 0,  // rocket explodes into sphere of sparks
    FIREWORK_FOUNTAIN  = 1,  // continuous ground emitter, narrow upward cone
    FIREWORK_TRAIL     = 2,  // rocket leaves white-orange trail, then starburst
    FIREWORK_CASCADE   = 3   // rocket bursts into sparks + 3-5 secondary mini-rockets
};

// -----------------------------------------------------------------------
// Spark particle (post-explosion or fountain/trail)
// -----------------------------------------------------------------------
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;    // base RGB
    float     alpha;    // fades 1 -> 0
    float     life;     // remaining life (seconds)
    float     maxLife;  // total lifespan (seconds)
    float     size;     // rendered point size in pixels
    bool      active;
};

// -----------------------------------------------------------------------
// Rocket — flies upward then explodes (or acts as fountain emitter)
// -----------------------------------------------------------------------
struct Rocket {
    glm::vec3    position;
    glm::vec3    velocity;
    glm::vec3    color;
    float        life;         // countdown to explosion / deactivation
    bool         active;
    FireworkType type;
    float        trailTimer;   // TRAIL: time until next trail spark
    bool         isCascadeSecondary; // CASCADE sub-rockets don't recurse
    float        lightLife;    // seconds the explosion glow lingers (for dynamic lights)
};

// -----------------------------------------------------------------------
// Vertex data sent to GPU each frame (dynamic VBO)
// Layout: position(vec3 @ 0) | color(vec4 @ 12) | size(float @ 28)
// -----------------------------------------------------------------------
struct ParticleVertex {
    glm::vec3 position;  // offset  0 — 12 bytes
    glm::vec4 color;     // offset 12 — 16 bytes
    float     size;      // offset 28 —  4 bytes
};                       // total  32 bytes

// -----------------------------------------------------------------------
// ParticleSystem
// -----------------------------------------------------------------------
class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    // Launch a new firework from a ground position
    void Launch(glm::vec3 groundPos, FireworkType type);

    // Advance simulation by dt seconds
    void Update(float dt);

    // CPU-side vertex array ready to upload to GPU
    const std::vector<ParticleVertex>& GetVertices() const { return vertices_; }

    // Number of active particles this frame
    int ActiveCount() const { return activeCount_; }

    // Fill out[] with world-space positions of active explosion light sources.
    // Returns number of entries written (0 to MAX_DYN_LIGHTS).
    int GetActiveLightPositions(glm::vec3 out[MAX_DYN_LIGHTS]) const;

private:
    Particle particles_[MAX_PARTICLES];
    Rocket   rockets_[MAX_ROCKETS];
    std::vector<ParticleVertex> vertices_;
    int activeCount_;

    // Explode a rocket into spark particles
    void Explode(const Rocket& rocket, int numSparks);

    // Emit fountain sparks each frame
    void EmitFountainSparks(Rocket& r, float dt);

    // Pool helpers
    int FindFreeParticle();
    int FindFreeRocket();
};

#endif // PARTICLE_H_
