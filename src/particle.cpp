/*
COSC 3307 - Project: Interactive Fireworks Display
Particle system - implementation

Physics model:
  - Rockets fly upward, slowed by gravity, then expire and explode.
  - STARBURST : sphere of 150 sparks at the burst point.
  - FOUNTAIN  : no rocket phase; a ground-level slot continuously emits sparks
                in a narrow upward cone for 5-8 seconds.
  - TRAIL     : rocket emits white-orange trail sparks while rising,
                then a full starburst on expiry.
  - CASCADE   : rocket bursts into ~80 sparks PLUS 3-5 secondary mini-rockets
                that each travel 0.4-0.8 s before exploding with ~55 sparks.
  - All sparks experience gravity + slight drag and fade linearly.
*/

#include <particle.h>
#include <glm/gtc/constants.hpp>
#include <cstdlib>
#include <cmath>

// Random float in [lo, hi]
static float RandF(float lo, float hi) {
    return lo + (hi - lo) * (float)rand() / (float)RAND_MAX;
}

// Pick a vivid random colour
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
ParticleSystem::ParticleSystem() : activeCount_(0) {
    for (int i = 0; i < MAX_PARTICLES; i++) particles_[i].active = false;
    for (int i = 0; i < MAX_ROCKETS;   i++) {
        rockets_[i].active    = false;
        rockets_[i].lightLife = 0.0f;
    }
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
    return -1;
}

int ParticleSystem::FindFreeRocket() {
    for (int i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets_[i].active) return i;
    }
    return -1;
}

// -----------------------------------------------------------------------
// Launch
// -----------------------------------------------------------------------
void ParticleSystem::Launch(glm::vec3 groundPos, FireworkType type) {
    int idx = FindFreeRocket();
    if (idx < 0) return;

    Rocket& r             = rockets_[idx];
    r.active              = true;
    r.type                = type;
    r.color               = RandomColor();
    r.trailTimer          = 0.0f;
    r.isCascadeSecondary  = false;
    r.lightLife           = 0.0f;
    r.position            = groundPos;

    switch (type) {
        case FIREWORK_FOUNTAIN:
            r.velocity = glm::vec3(0.0f);          // stays on ground
            r.life     = RandF(5.0f, 8.0f);        // emitter duration
            break;

        case FIREWORK_TRAIL:
            r.velocity = glm::vec3(RandF(-1.5f, 1.5f), RandF(50.0f, 65.0f), RandF(-1.5f, 1.5f));
            r.life     = RandF(1.6f, 2.4f);
            break;

        case FIREWORK_CASCADE:
        case FIREWORK_STARBURST:
        default:
            r.velocity = glm::vec3(RandF(-2.0f, 2.0f), RandF(45.0f, 60.0f), RandF(-2.0f, 2.0f));
            r.life     = RandF(1.4f, 2.2f);
            break;
    }
}

