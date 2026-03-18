#version 330

in vec3 vFragPos;
in vec3 vFragNorm;

// Light position already transformed to view space (done in C++)
uniform vec3 lightPosView;

// Base colour of the ground (dark grass)
uniform vec3 ground_color;

out vec4 fragColor;

void main()
{
    vec3 N = normalize(vFragNorm);
    vec3 L = normalize(lightPosView - vFragPos);
    // Camera is at the origin in view space
    vec3 V = normalize(-vFragPos);

    // Phong model — same structure as A2 shaderPhong.frag
    float ambient  = 0.15;
    float diffuse  = max(dot(L, N), 0.0) * 0.55;

    vec3  R        = reflect(-L, N);
    float specular = pow(max(dot(R, V), 0.0), 16.0) * 0.15;

    vec3 finalColor = ground_color * (ambient + diffuse) + vec3(specular);
    fragColor = vec4(finalColor, 1.0);
}
