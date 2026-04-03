/*
COSC 3307 - Project: Interactive Fireworks Display
Shader utility functions — implementation.
*/
#include <shader_utils.h>
#include <stdexcept>
#include <fstream>
#include <sstream>

std::string LoadTextFile(const std::string& filename) {
    std::ifstream f(filename.c_str());
    if (f.fail())
        throw std::ios_base::failure("Error opening file: " + filename);
    std::string content, line;
    while (std::getline(f, line))
        content += line + "\n";
    f.close();
    return content;
}

GLuint LoadShaders(const std::string& shaderName) {
    // Compile vertex shader
    std::string vert_src = LoadTextFile(std::string(SHADER_DIRECTORY) + shaderName + ".vert");
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char* cv = vert_src.c_str();
    glShaderSource(vs, 1, &cv, NULL);
    glCompileShader(vs);
    GLint status;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[512]; glGetShaderInfoLog(vs, 512, NULL, buf);
        throw std::ios_base::failure("Vertex shader error (" + shaderName + "): " + buf);
    }

    // Compile fragment shader
    std::string frag_src = LoadTextFile(std::string(SHADER_DIRECTORY) + shaderName + ".frag");
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* cf = frag_src.c_str();
    glShaderSource(fs, 1, &cf, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[512]; glGetShaderInfoLog(fs, 512, NULL, buf);
        throw std::ios_base::failure("Fragment shader error (" + shaderName + "): " + buf);
    }

    // Link program
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[512]; glGetProgramInfoLog(prog, 512, NULL, buf);
        throw std::ios_base::failure("Shader link error (" + shaderName + "): " + buf);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void LoadShaderMatrix(GLuint shader, const glm::mat4& matrix, const std::string& name) {
    GLint loc = glGetUniformLocation(shader, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
}

void LoadShaderVec3(GLuint shader, const glm::vec3& v, const std::string& name) {
    GLint loc = glGetUniformLocation(shader, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(v));
}

void LoadShaderFloat(GLuint shader, float val, const std::string& name) {
    GLint loc = glGetUniformLocation(shader, name.c_str());
    glUniform1f(loc, val);
}

void LoadShaderInt(GLuint shader, int val, const std::string& name) {
    GLint loc = glGetUniformLocation(shader, name.c_str());
    glUniform1i(loc, val);
}
