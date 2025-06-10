#include "UIButton.h"
#include <iostream>

UIButton::UIButton(float x, float y, float width, float height, const std::string& text)
    : UIElement(x, y, width, height)
    , m_text(text)
    , m_normalColor(0.4f, 0.4f, 0.4f)   // Dark gray
    , m_hoverColor(0.6f, 0.6f, 0.6f)    // Light gray
    , m_pressedColor(0.2f, 0.2f, 0.2f)  // Very dark gray
    , m_isHovered(false)
    , m_isPressed(false)
{
    SetColor(m_normalColor);
}

void UIButton::SetText(const std::string& text) {
    m_text = text;
}

const std::string& UIButton::GetText() const {
    return m_text;
}

void UIButton::SetNormalColor(const vec3& color) {
    m_normalColor = color;
    UpdateColor();
}

void UIButton::SetHoverColor(const vec3& color) {
    m_hoverColor = color;
    UpdateColor();
}

void UIButton::SetPressedColor(const vec3& color) {
    m_pressedColor = color;
    UpdateColor();
}

void UIButton::SetTexture(std::shared_ptr<Texture> texture) {
    m_texture = texture;
}

void UIButton::SetTexture(const std::string& texturePath) {
    m_texture = std::make_shared<Texture>(GL_TEXTURE_2D, texturePath);
    if (!m_texture->Load()) {
        std::cerr << "Failed to load button texture: " << texturePath << std::endl;
        m_texture = nullptr;
    }
}

bool UIButton::HasTexture() const {
    return m_texture != nullptr;
}

std::shared_ptr<Texture> UIButton::GetTexture() const {
    return m_texture;
}

void UIButton::SetOnClickCallback(std::function<void()> callback) {
    m_onClickCallback = callback;
}

bool UIButton::OnClick(float x, float y) {
    if (!IsVisible() || !IsPointInside(x, y)) {
        return false;
    }
    
    m_isPressed = true;
    UpdateColor();
    
    // Call the callback if set
    if (m_onClickCallback) {
        m_onClickCallback();
    }
    
    std::cout << "Button '" << m_text << "' clicked!" << std::endl;
    
    // Reset pressed state after a brief moment (in a real game you'd handle this with mouse release)
    m_isPressed = false;
    UpdateColor();
    
    return true; // Event consumed
}

bool UIButton::OnMouseMove(float x, float y) {
    bool wasHovered = m_isHovered;
    m_isHovered = IsPointInside(x, y);
    
    if (wasHovered != m_isHovered) {
        UpdateColor();
    }
    
    return m_isHovered;
}

void UIButton::Render() {
    // Base class handles the actual rendering
    UIElement::Render();
}

void UIButton::UpdateColor() {
    if (m_isPressed) {
        SetColor(m_pressedColor);
    } else if (m_isHovered) {
        SetColor(m_hoverColor);
    } else {
        SetColor(m_normalColor);
    }
}
