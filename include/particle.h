/*
COSC 3307 - Project: Interactive Fireworks Display
Particle system header - billboard quad rendering (no GL_POINTS).
*/
#ifndef PARTICLE_H_
#define PARTICLE_H_

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// Pool sizes
static const int MAX_PARTICLES  = 3000;
static const int MAX_ROCKETS    = 64;
static const int MAX_DYN_LIGHTS = 8;

// Firework types
enum FireworkType {
    FIREWORK_STARBURST = 0,  // instant sphere burst (key 1)
    FIREWORK_FOUNTAIN  = 1,  // continuous ground emitter (key 2)
    FIREWORK_TRAIL     = 2,  // rocket with trail then burst (key 3)
    FIREWORK_CASCADE   = 3   // multi-burst cascade (key 4)
};

// GPU vertex: position + RGBA + UV for circle drawing
// Layout: x,y,z (12) | r,g,b,a (16) | u,v (8) = 36 bytes, all plain floats
struct ParticleVertex {
    float x, y, z;        // offset  0 — world-space position
    float r, g, b, a;     // offset 12 — RGBA colour
    float u, v;            // offset 28 — UV [0,1] for analytical circle
};                         // total  36 bytes

// Internal particle state
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float     alpha;
    float     life;
    float     maxLife;
    float     size;     // world-space billboard half-size at birth
    bool      active;
};

// Rocket (used for TRAIL and CASCADE types only)
struct Rocket {
    glm::vec3    position;
    glm::vec3    velocity;
    glm::vec3    color;
    float        life;
    bool         active;
    FireworkType type;
    float        trailTimer;
    bool         isCascadeSecondary;
    float        lightLife;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    // Launch a firework.
    // STARBURST / FOUNTAIN: spawn immediately from groundPos.
    // TRAIL / CASCADE    : launch a rocket from groundPos.
    void Launch(glm::vec3 groundPos, FireworkType type);

    // Physics update
    void Update(float dt);

    // Build billboard quad mesh for active particles.
    // Fills verts[0..4N-1] and idxs[0..6N-1] where N = active particle count.
    // camRight and camUp are world-space camera axes (from view matrix rows 0 & 1).
    // Returns N (active particle count).
    int BuildBillboardMesh(ParticleVertex* verts, unsigned int* idxs,
                           const glm::vec3& camRight,
                           const glm::vec3& camUp) const;

    int ActiveCount() const { return activeCount_; }

    // World-space explosion positions for dynamic ground lighting
    int GetActiveLightPositions(glm::vec3 out[MAX_DYN_LIGHTS]) const;

    // Info about active TRAIL rockets so the renderer can draw a rocket mesh
    struct RocketInfo {
        glm::vec3 position;
        glm::vec3 velocity;  // used to orient the rocket mesh along flight direction
        glm::vec3 color;
    };
    int GetActiveTrailRockets(RocketInfo* out, int maxCount) const;

private:
    Particle particles_[MAX_PARTICLES];
    Rocket   rockets_[MAX_ROCKETS];
    int      activeCount_;

    // Fountain state
    glm::vec3 fountainPos_;
    float     fountainLife_;
    float     fountainTimer_;
    bool      fountainActive_;

    void Explode(glm::vec3 pos, glm::vec3 color, int numSparks, float minSpeed, float maxSpeed);
    void EmitFountainSpark();
    int  FindFreeParticle();
    int  FindFreeRocket();
};

#endif // PARTICLE_H_
