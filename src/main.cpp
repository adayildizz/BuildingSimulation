#include "Core/Window.h"
#include "Core/Shader.h"
#include "Core/ShaderManager.h"
#include "Core/Camera.h"
#include "Grid/TerrainGrid.h"
#include "Core/Texture.h"
#include "Core/light.h"
#include "Core/Material.h"
#include "ObjectLoader/GameObject.h"
#include "ObjectLoader/ObjectLoader.h"
#include "Angel.h"
#include "Core/CelestialLightManager.h"
#include "ObjectLoader/GameObjectManager.h"

#include <iostream>
#include <memory>

#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>

//global mouse pos
double mouseX = 0.0f;
double mouseY = 0.0f;
vec3 intersectionPoint; // Store the last raycast intersection point
float objectPosX = 500.0f;
float ObjectPosY = 10.0f;
float ObjectPosZ = 600.0f;

// Texture painting state
bool isTexturePainting = false;
int currentTextureLayer = 0; // 0: sand, 1: grass, 2: dirt, 3: rock, 4: snow
float brushRadius = 15.0f;
float brushStrength = 2.5f;

// Constants
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int GRID_SIZE = 250; // Size of the grid
ObjectLoader* objectLoader;
GameObject* gameObject; //SelectedGameObject

// Forward declarations of callback functions
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
// Global Pointers
std::unique_ptr<Material> m_terrainMaterial;
std::shared_ptr<Shader> shader;
GameObjectManager* objectManager;

// Grid demo application
class GridDemo
{
public:
    GridDemo() = default;
    ~GridDemo() {
        if (objectLoader) {
            delete objectLoader;
            objectLoader = nullptr;
        }
    }

    void Init()
    {
        
        CreateWindow();
        InitCallbacks();
        InitCamera();
        InitMaterial();
        InitShader();
        InitGrid();
        InitObjects();
        InitLight();
        m_celestialLightManager = std::make_unique<CelestialLightManager>(); // INITIALIZE LIGHT MANAGER
    }

