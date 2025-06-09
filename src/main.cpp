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
#include "Core/ShadowMap.h"

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

// Constants
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int GRID_SIZE = 250; // Size of the grid
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096; // Shadow map resolution

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
std::shared_ptr<Shader> m_shadowShader; // Pointer for the shadow shader
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
        InitShader(); // This will now load both shaders
        InitGrid();
        InitObjects();
        InitLight();
        m_celestialLightManager = std::make_unique<CelestialLightManager>(); // INITIALIZE LIGHT MANAGER
        
        // Initialize the Shadow Map
        m_shadowMap = std::make_unique<ShadowMap>();
        if (!m_shadowMap->Init(SHADOW_WIDTH, SHADOW_HEIGHT)) {
            std::cerr << "Shadow Map initialization failed!" << std::endl;
            // Handle error appropriately
        }
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

    // Method for the first rendering pass (depth pass)
    void RenderSceneForShadowMap(const mat4& lightSpaceMatrix)
    {
        m_shadowShader->use();
        m_shadowShader->setUniform("gLightSpaceMatrix", lightSpaceMatrix);
        
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        m_shadowMap->Write(); // Bind the shadow FBO
        
        glClear(GL_DEPTH_BUFFER_BIT);
         
        // --- Render Terrain for Shadow Map ---
        mat4 terrainModelMatrix = mat4(1.0f);
        m_shadowShader->setUniform("gModelMatrix", terrainModelMatrix);
        grid->Render();

        // --- Render Objects for Shadow Map ---
        objectManager->RenderAll(*m_shadowShader); // We need to modify RenderAll to accept a shader
    }

    
    
    void RenderScene()
    {
        // --- Calculate Light Space Matrix ---
        mat4 lightSpaceMatrix;
        if (m_celestialLightManager) {
            vec3 lightDir = m_celestialLightManager->GetActiveLightDirection();
            
            // Create projection and view matrices from the light's perspective
            float near_plane = 1.0f, far_plane = 2000.0f;
            mat4 lightProjection = Ortho(-800.0f, 800.0f, -800.0f, 800.0f, near_plane, far_plane);
            
            // Position the light "behind" the scene center, looking at it
            vec3 lightPos = vec3(625.0f, 500.0f, 625.0f) - lightDir * 600.0f;
            mat4 lightView = LookAt(lightPos, vec3(625.0f, 0.0, 625.0f), vec3(0.0, 1.0, 0.0));
            
            lightSpaceMatrix = lightProjection * lightView;
        }   
 
        // --- PASS 1 - Render scene to depth map ---
        glCullFace(GL_FRONT); // Fix for peter-panning shadow artifact
        RenderSceneForShadowMap(lightSpaceMatrix);
        glCullFace(GL_BACK); // Reset culling

        // --- PASS 2 - Render scene normally with shadows ---
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to default framebuffer

        glViewport(0, 0, window->getWidth(), window->getHeight());


        // Get Sky Color and Clear Buffers
        vec3 currentSkyColor = vec3(0.0f);
        if (m_celestialLightManager) {
            currentSkyColor = m_celestialLightManager->GetCurrentSkyColor();
            if (light) {
                m_celestialLightManager->ConfigureLight(light.get());
            }
        } 
        glClearColor(currentSkyColor.x, currentSkyColor.y, currentSkyColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
        // Use the unified shader for all rendering
        shader->use();
          
        if (m_celestialLightManager) {
            // Check if the sun is at its peak.
            bool isSunAtZenith = m_celestialLightManager->IsSunAtZenith();
            // Enable shadows unless the sun is at its zenith.
            
 
            shader->setUniform("u_shadowsEnabled", !isSunAtZenith);
            
            if (isSunAtZenith) {
                std::cout << "SUN IS AT ZENITH - DISABLING SHADOWS" << std::endl;
            }
        }

        // Set Global Uniforms
        mat4 viewProjMatrix = camera->GetViewProjMatrix();
        shader->setUniform("gVP", viewProjMatrix);
        shader->setUniform("gViewPosition_world", camera->GetPosition());
        shader->setUniform("gLightSpaceMatrix", lightSpaceMatrix); // Pass light matrix to main shader

        // Bind shadow map texture to an available texture unit (e.g., 5)
        m_shadowMap->Read(GL_TEXTURE5);
        shader->setUniform("shadowMap", 5); // Tell shader which unit has the shadow map
        
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

        objectManager->RenderAll(*shader);
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
                case GLFW_KEY_N:{
                    //TODO:this should be in a thread or a process !!!!!
                    ObjectLoader* obj = new ObjectLoader(*shader);
                    obj->load("/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/Objects/Cat/cat.obj", {0});
                    int index = objectManager->CreateNewObject(*obj);
                    gameObject = objectManager->GetGameObject(index);
                    gameObject->Scale(0.7f);
                    gameObject->RotateX(-90.0f);
                    gameObject->isInPlacement = true; // Put new object in placement mode
                    break;
                };
                case GLFW_KEY_T:{
                    //TODO:this should be in a thread or a process !!!!!
                    ObjectLoader* obj = new ObjectLoader(*shader);
                    obj->load("/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/Objects/Tree/Tree1.obj", {0});
                    int index = objectManager->CreateNewObject(*obj);
                    gameObject = objectManager->GetGameObject(index);
                    gameObject->Scale(10.0f);
                    gameObject->isInPlacement = true; // Put new object in placement mode
                    break;
                };
                case GLFW_KEY_R:
                    gameObject->RotateY(5.0f);
                    break;
            }
        }
        camera->OnKeyboard(key);
    }

    void PassiveMouseCB(int x, int y)
    {
        mouseX = static_cast<double>(x);
        mouseY = static_cast<double>(y);
        camera->OnMouse(x, y);
    }

    void MouseCB(int button, int action, int x, int y)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                camera->UpdateMousePos(x, y);
                camera->StartRotation();
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
        objectLoader->load("/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/Objects/Cottage/cottage_obj.obj");
        int objectIndex = objectManager->CreateNewObject(*objectLoader);
        gameObject = objectManager->GetGameObject(objectIndex);
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
        float zFar = 3000.0f;
        PersProjInfo persProjInfo = {fov, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT), zNear, zFar};

        camera = std::make_unique<Camera>(persProjInfo, cameraPos, cameraTarget, cameraUp);
    }


    void InitMaterial()
    {
        m_terrainMaterial = std::make_unique<Material>(0.15f, 24.0f);
    }

    void InitLight() // This method now just creates the Light object. Its properties are set by CelestialLightManager.
    {
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
        
        // Load the main shader
        shader = shaderManager.loadShader("unifiedShader",
                                          "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/shaders/vshader.glsl",
                                          "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/shaders/fshader.glsl");
        if (!shader) {
            std::cerr << "Failed to load unified shader (shaders/vshader.glsl, shaders/fshader.glsl)" << std::endl;
            exit(-1);
        }
        std::cout << "Unified shader loaded successfully." << std::endl;

        // Load the shadow shader
        m_shadowShader = shaderManager.loadShader("shadowShader",
                                                   "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/shaders/shadow_vshader.glsl",
                                                   "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/shaders/shadow_fshader.glsl");
        if (!m_shadowShader) {
            std::cerr << "Failed to load shadow shader" << std::endl;
            exit(-1);
        }
        std::cout << "Shadow shader loaded successfully." << std::endl;
    }
        
    void InitGrid()
    {
        float worldScale = 5.0f;
        float textureScale = 10.0f;

        grid = std::make_unique<TerrainGrid>();
        TerrainGrid::TerrainType terrainType = TerrainGrid::TerrainType::FLAT;
        float maxEdgeHeightForGenerator = 120.0f;
        float centralFlatRatioForGenerator = 0.25f;

        grid->Init(GRID_SIZE, GRID_SIZE, worldScale, textureScale,
                    terrainType, maxEdgeHeightForGenerator, centralFlatRatioForGenerator);

        m_minTerrainHeight = grid->GetMinHeight();
        m_maxTerrainHeight = grid->GetMaxHeight();

        const auto& layerPercentages = grid->GetLayerInfo();
        std::vector<std::string> texturePaths = {
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/resources/textures/grass.jpg",
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/resources/textures/dirt.jpg",
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/resources/textures/rock.jpg",
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo2/TerrainDemo2/resources/textures/snow.jpg"
        };

        m_terrainTextures.clear();
        m_terrainTextureTransitionHeights.clear();

        float heightRange = m_maxTerrainHeight - m_minTerrainHeight;
        if (heightRange <= 1e-5f) {
            heightRange = 1.0f;
        }

        float transitionHeight1 = m_minTerrainHeight + heightRange * layerPercentages.layer1_percentage;
        float transitionHeight2 = m_minTerrainHeight + heightRange * layerPercentages.layer2_percentage;
        float transitionHeight3 = m_minTerrainHeight + heightRange * layerPercentages.layer3_percentage;
        float transitionHeight4 = m_maxTerrainHeight;

        std::vector<float> calculatedTransitions = {
            transitionHeight1, transitionHeight2, transitionHeight3, transitionHeight4
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
    std::unique_ptr<CelestialLightManager> m_celestialLightManager;
    std::unique_ptr<ShadowMap> m_shadowMap; // Shadow map member
    bool m_isWireframe = false;
    float m_minTerrainHeight = 0.0f;
    float m_maxTerrainHeight = 1.0f;

    std::vector<std::shared_ptr<Texture>> m_terrainTextures;
    std::vector<float> m_terrainTextureTransitionHeights;
    static const int MAX_SHADER_TEXTURE_LAYERS = 4;
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
