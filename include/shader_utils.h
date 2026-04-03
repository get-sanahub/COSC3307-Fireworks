/*
COSC 3307 - Project: Interactive Fireworks Display
Shader utility functions — load, compile, link GLSL shaders,
and set uniform variables from CPU-side values.
*/
#ifndef SHADER_UTILS_H_
#define SHADER_UTILS_H_

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

// Shaders are copied next to the executable by CMake POST_BUILD.
#define SHADER_DIRECTORY "resource/"

// Load a plain text file into a std::string.
std::string LoadTextFile(const std::string& filename);

// Compile shaderName.vert + shaderName.frag and link into a program.
// Throws std::ios_base::failure on any compile / link error.
GLuint LoadShaders(const std::string& shaderName);

// Set a mat4 uniform by name.
void LoadShaderMatrix(GLuint shader, const glm::mat4& matrix, const std::string& name);

// Set a vec3 uniform by name.
void LoadShaderVec3(GLuint shader, const glm::vec3& v, const std::string& name);

// Set a float uniform by name.
void LoadShaderFloat(GLuint shader, float val, const std::string& name);

// Set an int uniform by name.
void LoadShaderInt(GLuint shader, int val, const std::string& name);

#endif // SHADER_UTILS_H_
