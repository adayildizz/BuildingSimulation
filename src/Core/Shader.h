#pragma once

#include "Angel.h"
#include <string>
#include <unordered_map>
#include <memory>

class Shader {
public:
    // Constructor and destructor
    Shader();
    ~Shader();
    
    // Load and compile shaders from files
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    // Load and compile shaders from strings
    bool loadFromStrings(const std::string& vertexSource, const std::string& fragmentSource);

    // Use the shader program
    void use() const;
    
    // Set uniform values
    void setUniform(const std::string& name, bool value) const;
    void setUniform(const std::string& name, int value) const;
    void setUniform(const std::string& name, float value) const;
    void setUniform(const std::string& name, const vec2& value) const;
    void setUniform(const std::string& name, const vec3& value) const;
    void setUniform(const std::string& name, const vec4& value) const;
    void setUniform(const std::string& name, const mat2& value) const;
    void setUniform(const std::string& name, const mat3& value) const;
    void setUniform(const std::string& name, const mat4& value) const;
    
    // Get program ID
    GLuint getProgramID() const { return m_programID; }
    
    // Check if shader is valid
    bool isValid() const { return m_programID != 0; }
    
private:
    // Helper methods
    GLuint compileShader(const std::string& source, GLenum type);
    bool linkProgram();
    GLint getUniformLocation(const std::string& name) const;
    
    // Shader program ID
    GLuint m_programID;
    
    // Cache for uniform locations
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;
}; 
