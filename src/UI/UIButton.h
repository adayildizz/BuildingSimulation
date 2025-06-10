#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "UIElement.h"
#include "../Core/Texture.h"
#include <functional>
#include <string>
#include <memory>

class UIButton : public UIElement {
public:
    UIButton(float x, float y, float width, float height, const std::string& text = "");
    virtual ~UIButton() = default;
    
    // Text
    void SetText(const std::string& text);
    const std::string& GetText() const;
    
    // Colors
    void SetNormalColor(const vec3& color);
    void SetHoverColor(const vec3& color);
    void SetPressedColor(const vec3& color);
    
    // Texture
    void SetTexture(std::shared_ptr<Texture> texture);
    void SetTexture(const std::string& texturePath);
    bool HasTexture() const;
    std::shared_ptr<Texture> GetTexture() const;
    
    // Callback
    void SetOnClickCallback(std::function<void()> callback);
    
    // Override input handling
    virtual bool OnClick(float x, float y) override;
    virtual bool OnMouseMove(float x, float y) override;
    
    // Rendering
    virtual void Render() override;
    
private:
    std::string m_text;
    vec3 m_normalColor;
    vec3 m_hoverColor;
    vec3 m_pressedColor;
    
    bool m_isHovered;
    bool m_isPressed;
    
    std::shared_ptr<Texture> m_texture;
    
    std::function<void()> m_onClickCallback;
    
    void UpdateColor();
};

#endif // UI_BUTTON_H
