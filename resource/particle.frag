#version 330

in  vec4 vColor;
in  vec2 vUV;
out vec4 fragColor;

void main()
{
    // Remap UV to [-1,1]
    vec2  c    = vUV * 2.0 - 1.0;
    float dist = length(c);
    if (dist > 1.0) discard;

    // Bright solid core (inner 40%) + wide soft glow ring out to edge
    float core = 1.0 - smoothstep(0.0, 0.4, dist);   // stays solid near centre
    float halo = 1.0 - smoothstep(0.0, 1.0, dist);   // falls off to edge
    float alpha = clamp(core * 0.9 + halo * 0.6, 0.0, 1.0);

    // Slightly overbright the RGB so sparks pop on a projector
    vec3 brightColor = vColor.rgb * 1.4;

    fragColor = vec4(brightColor, vColor.a * alpha);
}
