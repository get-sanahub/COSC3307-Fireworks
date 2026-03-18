/*
COSC 3307 - Project: Interactive Fireworks Display
Particle system - header

Particle pool manages rockets (launch phase) and spark particles (explosion phase).
Phase 1 (50%): Starburst firework type.
Phase 2 (TODO): Fountain, Rocket-with-trail, Cascade types + textures + dynamic lights.
*/
#ifndef PARTICLE_H_
#define PARTICLE_H_

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// -----------------------------------------------------------------------
// Maximum pool sizes
// -----------------------------------------------------------------------
static const int MAX_PARTICLES = 3000;
static const int MAX_ROCKETS   = 32;

// -----------------------------------------------------------------------
// Firework types — only STARBURST implemented in Phase 1
// -----------------------------------------------------------------------
enum FireworkType {
    FIREWORK_STARBURST = 0
    // FIREWORK_FOUNTAIN  = 1   // Phase 2
    // FIREWORK_ROCKET    = 2   // Phase 2
    // FIREWORK_CASCADE   = 3   // Phase 2
};

// -----------------------------------------------------------------------
// A single spark particle (post-explosion)
// -----------------------------------------------------------------------
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;    // base RGB
    float     alpha;    // fades 1 -> 0 as particle dies
    float     life;     // remaining life in seconds
    float     maxLife;  // total lifespan in seconds
    float     size;     // rendered point size in pixels
    bool      active;
};

// -----------------------------------------------------------------------
// Rocket — flies upward, then explodes into sparks
// -----------------------------------------------------------------------
struct Rocket {
    glm::vec3    position;
    glm::vec3    velocity;
    glm::vec3    color;     // colour passed on to explosion sparks
    float        life;      // countdown to explosion (seconds)
    bool         active;
    FireworkType type;
};

// -----------------------------------------------------------------------
// Vertex data sent to GPU each frame (dynamic VBO)
// Layout: position(vec3 @ 0) | color(vec4 @ 12) | size(float @ 28)
// -----------------------------------------------------------------------
struct ParticleVertex {
    glm::vec3 position;  // offset  0 — 12 bytes
    glm::vec4 color;     // offset 12 — 16 bytes  (alpha used for fade)
    float     size;      // offset 28 —  4 bytes
};                       // total  32 bytes

// -----------------------------------------------------------------------
// ParticleSystem — manages particle + rocket pools
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

    // Number of active (alive) particles this frame
    int ActiveCount() const { return activeCount_; }

private:
    Particle particles_[MAX_PARTICLES];
    Rocket   rockets_[MAX_ROCKETS];
    std::vector<ParticleVertex> vertices_;
    int activeCount_;

    // Spawn sparks from an exploding rocket
    void Explode(const Rocket& rocket);

    // Find the next free slot in each pool
    int FindFreeParticle();
    int FindFreeRocket();
};

#endif // PARTICLE_H_
