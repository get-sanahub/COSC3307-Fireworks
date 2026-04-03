/*
COSC 3307 - Project: Interactive Fireworks Display
Particle system - billboard quad rendering (no GL_POINTS).
*/

#include <particle.h>
#include <glm/gtc/constants.hpp>
#include <cstdlib>
#include <cmath>

static float RandF(float lo, float hi) {
    return lo + (hi - lo) * (float)rand() / (float)RAND_MAX;
}

static glm::vec3 RandomColor() {
    int c = rand() % 6;
    if      (c == 0) return glm::vec3(1.0f,  0.30f, 0.10f);  // red-orange
    else if (c == 1) return glm::vec3(0.20f, 0.80f, 1.0f);   // cyan-blue
    else if (c == 2) return glm::vec3(1.0f,  0.90f, 0.10f);  // golden yellow
    else if (c == 3) return glm::vec3(0.65f, 0.20f, 1.0f);   // purple
    else if (c == 4) return glm::vec3(0.10f, 1.0f,  0.40f);  // green
    else             return glm::vec3(1.0f,  0.50f, 0.85f);  // pink
}

// -----------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------
ParticleSystem::ParticleSystem()
    : activeCount_(0),
      fountainPos_(0.0f),
      fountainLife_(0.0f),
      fountainTimer_(0.0f),
      fountainActive_(false)
{
    for (int i = 0; i < MAX_PARTICLES; i++) particles_[i].active = false;
    for (int i = 0; i < MAX_ROCKETS;   i++) {
        rockets_[i].active    = false;
        rockets_[i].lightLife = 0.0f;
    }
}

ParticleSystem::~ParticleSystem() {}

// -----------------------------------------------------------------------
// Pool helpers
// -----------------------------------------------------------------------
int ParticleSystem::FindFreeParticle() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles_[i].active) return i;
    }
    return -1;
}

int ParticleSystem::FindFreeRocket() {
    for (int i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets_[i].active) return i;
    }
    return -1;
}