// -----------------------------------------------------------------------
// Explode — spawn numSparks particles in a sphere around rocket.position
// -----------------------------------------------------------------------
void ParticleSystem::Explode(const Rocket& rocket, int numSparks) {
    for (int i = 0; i < numSparks; i++) {
        int idx = FindFreeParticle();
        if (idx < 0) break;

        Particle& p = particles_[idx];
        p.active    = true;
        p.position  = rocket.position;
        p.color.r   = glm::clamp(rocket.color.r + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
        p.color.g   = glm::clamp(rocket.color.g + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
        p.color.b   = glm::clamp(rocket.color.b + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
        p.maxLife   = RandF(1.2f, 3.0f);
        p.life      = p.maxLife;
        p.alpha     = 1.0f;
        p.size      = RandF(4.0f, 9.0f);

        // Uniform sphere dispersion
        float theta = RandF(0.0f, 2.0f * glm::pi<float>());
        float phi   = RandF(0.0f, glm::pi<float>());
        float speed = RandF(6.0f, 22.0f);
        p.velocity  = glm::vec3(
            speed * sinf(phi) * cosf(theta),
            speed * sinf(phi) * sinf(theta),
            speed * cosf(phi));
    }
}

// -----------------------------------------------------------------------
// EmitFountainSparks — called every frame for FOUNTAIN rockets
// -----------------------------------------------------------------------
void ParticleSystem::EmitFountainSparks(Rocket& r, float dt) {
    const float RATE            = 60.0f;   // sparks per second
    const float EMIT_INTERVAL   = 1.0f / RATE;
    const float CONE_HALF_ANGLE = glm::radians(18.0f);

    r.trailTimer -= dt;
    if (r.trailTimer > 0.0f) return;
    r.trailTimer = EMIT_INTERVAL;

    int idx = FindFreeParticle();
    if (idx < 0) return;

    Particle& p = particles_[idx];
    p.active    = true;
    p.position  = r.position;
    p.color.r   = glm::clamp(r.color.r + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
    p.color.g   = glm::clamp(r.color.g + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
    p.color.b   = glm::clamp(r.color.b + RandF(-0.15f, 0.15f), 0.0f, 1.0f);
    p.maxLife   = RandF(0.7f, 1.4f);
    p.life      = p.maxLife;
    p.alpha     = 1.0f;
    p.size      = RandF(4.0f, 7.0f);

    // Narrow upward cone
    float az    = RandF(0.0f, 2.0f * glm::pi<float>());
    float cone  = RandF(0.0f, CONE_HALF_ANGLE);
    float speed = RandF(14.0f, 24.0f);
    p.velocity  = glm::vec3(
        speed * sinf(cone) * cosf(az),
        speed * cosf(cone),
        speed * sinf(cone) * sinf(az));
}

// -----------------------------------------------------------------------
// GetActiveLightPositions
// -----------------------------------------------------------------------
int ParticleSystem::GetActiveLightPositions(glm::vec3 out[MAX_DYN_LIGHTS]) const {
    int n = 0;
    for (int i = 0; i < MAX_ROCKETS && n < MAX_DYN_LIGHTS; i++) {
        if (rockets_[i].lightLife > 0.0f) {
            out[n++] = rockets_[i].position;
        }
    }
    return n;
}

// -----------------------------------------------------------------------
// Update — advance simulation by dt seconds
// -----------------------------------------------------------------------
void ParticleSystem::Update(float dt) {
    const glm::vec3 GRAVITY(0.0f, -9.8f, 0.0f);
    const float     DRAG = 0.985f;

    // Tick down all light-life values (even for inactive rockets)
    for (int i = 0; i < MAX_ROCKETS; i++) {
        if (rockets_[i].lightLife > 0.0f)
            rockets_[i].lightLife -= dt;
    }

    // --- Rockets ---
    for (int i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets_[i].active) continue;
        Rocket& r = rockets_[i];

        if (r.type == FIREWORK_FOUNTAIN) {
            // Fountain: no movement, just emit sparks
            EmitFountainSparks(r, dt);
            r.life -= dt;
            if (r.life <= 0.0f) r.active = false;
            continue;
        }

        // Physics for all rocket types
        r.velocity += GRAVITY * dt;
        r.position += r.velocity * dt;
        r.life     -= dt;

        // TRAIL: emit sparks while ascending
        if (r.type == FIREWORK_TRAIL) {
            r.trailTimer -= dt;
            if (r.trailTimer <= 0.0f) {
                r.trailTimer = 0.025f;  // 40 sparks/sec
                int tidx = FindFreeParticle();
                if (tidx >= 0) {
                    Particle& tp = particles_[tidx];
                    tp.active    = true;
                    tp.position  = r.position + glm::vec3(RandF(-0.4f, 0.4f), 0.0f, RandF(-0.4f, 0.4f));
                    // Trail drifts backward and slightly outward
                    tp.velocity  = -r.velocity * 0.25f
                                 + glm::vec3(RandF(-2.0f, 2.0f), RandF(-1.5f, 0.5f), RandF(-2.0f, 2.0f));
                    tp.color     = glm::vec3(1.0f, RandF(0.5f, 1.0f), RandF(0.0f, 0.25f));
                    tp.maxLife   = RandF(0.15f, 0.35f);
                    tp.life      = tp.maxLife;
                    tp.alpha     = 1.0f;
                    tp.size      = RandF(3.0f, 6.0f);
                }
            }
        }

        if (r.life <= 0.0f) {
            // Mark explosion position as a dynamic light source
            r.lightLife = 1.2f;

            if (r.type == FIREWORK_CASCADE && !r.isCascadeSecondary) {
                // Primary cascade: ~80 sparks + secondary mini-rockets
                Explode(r, 80);

                int numSecondary = 3 + rand() % 3;  // 3-5
                for (int s = 0; s < numSecondary; s++) {
                    int sidx = FindFreeRocket();
                    if (sidx < 0) break;

                    Rocket& sr            = rockets_[sidx];
                    sr.active             = true;
                    sr.type               = FIREWORK_CASCADE;
                    sr.isCascadeSecondary = true;
                    sr.trailTimer         = 0.0f;
                    sr.lightLife          = 0.0f;
                    sr.position           = r.position;
                    sr.color.r            = glm::clamp(r.color.r + RandF(-0.2f, 0.2f), 0.0f, 1.0f);
                    sr.color.g            = glm::clamp(r.color.g + RandF(-0.2f, 0.2f), 0.0f, 1.0f);
                    sr.color.b            = glm::clamp(r.color.b + RandF(-0.2f, 0.2f), 0.0f, 1.0f);

                    // Spread in a wide hemisphere
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
                // STARBURST, TRAIL, cascade secondary — all produce a sphere burst
                int numSparks = r.isCascadeSecondary ? 55 : 150;
                Explode(r, numSparks);
            }

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

        float t  = p.life / p.maxLife;
        p.alpha  = t;
        p.size   = t * 8.0f + 1.5f;

        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        }

        ParticleVertex& v = vertices_[activeCount_++];
        v.position = p.position;
        v.color    = glm::vec4(p.color, p.alpha);
        v.size     = p.size;
    }
}
