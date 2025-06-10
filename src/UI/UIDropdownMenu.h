#ifndef UI_DROPDOWN_MENU_H
#define UI_DROPDOWN_MENU_H

#include "UIElement.h"
#include "UIButton.h"
#include <vector>
#include <memory>
#include <functional>

// Forward declaration
class UIRenderer;

class UIDropdownMenu : public UIElement {
public:
    UIDropdownMenu(float x, float y, float width, float itemHeight = 50.0f);
    virtual ~UIDropdownMenu() = default;
    
    // Menu items
    void AddMenuItem(const std::string& text, std::function<void()> callback, const std::string& res_path = "");
    void ClearMenuItems();
    
    // Menu state
    void SetOpen(bool open);
    bool IsOpen() const { return m_isOpen; }
    void ToggleOpen() { SetOpen(!m_isOpen); }
    
    // Animation
    void SetAnimationSpeed(float speed) { m_animationSpeed = speed; }
    void Update(float deltaTime);
    
    // Set UIRenderer reference for managing menu items
    void SetUIRenderer(UIRenderer* renderer) { m_uiRenderer = renderer; }
    
    // Override input handling
    virtual bool OnClick(float x, float y) override;
    virtual bool OnMouseMove(float x, float y) override;
    
    // Rendering
    virtual void Render() override;
    
    // Helper to check if a point is inside the menu area (including items)
    bool IsPointInMenuArea(float x, float y) const;
    
private:
    struct MenuItem {
        std::shared_ptr<UIButton> button;
        float targetY;
        float currentY;
    };
    
    std::vector<MenuItem> m_menuItems;
    float m_itemHeight;
    float m_itemSpacing;
    bool m_isOpen;
    
    // Animation
    float m_animationSpeed;
    float m_animationProgress; // 0.0 = closed, 1.0 = open
    
    // Reference to UIRenderer for managing button visibility
    UIRenderer* m_uiRenderer;
    bool m_buttonsInRenderer; // Track if buttons are currently in the renderer
    
    void UpdateItemPositions();
    void UpdateAnimation(float deltaTime);
    void UpdateItemVisibility();
    float EaseOutCubic(float t) const; // Smooth animation curve
};

#endif // UI_DROPDOWN_MENU_H
