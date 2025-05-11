#include "Core/Window.h"
#include "Core/Shader.h"
#include "Core/ShaderManager.h"
#include "Core/Camera.h"
#include "Grid/TerrainGrid.h"
#include "Core/Texture.h"
#include "../include/Angel.h"
#include <iostream>
#include <memory>
#include <vector> // For std::vector to hold texture names and heights temporarily
#include <string> // For std::string
#include <cstdlib> // Added for srand
#include <ctime>   // Added for time

// Constants
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int GRID_SIZE = 250; // Size of the grid

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
        // Get the view projection matrix
        mat4 cameraMatrix = camera->GetViewProjMatrix(); 

        // Use the shader
        shader->use();
        shader->setUniform("gVP", cameraMatrix);
        shader->setUniform("gMinHeight", m_minTerrainHeight);
        shader->setUniform("gMaxHeight", m_maxTerrainHeight);

        for (size_t i = 0; i < m_terrainTextures.size(); ++i) {
            if (m_terrainTextures[i] && i < MAX_SHADER_TEXTURE_LAYERS) {
                m_terrainTextures[i]->Bind(GL_TEXTURE0 + i);
                shader->setUniform("gTextureHeight" + std::to_string(i), static_cast<int>(i));
                shader->setUniform("gHeight" + std::to_string(i), m_terrainTextureTransitionHeights[i]);
            }
        }

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
        vec3 cameraPos = vec3(625.0f, 150.0f, 625.0f); // Increased Y for better view of crater
        // Point slightly downwards initially
        vec3 cameraTarget = vec3(0.0f, -0.3f, 1.0f); // Adjusted target for new camera height
        vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
        
        float fov = 45.0f;
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
        
        grid = std::make_unique<TerrainGrid>();
        TerrainGrid::TerrainType terrainType = TerrainGrid::TerrainType::VOLCANIC_CALDERA;
        float maxEdgeHeightForGenerator = 120.0f; 
        float centralFlatRatioForGenerator = 0.25f;

        grid->Init(GRID_SIZE, GRID_SIZE, worldScale, textureScale, 
                  terrainType, maxEdgeHeightForGenerator, centralFlatRatioForGenerator);

        // Update min/max terrain heights from the grid itself
        m_minTerrainHeight = grid->GetMinHeight();
        m_maxTerrainHeight = grid->GetMaxHeight();
                  
        const auto& layerPercentages = grid->GetLayerInfo();

        // Define texture paths - these could be mapped based on terrainType in a more advanced system
        std::vector<std::string> texturePaths = {
            "resources/textures/grass.jpg", // Corresponds to layer defined by layer1_percentage
            "resources/textures/dirt.jpg",  // Corresponds to layer defined by layer2_percentage
            "resources/textures/rock.jpg",  // Corresponds to layer defined by layer3_percentage
            "resources/textures/snow.jpg"   // Corresponds to the final layer up to maxHeight
        };

        m_terrainTextures.clear();
        m_terrainTextureTransitionHeights.clear();

        float heightRange = m_maxTerrainHeight - m_minTerrainHeight;
        // Handle flat terrain case where heightRange might be 0
        if (heightRange <= 1e-5f) { // Use a small epsilon for float comparison
             // For flat terrain, transitions might not make sense or might be set to 0 or max.
             // Based on layerPercentages, if they are 0 for flat, all transitions will be m_minTerrainHeight.
             // If we want distinct layers even on "flat" terrain (e.g. different soil types based on tiny variations not in heightmap)
             // this logic would need adjustment. For now, proceed with calculation.
             // If min=max, all transitions will be equal to minHeight (or maxHeight).
             // Shader should gracefully handle this (e.g. use first texture).
             if (heightRange == 0.0f) heightRange = 1.0f; // Avoid division by zero if percentages are non-zero
        }


        // Calculate absolute transition heights
        // These are the ENDING heights for each layer/texture
        float transitionHeight1 = m_minTerrainHeight + heightRange * layerPercentages.layer1_percentage;
        float transitionHeight2 = m_minTerrainHeight + heightRange * layerPercentages.layer2_percentage;
        float transitionHeight3 = m_minTerrainHeight + heightRange * layerPercentages.layer3_percentage;
        float transitionHeight4 = m_maxTerrainHeight; // The last layer's "transition" is the max height

        std::vector<float> calculatedTransitions = {
            transitionHeight1, 
            transitionHeight2, 
            transitionHeight3, 
            transitionHeight4
        };
        
        // Load textures and store them with their transition heights
        for (size_t i = 0; i < texturePaths.size(); ++i) {
            if (i >= MAX_SHADER_TEXTURE_LAYERS) { // Ensure we don't exceed shader's capacity
                std::cout << "Warning: Exceeded max shader texture layers. Only loading " << MAX_SHADER_TEXTURE_LAYERS << std::endl;
                break;
            }
            auto tex = std::make_shared<Texture>(GL_TEXTURE_2D, texturePaths[i]);
            if (tex->Load()) {
                m_terrainTextures.push_back(tex);
                m_terrainTextureTransitionHeights.push_back(calculatedTransitions[i]);
                std::cout << "Loaded texture " << texturePaths[i] << " with transition height " << calculatedTransitions[i] << std::endl;
            } else {
                std::cerr << "Failed to load terrain texture: " << texturePaths[i] << std::endl;
                // Optionally, push a placeholder or skip to keep vectors aligned if shader expects all slots
            }
        }

        std::cout << "Terrain textures setup for GPU blending. Count: " << m_terrainTextures.size() << std::endl;
        if (m_terrainTextures.empty()) {
             std::cerr << "CRITICAL: No terrain textures were loaded!" << std::endl;
        }
    }
    
    // Member variables
    std::unique_ptr<Window> window;
    std::unique_ptr<Camera> camera;
    std::shared_ptr<Shader> shader;
    std::unique_ptr<TerrainGrid> grid;
    bool m_isWireframe = false;
    float m_minTerrainHeight = 0.0f;
    float m_maxTerrainHeight = 1.0f;
    
    std::vector<std::shared_ptr<Texture>> m_terrainTextures;
    std::vector<float> m_terrainTextureTransitionHeights;
    static const int MAX_SHADER_TEXTURE_LAYERS = 4; // Max layers shader supports
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