    void Run()
    {
        static double lastFrameTime = glfwGetTime();

        while (!window->shouldClose()) {
            double currentFrameTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);
            lastFrameTime = currentFrameTime;

            // Update the CelestialLightManager
            if (m_celestialLightManager) {
                m_celestialLightManager->Update(deltaTime);
            }
            
            RenderScene();

            window->pollEvents();
            window->swapBuffers();
        }
    }

    void RenderScene()
    {
        // --- Get Sky Color and Configure Light from CelestialLightManager ---
        vec3 currentSkyColor = vec3(0.0f); // Default to black if manager not ready
        if (m_celestialLightManager) {
            currentSkyColor = m_celestialLightManager->GetCurrentSkyColor();
            if (light) { // light is the std::unique_ptr<Light>
                m_celestialLightManager->ConfigureLight(light.get());
            }
        }
        
        glClearColor(currentSkyColor.x, currentSkyColor.y, currentSkyColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Use the unified shader for all rendering
        shader->use();

        // --- Set Global Uniforms (used by both terrain and objects) ---
        mat4 viewProjMatrix = camera->GetViewProjMatrix();
        shader->setUniform("gVP", viewProjMatrix);
        shader->setUniform("gViewPosition_world", camera->GetPosition());

        // Light Uniforms (The 'light' object is now configured by CelestialLightManager)
        if (light && shader->isValid()) {
            GLuint shaderID = shader->getProgramID();
            GLint ambientIntensityLoc = glGetUniformLocation(shaderID, "directionalLight.ambientIntensity");
            GLint ambientColorLoc     = glGetUniformLocation(shaderID, "directionalLight.color");
            GLint diffuseIntensityLoc = glGetUniformLocation(shaderID, "directionalLight.diffuseIntensity");
            GLint directionLoc        = glGetUniformLocation(shaderID, "directionalLight.direction");
            light->UseLight(ambientIntensityLoc, ambientColorLoc, diffuseIntensityLoc, directionLoc); // Use the configured light
        }

        // --- Render Terrain ---
        shader->setUniform("u_isTerrain", true);

        if (m_terrainMaterial && shader->isValid()) {
            GLuint shaderID = shader->getProgramID();
            GLint specularIntensityLoc = glGetUniformLocation(shaderID, "material.specularIntensity");
            GLint shininessLoc = glGetUniformLocation(shaderID, "material.shininess");
            m_terrainMaterial->UseMaterial(specularIntensityLoc, shininessLoc);
        }
        shader->setUniform("gMinHeight", m_minTerrainHeight);
        shader->setUniform("gMaxHeight", m_maxTerrainHeight);
        mat4 terrainModelMatrix = mat4(1.0f);
        shader->setUniform("gModelMatrix", terrainModelMatrix);

        for (size_t i = 0; i < m_terrainTextures.size(); ++i) {
            if (m_terrainTextures[i] && i < MAX_SHADER_TEXTURE_LAYERS) {
                m_terrainTextures[i]->Bind(GL_TEXTURE0 + static_cast<GLenum>(i));
                shader->setUniform("gTextureHeight" + std::to_string(i), static_cast<int>(i));
                shader->setUniform("gHeight" + std::to_string(i), m_terrainTextureTransitionHeights[i]);
            }
        }
        grid->Render();

        // --- Render Objects ---
        shader->setUniform("u_isTerrain", false);
        
        // Use raycasting to position objects on terrain
        if (gameObject && gameObject->isInPlacement) {
            vec3 intersectionPoint;
            if (camera->GetTerrainIntersection(mouseX, mouseY, grid.get(), intersectionPoint)) {
                // Center the object on the cursor by offsetting by half its width and depth
                float halfWidth = gameObject->GetWidth() / 2.0f;
                float halfDepth = gameObject->GetDepth() / 2.0f;
                gameObject->SetPosition(vec4(intersectionPoint.x - halfDepth, 
                                           intersectionPoint.y, 
                                           intersectionPoint.z - halfWidth, 1.0f));
            }
        }
        
        objectManager->RenderAll();
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
                    glPolygonMode(GL_FRONT_AND_BACK, m_isWireframe ? GL_LINE : GL_FILL);
                    break;
                case GLFW_KEY_C:
                    camera->Print();
                    break;
                case GLFW_KEY_P:
                    isTexturePainting = !isTexturePainting;
                    std::cout << "Texture painting mode: " << (isTexturePainting ? "ON" : "OFF") << std::endl;
                    break;
                case GLFW_KEY_1:
                case GLFW_KEY_2:
                case GLFW_KEY_3:
                case GLFW_KEY_4:
                case GLFW_KEY_5:
                    currentTextureLayer = key - GLFW_KEY_1;
                    std::cout << "Selected texture layer: " << currentTextureLayer << std::endl;
                    break;
                case GLFW_KEY_EQUAL: // Increase brush size
                    brushRadius = std::min(brushRadius * 1.2f, 50.0f);
                    std::cout << "Brush radius: " << brushRadius << std::endl;
                    break;
                case GLFW_KEY_MINUS: // Decrease brush size
                    brushRadius = std::max(brushRadius / 1.2f, 1.0f);
                    std::cout << "Brush radius: " << brushRadius << std::endl;
                    break;
                case GLFW_KEY_N:{
                    //TODO:this should be in a thread or a process !!!!!
                    ObjectLoader* obj = new ObjectLoader(*shader);
                    obj->load("Objects/Cat/cat.obj", {0});
                    int index = objectManager->CreateNewObject(*obj);
                    gameObject = objectManager->GetGameObject(index);
                    //gameObject->SetPosition(vec4(600.0f,150.0f,600.0f,1.0f));
                    gameObject->Scale(0.7f);
                    gameObject->RotateX(-90.0f);
                    gameObject->isInPlacement = true; // Put new object in placement mode
                    break;
                };
                case GLFW_KEY_T:{
                    //TODO:this should be in a thread or a process !!!!!
                    ObjectLoader* obj = new ObjectLoader(*shader);
                    obj->load("Objects/Tree/Tree1.obj", {0});
                    int index = objectManager->CreateNewObject(*obj);
                    gameObject = objectManager->GetGameObject(index);
                    //gameObject->SetPosition(vec4(600.0f,150.0f,600.0f,1.0f));
                    gameObject->Scale(10.0f);

                    gameObject->isInPlacement = true; // Put new object in placement mode
                    break;
                };
                case GLFW_KEY_R:
                    gameObject->RotateY(5.0f);
                    break;
                    
            }

            //std::cout << "X pos: " << objectPosX << "Y pos: " << ObjectPosY <<  "ObjectPos Z " << ObjectPosZ << std::endl;
        }
        camera->OnKeyboard(key);
    }



    void PassiveMouseCB(int x, int y)
    {
        mouseX = static_cast<double>(x);
        mouseY = static_cast<double>(y);
        
        // Update texture painting while dragging
        if (isTexturePainting && glfwGetMouseButton(window->getHandle(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            vec3 intersectionPoint;
            if (camera->GetTerrainIntersection(mouseX, mouseY, grid.get(), intersectionPoint)) {
                grid->PaintTexture(intersectionPoint.x, intersectionPoint.z, 
                                currentTextureLayer, brushRadius, brushStrength);
            }
        } else {
            camera->OnMouse(x, y);
        }
    }

    void MouseCB(int button, int action, int x, int y)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                camera->UpdateMousePos(x, y);
                camera->StartRotation();
                
                // Handle texture painting
                if (isTexturePainting) {
                    vec3 intersectionPoint;
                    if (camera->GetTerrainIntersection(mouseX, mouseY, grid.get(), intersectionPoint)) {
                        grid->PaintTexture(intersectionPoint.x, intersectionPoint.z, 
                                        currentTextureLayer, brushRadius, brushStrength);
                    }
                }
                
                // Only finalize object placement if there's an object in placement mode
                if (gameObject && gameObject->isInPlacement) {
                    gameObject->isInPlacement = false;
                }
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
        window = std::make_unique<Window>(WINDOW_WIDTH, WINDOW_HEIGHT, "Unified Shader Demo");
    }

    void InitCallbacks()
    {
        glfwSetKeyCallback(window->getHandle(), KeyCallback);
        glfwSetCursorPosCallback(window->getHandle(), CursorPosCallback);
        glfwSetMouseButtonCallback(window->getHandle(), MouseButtonCallback);
        glfwSetFramebufferSizeCallback(window->getHandle(), FramebufferSizeCallback);
        glfwSetCursorPos(window->getHandle(), WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    }

    void PlaceObject() {
        // Simple object placement - you can modify this to place objects at specific locations
        // For now, just print a message indicating object placement was attempted
        
    }

    void InitObjects(){

        std::cout << "loading objects.." << std::endl;
        objectManager = new GameObjectManager();
        objectLoader = new ObjectLoader(*shader);
        objectLoader->load("Objects/Cottage/cottage_obj.obj", {4});
        int objectIndex = objectManager->CreateNewObject(*objectLoader);
        gameObject = objectManager->GetGameObject(objectIndex);
        //gameObject->SetPosition(vec4(600.0f,0,600.0f,1.0f));
        gameObject->Scale(5.0f);
        gameObject->isInPlacement = true; // Initially placed
    }

    void InitCamera()
    {
        vec3 cameraPos = vec3(625.0f, 150.0f, 625.0f);
        vec3 cameraTarget = vec3(0.0f, -0.3f, 1.0f);
        vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
        float fov = 45.0f;
        float zNear = 0.1f;
        float zFar = 2000.0f;
        PersProjInfo persProjInfo = {fov, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT), zNear, zFar};

        camera = std::make_unique<Camera>(persProjInfo, cameraPos, cameraTarget, cameraUp);
    }


    void InitMaterial()
    {
        m_terrainMaterial = std::make_unique<Material>(0.15f, 24.0f);
    }

    void InitLight() // This method now just creates the Light object. Its properties are set by CelestialLightManager.
    {
        // Initial values for the Light object. These will be quickly overwritten by CelestialLightManager.
        GLfloat initialColorRed = 1.0f;
        GLfloat initialColorGreen = 1.0f;
        GLfloat initialColorBlue = 1.0f;
        GLfloat initialAmbientIntensity = 0.2f;
        GLfloat initialDirectionX = 0.0f;
        GLfloat initialDirectionY = 1.0f; // Pointing up initially
        GLfloat initialDirectionZ = 0.0f;
        GLfloat initialDiffuseIntensity = 0.5f;

        light = std::make_unique<Light>(initialColorRed, initialColorGreen, initialColorBlue, initialAmbientIntensity,
                                        initialDirectionX, initialDirectionY, initialDirectionZ, initialDiffuseIntensity);
        std::cout << "Directional Light object created." << std::endl;
    }

    void InitShader()
    {
        auto& shaderManager = ShaderManager::getInstance();
        shader = shaderManager.loadShader("unifiedShader",
                                        "shaders/vshader.glsl",
                                        "shaders/fshader.glsl");
        if (!shader) {
            std::cerr << "Failed to load unified shader (shaders/vshader.glsl, shaders/fshader.glsl)" << std::endl;
            exit(-1);
        }
        std::cout << "Unified shader loaded successfully." << std::endl;
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

        m_minTerrainHeight = grid->GetMinHeight();
        m_maxTerrainHeight = grid->GetMaxHeight();

        const auto& layerPercentages = grid->GetLayerInfo();
        std::vector<std::string> texturePaths = {
            "resources/textures/sand.jpg",
            "resources/textures/grass.jpg",
            "resources/textures/dirt.jpg",
            "resources/textures/rock.jpg",
            "resources/textures/snow.jpg"
        };

        m_terrainTextures.clear();
        m_terrainTextureTransitionHeights.clear();

        float heightRange = m_maxTerrainHeight - m_minTerrainHeight;
        if (heightRange <= 1e-5f) {
            heightRange = 1.0f;
        }

        // Update transition heights for 5 textures
        float transitionHeight1 = m_minTerrainHeight + heightRange * 0.20f; // sand -> grass
        float transitionHeight2 = m_minTerrainHeight + heightRange * 0.40f; // grass -> dirt  
        float transitionHeight3 = m_minTerrainHeight + heightRange * 0.60f; // dirt -> rock
        float transitionHeight4 = m_minTerrainHeight + heightRange * 0.80f; // rock -> snow
        float transitionHeight5 = m_maxTerrainHeight; // final snow

        std::vector<float> calculatedTransitions = {
            transitionHeight1, transitionHeight2, transitionHeight3, transitionHeight4, transitionHeight5
        };

        for (size_t i = 0; i < texturePaths.size(); ++i) {
            if (i >= MAX_SHADER_TEXTURE_LAYERS) break;
            auto tex = std::make_shared<Texture>(GL_TEXTURE_2D, texturePaths[i]);
            if (tex->Load()) {
                m_terrainTextures.push_back(tex);
                m_terrainTextureTransitionHeights.push_back(calculatedTransitions[i]);
                std::cout << "Loaded texture " << texturePaths[i] << " with transition height " << calculatedTransitions[i] << std::endl;
            } else {
                std::cerr << "Failed to load terrain texture: " << texturePaths[i] << std::endl;
            }
        }

        if (m_terrainTextures.empty()) {
            std::cerr << "CRITICAL: No terrain textures were loaded!" << std::endl;
        }
    }

    // Member variables
    std::unique_ptr<Window> window;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<TerrainGrid> grid;
    std::unique_ptr<Light> light; // The actual light object used by shaders
    std::unique_ptr<CelestialLightManager> m_celestialLightManager; // <<< ADD LIGHT MANAGER MEMBER
    bool m_isWireframe = false;
    float m_minTerrainHeight = 0.0f;
    float m_maxTerrainHeight = 1.0f;

    std::vector<std::shared_ptr<Texture>> m_terrainTextures;
    std::vector<float> m_terrainTextureTransitionHeights;
    static const int MAX_SHADER_TEXTURE_LAYERS = 5;
};

GridDemo* g_app = nullptr;

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (g_app) g_app->KeyboardCB(key, action);
}
static void CursorPosCallback(GLFWwindow* window, double x, double y) {
    if (g_app) g_app->PassiveMouseCB(static_cast<int>(x), static_cast<int>(y));
}
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (g_app) g_app->MouseCB(Button, Action, static_cast<int>(x), static_cast<int>(y));
}
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (g_app) g_app->ResizeCB(width, height);
}



int main(int argc, char** argv)
{
    g_app = new GridDemo();
    g_app->Init();

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    g_app->Run();

    delete g_app;
    g_app = nullptr;
    return 0;
}
