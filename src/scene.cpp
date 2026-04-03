/*
COSC 3307 - Project: Interactive Fireworks Display
Scene geometry — implementation.
Creates GPU buffers for the ground plane and skybox sphere,
and generates the procedural ground texture.
*/
#include <scene.h>
#include <glm/gtc/constants.hpp>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helper — upload an ObjectVertex mesh to GPU
// ─────────────────────────────────────────────────────────────────────────────
static Geometry* UploadObjectMesh(const std::vector<ObjectVertex>& verts,
                                   const std::vector<unsigned int>& idxs)
{
    Geometry* geom = new Geometry();
    geom->size = (GLuint)idxs.size();
    glGenVertexArrays(1, &geom->vao); glBindVertexArray(geom->vao);
    glGenBuffers(1, &geom->vbo);      glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(verts.size() * sizeof(ObjectVertex)),
                 verts.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &geom->ibo);      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(idxs.size() * sizeof(unsigned int)),
                 idxs.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
    return geom;
}

// ─────────────────────────────────────────────────────────────────────────────
// Cylinder — base at y=0, top at y=height
// ─────────────────────────────────────────────────────────────────────────────
Geometry* CreateCylinder(float radius, float height, int slices, glm::vec3 color)
{
    std::vector<ObjectVertex> verts;
    std::vector<unsigned int> idxs;
    float da = 2.0f * glm::pi<float>() / (float)slices;

    // Side ring — 2 vertices per slice step (bottom + top)
    for (int i = 0; i <= slices; i++) {
        float a = i * da;
        float x = cosf(a), z = sinf(a);
        glm::vec3 n(x, 0.0f, z);
        verts.push_back({ glm::vec3(radius*x, 0,      radius*z), n, color });
        verts.push_back({ glm::vec3(radius*x, height, radius*z), n, color });
    }
    for (int i = 0; i < slices; i++) {
        unsigned int b = i * 2;
        idxs.push_back(b);   idxs.push_back(b+2); idxs.push_back(b+1);
        idxs.push_back(b+1); idxs.push_back(b+2); idxs.push_back(b+3);
    }

    // Top cap
    unsigned int tc = (unsigned int)verts.size();
    verts.push_back({ glm::vec3(0, height, 0), glm::vec3(0,1,0), color });
    for (int i = 0; i <= slices; i++) {
        float a = i * da;
        verts.push_back({ glm::vec3(radius*cosf(a), height, radius*sinf(a)),
                          glm::vec3(0,1,0), color });
    }
    for (int i = 0; i < slices; i++) {
        idxs.push_back(tc);
        idxs.push_back(tc + 1 + i + 1);
        idxs.push_back(tc + 1 + i);
    }

    // Bottom cap
    unsigned int bc = (unsigned int)verts.size();
    verts.push_back({ glm::vec3(0, 0, 0), glm::vec3(0,-1,0), color });
    for (int i = 0; i <= slices; i++) {
        float a = i * da;
        verts.push_back({ glm::vec3(radius*cosf(a), 0, radius*sinf(a)),
                          glm::vec3(0,-1,0), color });
    }
    for (int i = 0; i < slices; i++) {
        idxs.push_back(bc);
        idxs.push_back(bc + 1 + i);
        idxs.push_back(bc + 1 + i + 1);
    }

    return UploadObjectMesh(verts, idxs);
}

// ─────────────────────────────────────────────────────────────────────────────
// Cone — base at y=0, tip at y=height
// ─────────────────────────────────────────────────────────────────────────────
Geometry* CreateCone(float radius, float height, int slices, glm::vec3 color)
{
    std::vector<ObjectVertex> verts;
    std::vector<unsigned int> idxs;
    float da = 2.0f * glm::pi<float>() / (float)slices;

    // Side — tip at top, ring at bottom
    float slopeN = radius / sqrtf(radius*radius + height*height); // outward slope for normals
    float slopeY = height / sqrtf(radius*radius + height*height);

    unsigned int tip = 0;
    verts.push_back({ glm::vec3(0, height, 0), glm::vec3(0,1,0), color }); // tip

    for (int i = 0; i <= slices; i++) {
        float a = i * da;
        float x = cosf(a), z = sinf(a);
        glm::vec3 n = glm::normalize(glm::vec3(x * slopeY, slopeN, z * slopeY));
        verts.push_back({ glm::vec3(radius*x, 0, radius*z), n, color });
    }
    for (int i = 0; i < slices; i++) {
        idxs.push_back(tip);
        idxs.push_back(1 + i);
        idxs.push_back(1 + i + 1);
    }

    // Bottom cap
    unsigned int bc = (unsigned int)verts.size();
    verts.push_back({ glm::vec3(0,0,0), glm::vec3(0,-1,0), color });
    for (int i = 0; i <= slices; i++) {
        float a = i * da;
        verts.push_back({ glm::vec3(radius*cosf(a), 0, radius*sinf(a)),
                          glm::vec3(0,-1,0), color });
    }
    for (int i = 0; i < slices; i++) {
        idxs.push_back(bc);
        idxs.push_back(bc + 1 + i);
        idxs.push_back(bc + 1 + i + 1);
    }

    return UploadObjectMesh(verts, idxs);
}

