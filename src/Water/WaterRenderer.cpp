#include "WaterRenderer.h"
#include <iostream>
#include <functional>
#include "Core/Camera.h"

WaterRenderer::WaterRenderer(int width, int height, Shader* water_shader) 
    : m_width(width), m_height(height), m_reflectionFBO(0), m_refractionFBO(0),
      m_reflectionDepthBuffer(0), m_refractionDepthBuffer(0), m_shader(water_shader) {
    std::cout << "WaterRenderer::WaterRenderer - Initializing water renderer..." << std::endl;
}

WaterRenderer::~WaterRenderer() {
    if (m_reflectionFBO) glDeleteFramebuffers(1, &m_reflectionFBO);
    if (m_refractionFBO) glDeleteFramebuffers(1, &m_refractionFBO);
    if (m_reflectionDepthBuffer) glDeleteRenderbuffers(1, &m_reflectionDepthBuffer);
    if (m_refractionDepthBuffer) glDeleteRenderbuffers(1, &m_refractionDepthBuffer);
}

void WaterRenderer::Init() {
    // Create reflection texture and FBO
    m_reflectionTexture = std::make_shared<Texture>(GL_TEXTURE_2D, "reflection");
    m_reflectionTexture->LoadRawData(m_width, m_height, 3, nullptr);
    m_reflectionFBO = CreateFBO(m_reflectionTexture);

    // Create refraction texture and FBO
    m_refractionTexture = std::make_shared<Texture>(GL_TEXTURE_2D, "refraction");
    m_refractionTexture->LoadRawData(m_width, m_height, 3, nullptr);
    m_refractionFBO = CreateFBO(m_refractionTexture);
}

GLuint WaterRenderer::CreateFBO(std::shared_ptr<Texture> texture) {
    GLuint fbo;
    GLuint drb;

    // Generate framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Bind texture to FBO
    texture->Bind(GL_TEXTURE0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->GetTextureObject(), 0);

    // Generate depth renderbuffer
    glGenRenderbuffers(1, &drb);
    glBindRenderbuffer(GL_RENDERBUFFER, drb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, drb);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;
}

void WaterRenderer::RenderToFBO(std::shared_ptr<Texture> targetTexture, 
                              std::shared_ptr<Shader> shader, 
                              GLuint vao, 
                              int vertexCount) {
    glBindFramebuffer(GL_FRAMEBUFFER, targetTexture == m_reflectionTexture ? m_reflectionFBO : m_refractionFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader->use();
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WaterRenderer::ReflectionPass(Camera& camera, Shader& sceneShader, std::function<void()> renderScene) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_reflectionFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Store original camera values
    vec3 originalPos = camera.GetPos();
    vec3 originalTarget = camera.GetTarget();
    
    // Apply reflection transformation
    vec3 reflectedPos = vec3(originalPos.x, -originalPos.y, originalPos.z);
    vec3 reflectedTarget = vec3(originalTarget.x, -originalTarget.y, originalTarget.z);

    // Update camera for reflection
    camera.SetPosition(reflectedPos);
    camera.SetTarget(reflectedTarget);  // This will call InitInternal() internally

    // Set camera matrices as uniforms
    sceneShader.use();
    sceneShader.setUniform("ModelView", camera.GetViewMatrix());
    sceneShader.setUniform("Projection", camera.GetProjMatrix());

    // Render the scene from the reflected camera
    renderScene();

    // Restore original camera values
    camera.SetPosition(originalPos);
    camera.SetTarget(originalTarget);  // This will call InitInternal() internally

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WaterRenderer::RefractionPass(Camera& camera, Shader& sceneShader, std::function<void()> renderScene) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_refractionFBO);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set camera matrices as uniforms
    sceneShader.use();
    sceneShader.setUniform("ModelView", camera.GetViewMatrix());
    sceneShader.setUniform("Projection", camera.GetProjMatrix());

    // Render the scene from the refracted camera
    renderScene();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WaterRenderer::Draw(const Water& water, const Camera& camera) {
    std::cout << "WaterRenderer::Draw - Starting water render..." << std::endl;
    
    if (!m_shader || !m_shader->isValid()) {
        std::cerr << "Error: Invalid water shader program" << std::endl;
        return;
    }

    m_shader->use();
    std::cout << "WaterRenderer::Draw - Shader bound successfully" << std::endl;

    // Set uniforms
    m_shader->setUniform("u_ViewProj", camera.GetViewProjMatrix());
    m_shader->setUniform("u_CameraPos", camera.GetPosition());
    m_shader->setUniform("u_Time", (float)glfwGetTime());
    
    std::cout << "WaterRenderer::Draw - Uniforms set:" << std::endl;
    std::cout << "  Camera Position: " << camera.GetPosition() << std::endl;

    // Draw water with reflection and refraction textures
    water.draw(*m_reflectionTexture, *m_refractionTexture);
    std::cout << "WaterRenderer::Draw - Water render complete" << std::endl;

    glUseProgram(0);
}
