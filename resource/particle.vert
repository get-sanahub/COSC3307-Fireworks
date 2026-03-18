#version 330

// Per-particle data (uploaded via dynamic VBO each frame)
in vec3  vertex;   // world-space position
in vec4  color;    // RGBA — alpha carries the fade value
in float size;     // desired point size in pixels

out vec4 vColor;

uniform mat4 world_mat;
uniform mat4 view_mat;
uniform mat4 projection_mat;

void main()
{
    vColor       = color;
    gl_PointSize = size;
    gl_Position  = projection_mat * view_mat * world_mat * vec4(vertex, 1.0);
}
