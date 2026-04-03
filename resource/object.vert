#version 330

// 3D scene object vertex — position + normal + per-vertex colour
in vec3 vertex;
in vec3 normal;
in vec3 color;

out vec3 vFragPos;   // view-space position  (for lighting)
out vec3 vFragNorm;  // view-space normal    (for lighting)
out vec3 vColor;     // vertex colour passed through

uniform mat4 world_mat;
uniform mat4 view_mat;
uniform mat4 projection_mat;

void main()
{
    vec4 viewPos = view_mat * world_mat * vec4(vertex, 1.0);
    vFragPos  = vec3(viewPos);

    // Normal matrix = transpose(inverse(MV)) — correct for non-uniform scale
    mat4 mv       = view_mat * world_mat;
    vFragNorm     = vec3(transpose(inverse(mv)) * vec4(normal, 0.0));

    vColor      = color;
    gl_Position = projection_mat * viewPos;
}
