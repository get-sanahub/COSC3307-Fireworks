#version 330

// Skybox sphere — vertex only needs world-space position as a direction vector.
in vec3 vertex;

// Direction passed to fragment shader for sky gradient + star computation.
out vec3 vDir;

uniform mat4 view_mat;
uniform mat4 projection_mat;

void main()
{
    vDir = vertex;

    // Strip translation from view so skybox is always centred on the camera.
    // mat3(view_mat) keeps only the rotation, mat4(...) re-pads to 4x4.
    mat4 viewRot = mat4(mat3(view_mat));

    // .xyww trick: after perspective divide, depth = w/w = 1.0 (far plane).
    // Combined with GL_LEQUAL depth test the skybox is always behind geometry.
    gl_Position = (projection_mat * viewRot * vec4(vertex, 1.0)).xyww;
}
