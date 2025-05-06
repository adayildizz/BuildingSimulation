#include "Core/Window.h"
#include "Core/Shader.h"
#include "Core/ShaderManager.h"
#include "Core/Camera.h"
#include "Grid/TerrainGrid.h"
#include "../include/Angel.h"
#include <iostream>
#include <memory>
#include <cstdlib> // Added for srand
#include <ctime>   // Added for time

// Constants
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int GRID_SIZE = 100; // Size of the grid

// Forward declarations of callback functions
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Grid demo application
class GridDemo
{
public:
    GridDemo() = default;
    ~GridDemo() {}

    void Init()
    {
        CreateWindow();
        InitCallbacks();
        InitCamera();
        InitShader();
        InitGrid();
    }

    void Run()
    {
        while (!window->shouldClose()) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderScene();
            window->pollEvents();
            window->swapBuffers();
        }
    }

    void RenderScene()
    {
        mat4 cameraMatrix = camera->GetViewProjMatrix();
        shader->use();
        shader->setUniform("camMatrix", cameraMatrix);
        shader->setUniform("gMinHeight", 0.0f);
        shader->setUniform("gMaxHeight", 50.0f); // Assuming default maxMountainHeight from SetCrater

        grid->Render();
    }

    void KeyboardCB(int key, int action)
    {
        if (action == GLFW_PRESS) {
            switch (key) {
                case GLFW_KEY_ESCAPE:
                case GLFW_KEY_Q:
                    glfwSetWindowShouldClose(window->getHandle(), GLFW_TRUE);
                    break;

                case GLFW_KEY_B:
                    m_isWireframe = !m_isWireframe;
                    if (m_isWireframe) {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    } else {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    }
                    break;

                case GLFW_KEY_C:
                    camera->Print();
                    break;
            }
        }
        
        camera->OnKeyboard(key);
    }

    void PassiveMouseCB(int x, int y)
    {
        camera->OnMouse(x, y);
    }

    void MouseCB(int button, int action, int x, int y)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                camera->UpdateMousePos(x, y);
                camera->StartRotation();
            } else if (action == GLFW_RELEASE) {
                camera->StopRotation();
            }
        }
    }

    void ResizeCB(int width, int height)
    {
        glViewport(0, 0, width, height);
    }
    

private:
    void CreateWindow()
    {
        window = std::make_unique<Window>(WINDOW_WIDTH, WINDOW_HEIGHT, "Grid Demo");
    }

    void InitCallbacks()
    {
        glfwSetKeyCallback(window->getHandle(), KeyCallback);
        glfwSetCursorPosCallback(window->getHandle(), CursorPosCallback);
        glfwSetMouseButtonCallback(window->getHandle(), MouseButtonCallback);
        glfwSetFramebufferSizeCallback(window->getHandle(), FramebufferSizeCallback);
        
        // Center cursor
        glfwSetCursorPos(window->getHandle(), WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    }

    void InitCamera()
    {
        // Start slightly higher to see the crater mountains
        vec3 cameraPos = vec3(250.0f, 60.0f, 250.0f); 
        // Point slightly downwards initially
        vec3 cameraTarget = vec3(0.0f, -0.2f, 1.0f); 
        vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
        
        float fov = 90.0f;
        float zNear = 0.1f;
        float zFar = 2000.0f;
        PersProjInfo persProjInfo = {fov, WINDOW_WIDTH, WINDOW_HEIGHT, zNear, zFar};

        camera = std::make_unique<Camera>(persProjInfo, cameraPos, cameraTarget, cameraUp);
    }

    void InitShader()
    {
        auto& shaderManager = ShaderManager::getInstance();
        shader = shaderManager.loadShader("basic", 
                                         "shaders/vshader.glsl", 
                                         "shaders/fshader.glsl");
        
        if (!shader) {
            std::cerr << "Failed to load shader" << std::endl;
            exit(-1);
        }
    }

    void InitGrid()
    {
        float worldScale = 5.0f;
        float textureScale = 10.0f;
        
        // Create grid
        grid = std::make_unique<TerrainGrid>();
        grid->Init(GRID_SIZE, GRID_SIZE, worldScale, textureScale, 
                  TerrainGrid::TerrainType::CRATER);        
    }
    
    // Member variables
    std::unique_ptr<Window> window;
    std::unique_ptr<Camera> camera;
    std::shared_ptr<Shader> shader;
    
    // Grid implementation
    std::unique_ptr<TerrainGrid> grid;
    bool m_isWireframe = false;
};

// Global app instance
GridDemo* g_app = nullptr;

// Callback functions
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    g_app->KeyboardCB(key, action);
}

static void CursorPosCallback(GLFWwindow* window, double x, double y)
{
    g_app->PassiveMouseCB((int)x, (int)y);
}

static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    g_app->MouseCB(Button, Action, (int)x, (int)y);
}

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    g_app->ResizeCB(width, height);
}

int main(int argc, char** argv)
{
    // Seed random number generator
    srand(time(0)); 

    g_app = new GridDemo();
    g_app->Init();

    // Set up OpenGL state
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // Sky blue color
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    g_app->Run();

    delete g_app;
    return 0;
}