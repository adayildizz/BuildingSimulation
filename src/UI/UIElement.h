#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "Angel.h"
#include <functional>

class UIElement {
public:
    UIElement(float x, float y, float width, float height);
    virtual ~UIElement() = default;
    
    // Position and size
    void SetPosition(float x, float y);
    void SetSize(float width, float height);
    void GetPosition(float& x, float& y) const;
    void GetSize(float& width, float& height) const;
    
    // Color
    void SetColor(const vec3& color);
    void SetColor(float r, float g, float b);
    const vec3& GetColor() const;
    
    // Visibility
    void SetVisible(bool visible);
    bool IsVisible() const;
    
    // Hit testing
    virtual bool IsPointInside(float x, float y) const;
    
    // Input handling
    virtual bool OnClick(float x, float y);
    virtual bool OnMouseMove(float x, float y);
    
    // Rendering
    virtual void Render();
    
    // Transform matrix for rendering
    mat4 GetModelMatrix() const;
    
protected:
    float m_x, m_y;        // Position (screen coordinates)
    float m_width, m_height; // Size
    vec3 m_color;          // Color
    bool m_visible;        // Visibility flag
};

#endif // UI_ELEMENT_H
