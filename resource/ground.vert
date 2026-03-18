#version 330

// Ground plane vertex — position + normal (no texcoords yet; Phase 2)
in vec3 vertex;
in vec3 normal;

// Passed to fragment shader for per-fragment Phong lighting
out vec3 vFragPos;
out vec3 vFragNorm;

uniform mat4 world_mat;
uniform mat4 view_mat;
uniform mat4 projection_mat;

void main()
{
    vec4 vtxPos = view_mat * world_mat * vec4(vertex, 1.0);

    // Normal matrix = transpose(inverse(MV)) — handles non-uniform scale correctly
    vFragNorm   = vec3(transpose(inverse(view_mat * world_mat)) * vec4(normal, 0.0));
    vFragPos    = vec3(vtxPos);

    gl_Position = projection_mat * vtxPos;
}
