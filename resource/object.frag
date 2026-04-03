#version 330

in vec3 vFragPos;
in vec3 vFragNorm;
in vec3 vColor;

// Main scene light (moon position in view space)
uniform vec3  lightPosView;
uniform float shininess;   // 32 = matte, 128 = metallic/shiny rocket

// Self-illuminated flag: 1.0 for moon sphere (skips Phong, just glows)
uniform float emissive;

// Dynamic explosion lights (same array as ground shader)
uniform vec3 dynLightsView[8];
uniform int  numDynLights;

out vec4 fragColor;

void main()
{
    // ── Self-illuminated objects (moon) ────────────────────────────────────
    if (emissive > 0.5) {
        fragColor = vec4(vColor * 1.5, 1.0);
        return;
    }

    vec3 N = normalize(vFragNorm);
    vec3 V = normalize(-vFragPos);          // direction toward camera
    vec3 L = normalize(lightPosView - vFragPos);  // direction toward moon

    // ── AMBIENT ────────────────────────────────────────────────────────────
    // Base low-level illumination — visible even on the shadowed side.
    // Simulates indirect/scattered moonlight.
    vec3 ambient = 0.18 * vColor;

    // ── DIFFUSE ────────────────────────────────────────────────────────────
    // Brightness proportional to angle between surface normal and light.
    // max(dot,0) ensures back-facing surfaces get zero diffuse.
    float diffFactor = max(dot(N, L), 0.0);
    vec3  diffuse    = 0.72 * diffFactor * vColor;

    // ── SPECULAR (Phong reflection model) ──────────────────────────────────
    // Shiny highlight: bright spot where reflected ray aligns with camera.
    // Higher shininess = smaller, sharper highlight (more metallic look).
    vec3  R          = reflect(-L, N);
    float specFactor = pow(max(dot(R, V), 0.0), shininess);
    vec3  specular   = 0.65 * specFactor * vec3(0.98, 0.98, 0.90);

    vec3 result = ambient + diffuse + specular;

    // ── Dynamic explosion lights ───────────────────────────────────────────
    // Each active firework burst adds a warm orange glow to nearby objects.
    for (int i = 0; i < numDynLights; i++) {
        vec3  Ldyn  = dynLightsView[i] - vFragPos;
        float dist  = length(Ldyn);
        float atten = 1.0 / (1.0 + 0.001 * dist * dist);
        float diff  = max(dot(normalize(Ldyn), N), 0.0);
        result += vColor * diff * atten * vec3(1.0, 0.60, 0.15) * 0.5;
    }

    fragColor = vec4(result, 1.0);
}
