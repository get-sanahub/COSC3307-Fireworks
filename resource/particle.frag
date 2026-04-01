#version 330

in  vec4 vColor;
out vec4 fragColor;

// Phase 2: procedural glow texture (64x64 circular alpha gradient)
// Sampled via gl_PointCoord (point-sprite UV, (0,0) top-left to (1,1) bottom-right).
uniform sampler2D glowTex;

void main()
{
    vec4 glow = texture(glowTex, gl_PointCoord);

    // Discard transparent edge fragments so additive blending doesn't accumulate
    // invisible quads everywhere.
    if (glow.a < 0.01) discard;

    // Modulate particle colour by the glow alpha mask
    fragColor = vec4(vColor.rgb, vColor.a * glow.a);
}
