#include "Core/Window.h"
#include "Core/Shader.h"
#include "Core/ShaderManager.h"
#include "Core/Camera.h"
#include "Grid/TerrainGrid.h"
#include "Core/Texture.h"
#include "light.h"
#include "Material.h"
#include "Angel.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

// Constants
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int GRID_SIZE = 250; // Size of the grid

// Forward declarations of callback functions
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Material pointer
std::unique_ptr<Material> m_terrainMaterial;
float m_timeOfDay = 0.0f;           // Current time, progresses from 0 upwards
float m_dayCycleSpeed = 0.05f;    // How fast m_timeOfDay increases (adjust for desired speed)

// Properties for SUNLIGHT (Daytime)
vec3 m_sunDayColor = vec3(1.0f, 1.0f, 0.95f); // Bright, slightly warm white
GLfloat m_sunDayAmbientIntensity = 0.3f;
GLfloat m_sunDayDiffuseIntensity = 0.85f;

// Properties for MOONLIGHT or very dim ambient (Nighttime)
vec3 m_moonNightColor = vec3(0.05f, 0.05f, 0.15f); // Very dim, cool bluish
GLfloat m_moonNightAmbientIntensity = 0.05f;
GLfloat m_moonNightDiffuseIntensity = 0.0f;  // Moonlight often has minimal direct diffuse


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
        InitMaterial();
        InitShader();
        InitGrid();
        InitLight();
    }

    void Run()
    {
        static double lastFrameTime = glfwGetTime(); // Initialize when Run() is first called

        while (!window->shouldClose()) {
            double currentFrameTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);
            lastFrameTime = currentFrameTime;

            // --- UPDATE TIME OF DAY ---
            m_timeOfDay += deltaTime * m_dayCycleSpeed;
            // Optional: Keep m_timeOfDay from growing infinitely (e.g., wrap around every 2*PI for a full cycle)
            // m_timeOfDay = fmod(m_timeOfDay, 2.0f * M_PI); // M_PI from <cmath> or define it

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderScene(); // We'll modify RenderScene next
            window->pollEvents();
            window->swapBuffers();
        }
    }

    void RenderScene()
    {
        // Get the view projection matrix
        mat4 cameraMatrix = camera->GetViewProjMatrix();

        shader->use();
        shader->setUniform("gVP", cameraMatrix);
        shader->setUniform("gMinHeight", m_minTerrainHeight);
        shader->setUniform("gMaxHeight", m_maxTerrainHeight);

        mat4 modelMatrix = mat4(1.0f);
        shader->setUniform("gModelMatrix", modelMatrix);

        // --- Animate Sun Direction ---
        vec3 currentSunDirection;
        float sunAngle = m_timeOfDay; // Angle from 0 towards 2*PI

        // Define sun's path (direction FROM WHICH light comes)
        // Rises in the East (+X), arcs overhead, sets in the West (-X)
        // Y is 0 at horizon, 1 at noon (if angle is 0 to PI for day)
        currentSunDirection.x = cos(sunAngle);
        currentSunDirection.y = sin(sunAngle); // sin goes 0 -> 1 -> 0 -> -1 -> 0
        currentSunDirection.z = 0.2f;          // Slight southerly offset for the arc path
        currentSunDirection = normalize(currentSunDirection);

        // --- Determine Day/Night Light Properties ---
        vec3  activeLightColor;
        GLfloat activeAmbientIntensity;
        GLfloat activeDiffuseIntensity;

        if (currentSunDirection.y > 0.0f) { // Daytime: Sun is above the horizon
            activeLightColor = m_sunDayColor;
            activeAmbientIntensity = m_sunDayAmbientIntensity;
            activeDiffuseIntensity = m_sunDayDiffuseIntensity;
            // Optional: Adjust m_terrainMaterial->specularIntensity if desired for day
        } else { // Nighttime: Sun is below the horizon
            activeLightColor = m_moonNightColor;
            activeAmbientIntensity = m_moonNightAmbientIntensity;
            activeDiffuseIntensity = m_moonNightDiffuseIntensity;
            // Optional: Significantly reduce m_terrainMaterial->specularIntensity for night,
            // or let the very dim moonNightColor handle making specular almost invisible.
            // For simplicity, we'll let the light color control specular brightness for now.
        }

        // --- Update Light Object ---
        if (light) {
            light->SetDirection(currentSunDirection);
            light->SetColor(activeLightColor);
            light->SetAmbientIntensity(activeAmbientIntensity);
            light->SetDiffuseIntensity(activeDiffuseIntensity);
        }

        // --- Set Light Uniforms ---
        if (light && shader->isValid()) {
            GLuint shaderID = shader->getProgramID();
            GLint ambientIntensityLoc = glGetUniformLocation(shaderID, "directionalLight.ambientIntensity");
            GLint ambientColorLoc     = glGetUniformLocation(shaderID, "directionalLight.color");
            GLint diffuseIntensityLoc = glGetUniformLocation(shaderID, "directionalLight.diffuseIntensity");
            GLint directionLoc        = glGetUniformLocation(shaderID, "directionalLight.direction");

            light->UseLight(ambientIntensityLoc, ambientColorLoc, diffuseIntensityLoc, directionLoc);
        }

        // --- Set Material Uniforms (Shininess is likely constant, Specular Intensity might change if you want) ---
        if (m_terrainMaterial && shader->isValid()) {
             GLuint shaderID = shader->getProgramID();
             GLint specularIntensityLoc = glGetUniformLocation(shaderID, "material.specularIntensity");
             GLint shininessLoc = glGetUniformLocation(shaderID, "material.shininess");
             // If you wanted to change material's specular intensity for night:
             // m_terrainMaterial->SetSpecularIntensity(isDay ? daySpecIntensity : nightSpecIntensity);
             m_terrainMaterial->UseMaterial(specularIntensityLoc, shininessLoc);
        }

        // --- Set Camera Position Uniform ---
        if (camera && shader->isValid()) {
            shader->setUniform("gViewPosition_world", camera->GetPosition());
        }

        // Bind terrain textures and set corresponding uniforms (as before)
        for (size_t i = 0; i < m_terrainTextures.size(); ++i) {
            // ... (your existing texture binding code) ...
             if (m_terrainTextures[i] && i < MAX_SHADER_TEXTURE_LAYERS) {
                m_terrainTextures[i]->Bind(GL_TEXTURE0 + i);
                shader->setUniform("gTextureHeight" + std::to_string(i), static_cast<int>(i));
                shader->setUniform("gHeight" + std::to_string(i), m_terrainTextureTransitionHeights[i]);
            }
        }

        // Render terrain grid
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
    
    void InitMaterial()
    {
        m_terrainMaterial = std::make_unique<Material>(0.15f, 24.0f);
    }
    
    void InitLight()
    {
        // Sun-like parameters:
        GLfloat sunColorRed = 1.0f;
        GLfloat sunColorGreen = 1.0f;
        GLfloat sunColorBlue = 0.95f; // Slightly warm white
        GLfloat sunAmbientIntensity = 0.3f; // Keep current, or slightly adjust (e.g., 0.25f)

        // Direction FROM WHICH the sun's rays come
        // Example: High, slightly to the right (+X), and slightly to the front (+Z)
        GLfloat sunDirectionX = 0.4f;
        GLfloat sunDirectionY = 0.8f; // Dominantly from above
        GLfloat sunDirectionZ = 0.3f;

        GLfloat sunDiffuseIntensity = 0.85f; // Strong diffuse component

        light = std::make_unique<Light>(sunColorRed, sunColorGreen, sunColorBlue, sunAmbientIntensity,
                                       sunDirectionX, sunDirectionY, sunDirectionZ, sunDiffuseIntensity);
        std::cout << "Sun-like Directional Light initialized." << std::endl;
    }

    void InitShader()
    {
        auto& shaderManager = ShaderManager::getInstance();
        shader = shaderManager.loadShader("basic",
                                             "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/shaders/vshader.glsl",
                                             "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/shaders/fshader.glsl");

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
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/resources/textures/grass.jpg", // Corresponds to layer defined by layer1_percentage
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/resources/textures/dirt.jpg",  // Corresponds to layer defined by layer2_percentage
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/resources/textures/rock.jpg",  // Corresponds to layer defined by layer3_percentage
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/resources/textures/snow.jpg"   // Corresponds to the final layer up to maxHeight
        };

        m_terrainTextures.clear();
        m_terrainTextureTransitionHeights.clear();

        float heightRange = m_maxTerrainHeight - m_minTerrainHeight;
        // Handle flat terrain case where heightRange might be 0
        if (heightRange <= 1e-5f) { // Use a small epsilon for float comparison
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
    std::unique_ptr<Light> light; // The light object
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
