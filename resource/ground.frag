#version 330

in vec3 vFragPos;
in vec3 vFragNorm;
in vec2 vUV;

// Static scene light — position already in view space (computed in C++)
uniform vec3 lightPosView;

// Phase 2: ground texture (procedural, no image file)
uniform sampler2D groundTex;

// Phase 2: dynamic point lights from active firework bursts (view space)
uniform vec3 dynLightsView[8];
uniform int  numDynLights;

out vec4 fragColor;

void main()
{
    vec3 N = normalize(vFragNorm);
    vec3 V = normalize(-vFragPos);   // camera at origin in view space

    // ------------------------------------------------------------------
    // Base colour from procedural ground texture
    // ------------------------------------------------------------------
    vec3 baseColor = texture(groundTex, vUV).rgb * 2.5;

    // ------------------------------------------------------------------
    // Static scene light (Phong) — same formulation as A2
    // ------------------------------------------------------------------
    vec3  L        = normalize(lightPosView - vFragPos);
    float ambient  = 0.12;
    float diffuse  = max(dot(L, N), 0.0) * 0.50;
    vec3  R        = reflect(-L, N);
    float specular = pow(max(dot(R, V), 0.0), 16.0) * 0.10;

    vec3 finalColor = baseColor * (ambient + diffuse) + vec3(specular);

    // ------------------------------------------------------------------
    // Dynamic point lights from active firework explosions
    // Warm orange-gold glow, quadratic attenuation
    // ------------------------------------------------------------------
    for (int i = 0; i < numDynLights; i++) {
        vec3  Ldyn  = dynLightsView[i] - vFragPos;
        float dist  = length(Ldyn);
        float atten = 1.0 / (1.0 + 0.0008 * dist * dist);
        float diff  = max(dot(normalize(Ldyn), N), 0.0);
        finalColor += baseColor * diff * atten * vec3(1.0, 0.65, 0.2) * 0.55;
    }

    fragColor = vec4(finalColor, 1.0);
}
