#pragma once

#include "Shader.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

class ShaderManager {
public:
    // Get singleton instance
    static ShaderManager& getInstance();
    
    // Load shader from files
    std::shared_ptr<Shader> loadShader(const std::string& name, 
                                      const std::string& vertexPath, 
                                      const std::string& fragmentPath);
    
    // Load shader from strings
    std::shared_ptr<Shader> loadShaderFromStrings(const std::string& name,
                                                 const std::string& vertexSource,
                                                 const std::string& fragmentSource);
    
    // Get shader by name
    std::shared_ptr<Shader> getShader(const std::string& name);

    // Set file change callback
    void setFileChangeCallback(std::function<void(const std::string&)> callback);
    
private:
    // Private constructor for singleton
    ShaderManager() {}
    
    // Prevent copying
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    
    // Shader cache
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
    
    // File change callback
    std::function<void(const std::string&)> m_fileChangeCallback;
}; 