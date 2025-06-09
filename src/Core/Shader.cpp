#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

Shader::Shader() : m_programID(0) {
}

Shader::~Shader() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
    }
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    // Read shader source from files
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    
    // Ensure ifstream objects can throw exceptions
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        // Open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        
        std::stringstream vShaderStream, fShaderStream;
        
        // Read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        
        // Close file handlers
        vShaderFile.close();
        fShaderFile.close();
        
        // Convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        return false;
    }
    
    // Compile shaders from strings
    return loadFromStrings(vertexCode, fragmentCode);
}

bool Shader::loadFromStrings(const std::string& vertexSource, const std::string& fragmentSource) {
    // Delete existing program if any
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
        m_programID = 0;
    }
    
    // Compile shaders
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    if (vertexShader == 0) return false;
    
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }
    
    // Create shader program
    m_programID = glCreateProgram();
    glAttachShader(m_programID, vertexShader);
    glAttachShader(m_programID, fragmentShader);
    
    // Link program
    if (!linkProgram()) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        m_programID = 0;
        return false;
    }
    
    // Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

void Shader::use() const {

    if (m_programID != 0) {
        glUseProgram(m_programID);
    }
}

GLuint Shader::compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    // Check for compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool Shader::linkProgram() {
    glLinkProgram(m_programID);
    
    // Check for linking errors
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_programID, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return false;
    }
    
    return true;
}

GLint Shader::getUniformLocation(const std::string& name) const {
    // Check if location is already cached
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }
    
    // Get location and cache it
    GLint location = glGetUniformLocation(m_programID, name.c_str());
    m_uniformLocationCache[name] = location;
    
    return location;
}

void Shader::setUniform(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setUniform(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setUniform(const std::string& name, const vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, value);
}

void Shader::setUniform(const std::string& name, const vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, value);
}

void Shader::setUniform(const std::string& name, const vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, value);
}

void Shader::setUniform(const std::string& name, const mat2& value) const {
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_TRUE, value);
}

void Shader::setUniform(const std::string& name, const mat3& value) const {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_TRUE, value);
}

void Shader::setUniform(const std::string& name, const mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_TRUE, value);
}

void Shader::setUniform(const std::string& name, bool value) const {
    // We get the location and then explicitly convert the C++ bool
    // to a GLint (0 for false, 1 for true) for OpenGL.
    glUniform1i(getUniformLocation(name), static_cast<GLint>(value));
}
