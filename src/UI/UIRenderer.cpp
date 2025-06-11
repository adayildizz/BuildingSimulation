#include "UIRenderer.h"
#include "UIElement.h"
#include "UIButton.h"
#include "../Core/ShaderManager.h"
#include <iostream>

UIRenderer::UIRenderer() 
    : m_quadVAO(0), m_quadVBO(0), m_quadEBO(0)
    , m_screenWidth(1920), m_screenHeight(1080)
    , m_initialized(false)
{
}

UIRenderer::~UIRenderer() {
    Cleanup();
}

bool UIRenderer::Init(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    
    // Load UI shader
    auto& shaderManager = ShaderManager::getInstance();
    m_uiShader = shaderManager.loadShader("uiShader",
                                          "shaders/ui_vertex.glsl",
                                          "shaders/ui_fragment.glsl");
    
    if (!m_uiShader) {
        std::cerr << "Failed to load UI shader" << std::endl;
        return false;
    }
    
    // Create orthographic projection matrix
    m_projectionMatrix = Ortho(0.0f, static_cast<float>(m_screenWidth),
                              0.0f, static_cast<float>(m_screenHeight),
                              -1.0f, 1.0f);
    
    // Create quad geometry
    CreateQuadGeometry();
    
    m_initialized = true;
    std::cout << "UI Renderer initialized successfully" << std::endl;
    return true;
}

void UIRenderer::SetScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
    
    // Update projection matrix
    m_projectionMatrix = Ortho(0.0f, static_cast<float>(m_screenWidth),
                              0.0f, static_cast<float>(m_screenHeight),
                              -1.0f, 1.0f);
}

void UIRenderer::BeginUIRender() {
    if (!m_initialized) return;
    
    // Disable depth testing for UI
    glDisable(GL_DEPTH_TEST);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use UI shader
    m_uiShader->use();
    m_uiShader->setUniform("u_projection", m_projectionMatrix);
}

void UIRenderer::EndUIRender() {
    if (!m_initialized) return;
    
    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Disable blending
    glDisable(GL_BLEND);
}

void UIRenderer::RenderUIElement(UIElement* element) {
    if (!element || !element->IsVisible()) return;
    
    // Set uniforms
    m_uiShader->setUniform("u_model", element->GetModelMatrix());
    m_uiShader->setUniform("u_color", element->GetColor());
    
    // Check if element is a button with texture
    UIButton* button = dynamic_cast<UIButton*>(element);
    if (button && button->HasTexture()) {
        m_uiShader->setUniform("u_hasTexture", true);
        button->GetTexture()->Bind(GL_TEXTURE10);  // Use texture unit 6 to avoid conflicts
        m_uiShader->setUniform("u_texture", 10);
    } else {
        m_uiShader->setUniform("u_hasTexture", false);
    }
    
    // Render quad
    glBindVertexArray(m_quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Clean up texture state if we used a texture
    if (button && button->HasTexture()) {
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void UIRenderer::AddUIElement(std::shared_ptr<UIElement> element) {
    m_uiElements.push_back(element);
}

void UIRenderer::RemoveUIElement(std::shared_ptr<UIElement> element) {
    auto it = std::find(m_uiElements.begin(), m_uiElements.end(), element);
    if (it != m_uiElements.end()) {
        m_uiElements.erase(it);
    }
}

bool UIRenderer::HandleMouseClick(int x, int y) {
    // Use screen coordinates directly (no Y flip needed)
    // UI elements are already positioned in screen space (Y=0 at top)
    
    // Check all UI elements from front to back
    for (auto it = m_uiElements.rbegin(); it != m_uiElements.rend(); ++it) {
        if ((*it)->IsVisible() && (*it)->OnClick(static_cast<float>(x), static_cast<float>(y))) {
            return true; // Event consumed
        }
    }
    return false; // Event not handled
}

bool UIRenderer::HandleMouseMove(int x, int y) {
    // Convert screen coordinates to UI coordinates (flip Y)
    float uiY = m_screenHeight - y;
    
    bool handled = false;
    
    // Handle regular UI elements
    for (auto& element : m_uiElements) {
        if (element->IsVisible() && element->OnMouseMove(static_cast<float>(x), uiY)) {
            handled = true;
        }
    }
    return handled;
}

void UIRenderer::Update(float deltaTime) {
    // Update dropdowns are now handled by the main application
    // since dropdown items are managed as regular UI elements
}

void UIRenderer::RenderAll() {
    if (!m_initialized) return;
    
    BeginUIRender();
    
    // Render all UI elements (including dropdown items)
    for (auto& element : m_uiElements) {
        RenderUIElement(element.get());
    }
    
    EndUIRender();
}

void UIRenderer::CreateQuadGeometry() {
    // Quad vertices (position only, normalized device coordinates)
    float vertices[] = {
        // Position
        0.0f, 0.0f,  // Bottom-left
        1.0f, 0.0f,  // Bottom-right
        1.0f, 1.0f,  // Top-right
        0.0f, 1.0f   // Top-left
    };
    
    unsigned int indices[] = {
        0, 1, 2,   // First triangle
        2, 3, 0    // Second triangle
    };
    
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_quadEBO);
    
    glBindVertexArray(m_quadVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void UIRenderer::Cleanup() {
    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    if (m_quadEBO) glDeleteBuffers(1, &m_quadEBO);
    
    m_quadVAO = m_quadVBO = m_quadEBO = 0;
    m_initialized = false;
}
