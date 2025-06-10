#include "UIElement.h"

UIElement::UIElement(float x, float y, float width, float height)
    : m_x(x), m_y(y), m_width(width), m_height(height)
    , m_color(0.7f, 0.7f, 0.7f) // Default gray color
    , m_visible(true)
{
}

void UIElement::SetPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

void UIElement::SetSize(float width, float height) {
    m_width = width;
    m_height = height;
}

void UIElement::GetPosition(float& x, float& y) const {
    x = m_x;
    y = m_y;
}

void UIElement::GetSize(float& width, float& height) const {
    width = m_width;
    height = m_height;
}

void UIElement::SetColor(const vec3& color) {
    m_color = color;
}

void UIElement::SetColor(float r, float g, float b) {
    m_color = vec3(r, g, b);
}

const vec3& UIElement::GetColor() const {
    return m_color;
}

void UIElement::SetVisible(bool visible) {
    m_visible = visible;
}

bool UIElement::IsVisible() const {
    return m_visible;
}

bool UIElement::IsPointInside(float x, float y) const {
    return x >= m_x && x <= m_x + m_width &&
           y >= m_y && y <= m_y + m_height;
}

bool UIElement::OnClick(float x, float y) {
    return IsPointInside(x, y);
}

bool UIElement::OnMouseMove(float x, float y) {
    return IsPointInside(x, y);
}

void UIElement::Render() {
    // Base class rendering is handled by UIRenderer
}

mat4 UIElement::GetModelMatrix() const {
    // Create transformation matrix: translate to position and scale to size
    mat4 translation = Translate(m_x, m_y, 0.0f);
    mat4 scale = Angel::Scale(m_width, m_height, 1.0f);
    return translation * scale;
}
