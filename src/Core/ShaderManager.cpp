#include "ShaderManager.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>

ShaderManager& ShaderManager::getInstance() {
    static ShaderManager instance;
    return instance;
}

std::shared_ptr<Shader> ShaderManager::loadShader(const std::string& name, 
                                                 const std::string& vertexPath, 
                                                 const std::string& fragmentPath) {
    // Check if shader already exists
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        std::cerr << "Shader '" << name << "' already exists. Use a different name or get the existing shader." << std::endl;
        return it->second;
    }
    
    // Create new shader
    auto shader = std::make_shared<Shader>();
    
    // Load shader from files
    if (!shader->loadFromFiles(vertexPath, fragmentPath)) {
        std::cerr << "Failed to load shader '" << name << "' from files." << std::endl;
        return nullptr;
    }
    
    // Add to cache
    m_shaders[name] = shader;
    
    return shader;
}

std::shared_ptr<Shader> ShaderManager::loadShaderFromStrings(const std::string& name,
                                                            const std::string& vertexSource,
                                                            const std::string& fragmentSource) {
    // Check if shader already exists
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        std::cerr << "Shader '" << name << "' already exists. Use a different name or get the existing shader." << std::endl;
        return it->second;
    }
    
    // Create new shader
    auto shader = std::make_shared<Shader>();
    
    // Load shader from strings
    if (!shader->loadFromStrings(vertexSource, fragmentSource)) {
        std::cerr << "Failed to load shader '" << name << "' from strings." << std::endl;
        return nullptr;
    }
    
    // Add to cache
    m_shaders[name] = shader;
    
    return shader;
}

std::shared_ptr<Shader> ShaderManager::getShader(const std::string& name) {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second;
    }
    
    std::cerr << "Shader '" << name << "' not found." << std::endl;
    return nullptr;
}

void ShaderManager::setFileChangeCallback(std::function<void(const std::string&)> callback) {
    m_fileChangeCallback = callback;
} 