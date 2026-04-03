#version 330

in  vec3 vDir;
out vec4 fragColor;

// Simple hash: vec2 -> float in [0, 1]
float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    vec3  d = normalize(vDir);
    float y = d.y;  // -1 (nadir) to +1 (zenith)

    // ------------------------------------------------------------------
    // Night-sky gradient: dark navy blue at horizon, near-black at zenith
    // ------------------------------------------------------------------
    float t   = clamp(y * 0.5 + 0.5, 0.0, 1.0);       // 0 = horizon, 1 = zenith
    vec3  sky = mix(vec3(0.04, 0.04, 0.14),             // dark blue horizon
                    vec3(0.005, 0.005, 0.02),            // near-black zenith
                    t * t);

    // ------------------------------------------------------------------
    // Procedural star field (visible above the horizon only)
    // ------------------------------------------------------------------
    if (y > 0.02) {
        // Spherical coordinates: azimuth and elevation
        float az  = atan(d.z, d.x);              // [-pi, pi]
        float el  = asin(clamp(d.y, -1.0, 1.0)); // [-pi/2, pi/2]

        // Quantise into a grid — each cell may or may not contain a star
        float cellScale = 38.0;
        vec2  cell      = floor(vec2(az, el) * cellScale);
        float r         = hash2(cell);

        // Only ~2 % of cells contain a star
        if (r > 0.978) {
            // Sub-cell position; star is a tiny bright disc near the cell centre
            vec2  frac   = fract(vec2(az, el) * cellScale) - 0.5;
            float dist   = length(frac);
            float bright = (r - 0.978) * 45.0;        // brighter for higher r
            float glow   = bright * (1.0 - smoothstep(0.0, 0.22, dist));
            // Slight blue-white tint for stars
            sky += vec3(glow * 0.85, glow * 0.90, glow);
        }
    }

    fragColor = vec4(sky, 1.0);
}
