#pragma once

#include "Angel.h"
#include <string>
#include <functional>

class Window {
public:
    // Constructor and destructor
    Window(int width, int height, const std::string& title);
    ~Window();

    // Window management
    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    
    // Getters
    GLFWwindow* getHandle() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    // Callback setters
    void setKeyCallback(std::function<void(int, int, int, int)> callback);
    void setMouseButtonCallback(std::function<void(int, int, int)> callback);
    void setCursorPosCallback(std::function<void(double, double)> callback);
    void setScrollCallback(std::function<void(double, double)> callback);
    void setFramebufferSizeCallback(std::function<void(int, int)> callback);

private:
    // Window properties
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    
    // Callback functions
    std::function<void(int, int, int, int)> m_keyCallback;
    std::function<void(int, int, int)> m_mouseButtonCallback;
    std::function<void(double, double)> m_cursorPosCallback;
    std::function<void(double, double)> m_scrollCallback;
    std::function<void(int, int)> m_framebufferSizeCallback;
    
    // Static callback handlers
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    
    // Error handling
    static void errorCallback(int error, const char* description);
}; 