// -----------------------------------------------------------------------
// Explode — sphere burst of numSparks particles at pos
// -----------------------------------------------------------------------
void ParticleSystem::Explode(glm::vec3 pos, glm::vec3 color,
                              int numSparks, float minSpeed, float maxSpeed)
{
    for (int i = 0; i < numSparks; i++) {
        int idx = FindFreeParticle();
        if (idx < 0) break;

        Particle& p = particles_[idx];
        p.active   = true;
        p.position = pos;
        p.color.r  = glm::clamp(color.r + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
        p.color.g  = glm::clamp(color.g + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
        p.color.b  = glm::clamp(color.b + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
        p.maxLife  = RandF(4.0f, 7.0f);
        p.life     = p.maxLife;
        p.alpha    = 1.0f;
        p.size     = RandF(1.2f, 2.8f);  // 2x — visible on projector

        float theta = RandF(0.0f, 2.0f * glm::pi<float>());
        float phi   = RandF(0.0f, glm::pi<float>());
        float speed = RandF(minSpeed, maxSpeed);
        p.velocity  = glm::vec3(
            speed * sinf(phi) * cosf(theta),
            speed * sinf(phi) * sinf(theta),
            speed * cosf(phi));
    }
}

// -----------------------------------------------------------------------
// EmitFountainSpark — one spark from fountainPos_ in a narrow upward cone
// -----------------------------------------------------------------------
void ParticleSystem::EmitFountainSpark() {
    int idx = FindFreeParticle();
    if (idx < 0) return;

    Particle& p = particles_[idx];
    p.active   = true;
    p.position = fountainPos_;
    p.color    = RandomColor();
    p.maxLife  = RandF(5.0f, 8.0f);
    p.life     = p.maxLife;
    p.alpha    = 1.0f;
    p.size     = RandF(1.0f, 2.0f);  // 2x fountain spark

    const float CONE = glm::radians(35.0f);
    float az    = RandF(0.0f, 2.0f * glm::pi<float>());
    float cone  = RandF(0.0f, CONE);
    float speed = RandF(110.0f, 180.0f);
    p.velocity  = glm::vec3(
        speed * sinf(cone) * cosf(az),
        speed * cosf(cone),
        speed * sinf(cone) * sinf(az));
}

// -----------------------------------------------------------------------
// Launch
// -----------------------------------------------------------------------
void ParticleSystem::Launch(glm::vec3 groundPos, FireworkType type) {

    if (type == FIREWORK_STARBURST) {
        // Immediate burst — no rocket phase
        glm::vec3 burstPos = groundPos + glm::vec3(0.0f, 90.0f, 0.0f);
        glm::vec3 col      = RandomColor();
        Explode(burstPos, col, 200, 50.0f, 120.0f);
        // Store position for dynamic ground light
        int lidx = FindFreeRocket();
        if (lidx >= 0) {
            rockets_[lidx].active    = false;
            rockets_[lidx].lightLife = 1.5f;
            rockets_[lidx].position  = burstPos;
        }
        return;
    }

    if (type == FIREWORK_FOUNTAIN) {
        fountainActive_ = true;
        fountainPos_    = groundPos;
        fountainLife_   = RandF(7.0f, 8.0f);
        fountainTimer_  = 0.0f;
        return;
    }

    // TRAIL and CASCADE — launch a rocket
    int idx = FindFreeRocket();
    if (idx < 0) return;

    Rocket& r            = rockets_[idx];
    r.active             = true;
    r.type               = type;
    r.color              = RandomColor();
    r.trailTimer         = 0.0f;
    r.isCascadeSecondary = false;
    r.lightLife          = 0.0f;
    r.position           = groundPos;
    r.velocity           = glm::vec3(RandF(-2.0f, 2.0f),
                                      RandF(45.0f, 60.0f),
                                      RandF(-2.0f, 2.0f));
    r.life               = RandF(1.4f, 2.2f);
}

// -----------------------------------------------------------------------
// GetActiveLightPositions
// -----------------------------------------------------------------------
int ParticleSystem::GetActiveTrailRockets(RocketInfo* out, int maxCount) const {
    int n = 0;
    for (int i = 0; i < MAX_ROCKETS && n < maxCount; i++) {
        if (rockets_[i].active && rockets_[i].type == FIREWORK_TRAIL) {
            out[n].position = rockets_[i].position;
            out[n].velocity = rockets_[i].velocity;
            out[n].color    = rockets_[i].color;
            n++;
        }
    }
    return n;
}

int ParticleSystem::GetActiveLightPositions(glm::vec3 out[MAX_DYN_LIGHTS]) const {
    int n = 0;
    for (int i = 0; i < MAX_ROCKETS && n < MAX_DYN_LIGHTS; i++) {
        if (rockets_[i].lightLife > 0.0f)
            out[n++] = rockets_[i].position;
    }
    return n;
}

// -----------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------
void ParticleSystem::Update(float dt) {
    const glm::vec3 GRAVITY(0.0f, -9.8f, 0.0f);
    const float     DRAG = 0.995f;

    // Tick light timers
    for (int i = 0; i < MAX_ROCKETS; i++) {
        if (rockets_[i].lightLife > 0.0f)
            rockets_[i].lightLife -= dt;
    }

    // Fountain emitter
    if (fountainActive_) {
        fountainLife_ -= dt;
        if (fountainLife_ <= 0.0f) {
            fountainActive_ = false;
        } else {
            fountainTimer_ -= dt;
            if (fountainTimer_ <= 0.0f) {
                fountainTimer_ = 1.0f / 70.0f;  // 70 sparks/sec
                EmitFountainSpark();
            }
        }
    }

    // Rockets (TRAIL and CASCADE only)
    for (int i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets_[i].active) continue;
        Rocket& r = rockets_[i];

        r.velocity += GRAVITY * dt;
        r.position += r.velocity * dt;
        r.life     -= dt;

        if (r.type == FIREWORK_TRAIL) {
            r.trailTimer -= dt;
            if (r.trailTimer <= 0.0f) {
                r.trailTimer = 0.025f;
                int tidx = FindFreeParticle();
                if (tidx >= 0) {
                    Particle& tp = particles_[tidx];
                    tp.active   = true;
                    tp.position = r.position + glm::vec3(
                        RandF(-0.4f, 0.4f), 0.0f, RandF(-0.4f, 0.4f));
                    tp.velocity = -r.velocity * 0.25f
                                + glm::vec3(RandF(-2.0f, 2.0f),
                                            RandF(-1.5f, 0.5f),
                                            RandF(-2.0f, 2.0f));
                    tp.color    = glm::vec3(1.0f, RandF(0.5f, 1.0f), RandF(0.0f, 0.25f));
                    tp.maxLife  = RandF(0.15f, 0.35f);
                    tp.life     = tp.maxLife;
                    tp.alpha    = 1.0f;
                    tp.size     = RandF(0.6f, 1.2f);  // 2x trail spark
                }
            }
        }

        if (r.life <= 0.0f) {
            r.lightLife = 1.2f;

            if (r.type == FIREWORK_CASCADE && !r.isCascadeSecondary) {
                Explode(r.position, r.color, 80, 35.0f, 80.0f);

                int numSec = 3 + rand() % 3;
                for (int s = 0; s < numSec; s++) {
                    int sidx = FindFreeRocket();
                    if (sidx < 0) break;

                    Rocket& sr            = rockets_[sidx];
                    sr.active             = true;
                    sr.type               = FIREWORK_CASCADE;
                    sr.isCascadeSecondary = true;
                    sr.trailTimer         = 0.0f;
                    sr.lightLife          = 0.0f;
                    sr.position           = r.position;
                    sr.color.r = glm::clamp(r.color.r + RandF(-0.2f, 0.2f), 0.0f, 1.0f);
                    sr.color.g = glm::clamp(r.color.g + RandF(-0.2f, 0.2f), 0.0f, 1.0f);
                    sr.color.b = glm::clamp(r.color.b + RandF(-0.2f, 0.2f), 0.0f, 1.0f);

                    float theta = RandF(0.0f, 2.0f * glm::pi<float>());
                    float phi   = RandF(glm::pi<float>() * 0.05f, glm::pi<float>() * 0.40f);
                    float speed = RandF(12.0f, 22.0f);
                    sr.velocity = glm::vec3(
                        speed * sinf(phi) * cosf(theta),
                        speed * cosf(phi) * 0.8f + RandF(3.0f, 10.0f),
                        speed * sinf(phi) * sinf(theta));
                    sr.life = RandF(0.4f, 0.8f);
                }

            } else {
                int numSparks = r.isCascadeSecondary ? 55 : 150;
                Explode(r.position, r.color, numSparks, 40.0f, 100.0f);
            }

            r.active = false;
        }
    }

    // Spark particles — physics + count actives
    activeCount_ = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles_[i].active) continue;
        Particle& p = particles_[i];

        p.velocity *= DRAG;
        p.velocity += GRAVITY * dt;
        p.position += p.velocity * dt;
        p.life     -= dt;
        p.alpha     = p.life / p.maxLife;

        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        }
        activeCount_++;
    }
}

