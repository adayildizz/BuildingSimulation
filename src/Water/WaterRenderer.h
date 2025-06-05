#pragma once

#include "Angel.h"
#include "Core/Texture.h"
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Water.h"
#include <memory>
#include <functional>

class WaterRenderer {
public:
    WaterRenderer(int width, int height, Shader* water_shader);
    ~WaterRenderer();

    void Init();
    void Draw(const Water& water, const Camera& camera);

    void RenderToFBO(std::shared_ptr<Texture> targetTexture, 
                              std::shared_ptr<Shader> shader, 
                              GLuint vao, 
                              int vertexCount);
    
    // Reflection and Refraction Passes
    void ReflectionPass(Camera& camera, Shader& sceneShader, std::function<void()> renderScene);
    void RefractionPass(Camera& camera, Shader& sceneShader, std::function<void()> renderScene);

    // Getters for textures
    std::shared_ptr<Texture> GetReflectionTexture() const { return m_reflectionTexture; }
    std::shared_ptr<Texture> GetRefractionTexture() const { return m_refractionTexture; }

private:
    GLuint CreateFBO(std::shared_ptr<Texture> texture);
    
    int m_width;
    int m_height;
    GLuint m_reflectionFBO;
    GLuint m_refractionFBO;
    GLuint m_reflectionDepthBuffer;
    GLuint m_refractionDepthBuffer;
    std::shared_ptr<Texture> m_reflectionTexture;
    std::shared_ptr<Texture> m_refractionTexture;
    Shader* m_shader;
};

