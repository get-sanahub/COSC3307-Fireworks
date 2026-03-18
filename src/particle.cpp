/*
COSC 3307 - Project: Interactive Fireworks Display
Particle system - implementation

Physics model:
  - Rockets fly upward with initial velocity, slowed by gravity.
  - On expiry, a rocket explodes into NUM_SPARKS spark particles
    dispersed in a sphere (starburst pattern).
  - Each spark experiences gravity + slight drag, and fades out
    linearly over its lifespan.
*/

#include <particle.h>
#include <glm/gtc/constants.hpp>
#include <cstdlib>
#include <cmath>

// Random float in [lo, hi]
static float RandF(float lo, float hi) {
    return lo + (hi - lo) * (float)rand() / (float)RAND_MAX;
}

// -----------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------
ParticleSystem::ParticleSystem() : activeCount_(0) {
    for (int i = 0; i < MAX_PARTICLES; i++) particles_[i].active = false;
    for (int i = 0; i < MAX_ROCKETS;   i++) rockets_[i].active   = false;
    vertices_.resize(MAX_PARTICLES);
}

ParticleSystem::~ParticleSystem() {}

// -----------------------------------------------------------------------
// Pool helpers
// -----------------------------------------------------------------------
int ParticleSystem::FindFreeParticle() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles_[i].active) return i;
    }
    return -1; // pool exhausted
}

int ParticleSystem::FindFreeRocket() {
    for (int i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets_[i].active) return i;
    }
    return -1;
}

// -----------------------------------------------------------------------
// Launch — fire a rocket from groundPos
// -----------------------------------------------------------------------
void ParticleSystem::Launch(glm::vec3 groundPos, FireworkType type) {
    int idx = FindFreeRocket();
    if (idx < 0) return;

    Rocket& r  = rockets_[idx];
    r.position = groundPos;
    r.velocity = glm::vec3(RandF(-2.0f, 2.0f), RandF(45.0f, 60.0f), RandF(-2.0f, 2.0f));
    r.life     = RandF(1.4f, 2.2f); // time in seconds before it bursts
    r.active   = true;
    r.type     = type;

    // Choose a vivid random colour for this burst
    int c = rand() % 6;
    if      (c == 0) r.color = glm::vec3(1.0f, 0.30f, 0.10f); // red-orange
    else if (c == 1) r.color = glm::vec3(0.20f, 0.80f, 1.0f); // cyan-blue
    else if (c == 2) r.color = glm::vec3(1.0f, 0.90f, 0.10f); // golden yellow
    else if (c == 3) r.color = glm::vec3(0.65f, 0.20f, 1.0f); // purple
    else if (c == 4) r.color = glm::vec3(0.10f, 1.0f,  0.40f); // green
    else             r.color = glm::vec3(1.0f, 0.50f, 0.85f); // pink
}

// -----------------------------------------------------------------------
// Explode — spawn spark particles from an expired rocket
// -----------------------------------------------------------------------
void ParticleSystem::Explode(const Rocket& rocket) {
    const int NUM_SPARKS = 150; // starburst: sphere of sparks

    for (int i = 0; i < NUM_SPARKS; i++) {
        int idx = FindFreeParticle();
        if (idx < 0) break;

        Particle& p = particles_[idx];
        p.active   = true;
        p.position = rocket.position;
        p.color    = rocket.color;

        // Slight colour variation per spark so the burst looks richer
        p.color.r = glm::clamp(rocket.color.r + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
        p.color.g = glm::clamp(rocket.color.g + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
        p.color.b = glm::clamp(rocket.color.b + RandF(-0.15f, 0.15f), 0.0f, 1.0f);

        p.maxLife = RandF(1.2f, 3.0f);
        p.life    = p.maxLife;
        p.alpha   = 1.0f;
        p.size    = RandF(5.0f, 10.0f);

        // Uniform sphere dispersion (starburst pattern)
        float theta = RandF(0.0f, 2.0f * glm::pi<float>());
        float phi   = RandF(0.0f,        glm::pi<float>());
        float speed = RandF(8.0f, 22.0f);

        p.velocity = glm::vec3(
            speed * sinf(phi) * cosf(theta),
            speed * sinf(phi) * sinf(theta),
            speed * cosf(phi)
        );
    }
}

// -----------------------------------------------------------------------
// Update — advance simulation by dt seconds
// -----------------------------------------------------------------------
void ParticleSystem::Update(float dt) {
    const glm::vec3 GRAVITY(0.0f, -9.8f, 0.0f);
    const float     DRAG = 0.985f; // per-frame velocity multiplier

    // --- Rockets ---
    for (int i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets_[i].active) continue;
        Rocket& r = rockets_[i];

        r.velocity += GRAVITY * dt;
        r.position += r.velocity * dt;
        r.life     -= dt;

        if (r.life <= 0.0f) {
            Explode(r);
            r.active = false;
        }
    }

    // --- Spark particles ---
    activeCount_ = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles_[i].active) continue;
        Particle& p = particles_[i];

        p.velocity *= DRAG;
        p.velocity += GRAVITY * dt;
        p.position += p.velocity * dt;
        p.life     -= dt;

        // Linear fade-out; also shrink point size as particle ages
        float t = p.life / p.maxLife;  // 1 when born, 0 when dead
        p.alpha  = t;
        p.size   = t * 8.0f + 1.5f;

        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        }

        // Write into the vertex upload buffer
        ParticleVertex& v = vertices_[activeCount_++];
        v.position = p.position;
        v.color    = glm::vec4(p.color, p.alpha);
        v.size     = p.size;
    }
}