// -----------------------------------------------------------------------
// BuildBillboardMesh — one screen-aligned quad per active particle.
// verts must hold at least MAX_PARTICLES*4 ParticleVertex entries.
// idxs  must hold at least MAX_PARTICLES*6 unsigned int entries.
// Returns N (active particle count) = number of quads written.
// -----------------------------------------------------------------------
int ParticleSystem::BuildBillboardMesh(ParticleVertex* verts, unsigned int* idxs,
                                        const glm::vec3& camRight,
                                        const glm::vec3& camUp) const
{
    int n = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles_[i].active) continue;
        const Particle& p = particles_[i];

        float t = p.life / p.maxLife;          // 1 (fresh) → 0 (dead)
        float s = t * p.size + 0.2f;           // world-space half-size, stays visible

        glm::vec3 pos = p.position;
        glm::vec3 R   = camRight * s;
        glm::vec3 U   = camUp   * s;

        // 4 corners of the quad
        glm::vec3 c0 = pos - R - U;  // bottom-left
        glm::vec3 c1 = pos + R - U;  // bottom-right
        glm::vec3 c2 = pos + R + U;  // top-right
        glm::vec3 c3 = pos - R + U;  // top-left

        float r = p.color.r, g = p.color.g, b = p.color.b, a = p.alpha;
        int base = n * 4;

        verts[base+0] = {c0.x, c0.y, c0.z,  r, g, b, a,  0.0f, 0.0f};
        verts[base+1] = {c1.x, c1.y, c1.z,  r, g, b, a,  1.0f, 0.0f};
        verts[base+2] = {c2.x, c2.y, c2.z,  r, g, b, a,  1.0f, 1.0f};
        verts[base+3] = {c3.x, c3.y, c3.z,  r, g, b, a,  0.0f, 1.0f};

        int bi = n * 6;
        idxs[bi+0] = base+0;  idxs[bi+1] = base+1;  idxs[bi+2] = base+2;
        idxs[bi+3] = base+0;  idxs[bi+4] = base+2;  idxs[bi+5] = base+3;

        n++;
    }
    return n;
}
