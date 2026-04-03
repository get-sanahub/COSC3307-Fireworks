#version 330

// Billboard quad vertices — already in world space from CPU
in vec3 vertex;    // world-space corner position
in vec4 color;     // RGBA (alpha carries fade)
in vec2 texcoord;  // UV [0,1] per corner

out vec4 vColor;
out vec2 vUV;

uniform mat4 view_mat;
uniform mat4 projection_mat;

void main()
{
    vColor      = color;
    vUV         = texcoord;
    gl_Position = projection_mat * view_mat * vec4(vertex, 1.0);
}
