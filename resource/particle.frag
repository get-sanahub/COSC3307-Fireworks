#version 330

in  vec4 vColor;
out vec4 fragColor;

void main()
{
    // gl_PointCoord is (0,0) top-left, (1,1) bottom-right for the point sprite.
    // Map to [-0.5, 0.5] and compute distance from centre.
    vec2  coord = gl_PointCoord - vec2(0.5);
    float dist  = length(coord);

    // Discard the corners so the point renders as a circle, not a square.
    if (dist > 0.5) discard;

    // Quadratic fall-off: bright at centre, fades to edge — creates a soft glow.
    float t         = 1.0 - (dist * 2.0);
    float intensity = t * t;

    fragColor = vec4(vColor.rgb, vColor.a * intensity);
}