// ─────────────────────────────────────────────────────────────────────────────
// Sphere — centred at origin
// ─────────────────────────────────────────────────────────────────────────────
Geometry* CreateSphere(float radius, int stacks, int slices, glm::vec3 color)
{
    std::vector<ObjectVertex> verts;
    std::vector<unsigned int> idxs;

    for (int lat = 0; lat <= stacks; lat++) {
        float theta = glm::pi<float>() * (float)lat / (float)stacks;
        for (int lon = 0; lon <= slices; lon++) {
            float phi = 2.0f * glm::pi<float>() * (float)lon / (float)slices;
            glm::vec3 n(sinf(theta)*cosf(phi), cosf(theta), sinf(theta)*sinf(phi));
            verts.push_back({ n * radius, n, color });
        }
    }
    for (int lat = 0; lat < stacks; lat++) {
        for (int lon = 0; lon < slices; lon++) {
            unsigned int a = lat*(slices+1)+lon, b=a+1, c=a+(slices+1), d=c+1;
            idxs.push_back(a); idxs.push_back(b); idxs.push_back(c);
            idxs.push_back(b); idxs.push_back(d); idxs.push_back(c);
        }
    }
    return UploadObjectMesh(verts, idxs);
}

// ─────────────────────────────────────────────────────────────────────────────
// Road strip — flat quad, normal pointing up (y-up)
// ─────────────────────────────────────────────────────────────────────────────
Geometry* CreateRoadStrip(float halfW, float halfL, float yOffset, glm::vec3 color)
{
    glm::vec3 n(0,1,0);
    std::vector<ObjectVertex> verts = {
        { glm::vec3(-halfW, yOffset, -halfL), n, color },
        { glm::vec3( halfW, yOffset, -halfL), n, color },
        { glm::vec3( halfW, yOffset,  halfL), n, color },
        { glm::vec3(-halfW, yOffset,  halfL), n, color },
    };
    std::vector<unsigned int> idxs = { 0,1,2, 0,2,3 };
    return UploadObjectMesh(verts, idxs);
}

// ── Procedural ground texture ────────────────────────────────────────────────
GLuint CreateGroundTexture() {
    const int W = 128, H = 128;
    unsigned char pixels[W * H * 3];

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int   i  = (y * W + x) * 3;
            float nx = (float)x / (float)W;
            float ny = (float)y / (float)H;
            float v  = 0.5f + 0.5f * sinf(nx * 37.0f) * cosf(ny * 31.0f);
            bool onGrid = (x % 16 == 0) || (y % 16 == 0);
            if (onGrid) {
                pixels[i+0] = 28; pixels[i+1] = 62; pixels[i+2] = 20;
            } else {
                pixels[i+0] = (unsigned char)(10 + v * 22);
                pixels[i+1] = (unsigned char)(32 + v * 52);
                pixels[i+2] = (unsigned char)(8  + v * 18);
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

// ── Ground plane ─────────────────────────────────────────────────────────────
Geometry* CreateGroundPlane(float halfSize) {
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const float tileCount = 20.0f;

    GroundVertex verts[4] = {
        { glm::vec3(-halfSize, 0.0f, -halfSize), up, glm::vec2(0.0f,      0.0f)      },
        { glm::vec3( halfSize, 0.0f, -halfSize), up, glm::vec2(tileCount, 0.0f)      },
        { glm::vec3( halfSize, 0.0f,  halfSize), up, glm::vec2(tileCount, tileCount) },
        { glm::vec3(-halfSize, 0.0f,  halfSize), up, glm::vec2(0.0f,      tileCount) },
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

    glBindVertexArray(0);
    return geom;
}

// ── Skybox sphere ─────────────────────────────────────────────────────────────
Geometry* CreateSkyboxSphere(float radius) {
    const int STACKS = 24, SLICES = 32;
    std::vector<SkyVertex>    verts;
    std::vector<unsigned int> idxs;

    for (int lat = 0; lat <= STACKS; lat++) {
        float theta = glm::pi<float>() * (float)lat / (float)STACKS;
        for (int lon = 0; lon <= SLICES; lon++) {
            float phi = 2.0f * glm::pi<float>() * (float)lon / (float)SLICES;
            SkyVertex v;
            v.pos = glm::vec3(radius * sinf(theta) * cosf(phi),
                              radius * cosf(theta),
                              radius * sinf(theta) * sinf(phi));
            verts.push_back(v);
        }
    }
    for (int lat = 0; lat < STACKS; lat++) {
        for (int lon = 0; lon < SLICES; lon++) {
            unsigned int a = lat * (SLICES+1) + lon;
            unsigned int b = a + 1;
            unsigned int c = a + (SLICES+1);
            unsigned int d = c + 1;
            // Reversed winding — inside of sphere is the visible face
            idxs.push_back(a); idxs.push_back(c); idxs.push_back(b);
            idxs.push_back(b); idxs.push_back(c); idxs.push_back(d);
        }
    }

    Geometry* geom = new Geometry();
    geom->size = (GLuint)idxs.size();

    glGenVertexArrays(1, &geom->vao);
    glBindVertexArray(geom->vao);

    glGenBuffers(1, &geom->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(verts.size() * sizeof(SkyVertex)),
                 verts.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &geom->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(idxs.size() * sizeof(unsigned int)),
                 idxs.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    return geom;
}
