#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "Angel.h"
#include "../Core/Shader.h"
#include <memory>
#include <vector>

// Forward declarations
class UIElement;
class UIDropdownMenu;

class UIRenderer {
public:
    UIRenderer();
    ~UIRenderer();
    
    bool Init(int screenWidth, int screenHeight);
    void SetScreenSize(int width, int height);
    
    // Render all UI elements
    void BeginUIRender();
    void EndUIRender();
    void RenderUIElement(UIElement* element);
    
    // Add/remove UI elements
    void AddUIElement(std::shared_ptr<UIElement> element);
    void RemoveUIElement(std::shared_ptr<UIElement> element);
    
    // Handle input
    bool HandleMouseClick(int x, int y);
    bool HandleMouseMove(int x, int y);
    
    // Update UI elements (for animations, etc.)
    void Update(float deltaTime);
    
    // Render all managed UI elements
    void RenderAll();
    
private:
    void CreateQuadGeometry();
    void Cleanup();
    
    std::shared_ptr<Shader> m_uiShader;
    std::vector<std::shared_ptr<UIElement>> m_uiElements;
    
    // Quad geometry for rendering rectangles
    GLuint m_quadVAO;
    GLuint m_quadVBO;
    GLuint m_quadEBO;
    
    // Screen dimensions
    int m_screenWidth;
    int m_screenHeight;
    mat4 m_projectionMatrix;
    bool m_initialized;
};

#endif // UI_RENDERER_H
