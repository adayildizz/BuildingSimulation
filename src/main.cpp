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

//shadow properties
GLuint m_shadowMapFBO;
GLuint m_shadowMapTexture;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; // Or other desired resolution
std::shared_ptr<Shader> m_depthShader; // For the depth pass
mat4 m_lightSpaceMatrix; // To store the light's combined view-projection matrix

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
        
        if (m_shadowMapFBO != 0) {
            glDeleteFramebuffers(1, &m_shadowMapFBO);
        }
        if (m_shadowMapTexture != 0) {
            glDeleteTextures(1, &m_shadowMapTexture);
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
        InitShadows();
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
        // --- Calculate Light Space Matrix ---
        vec3 lightDir = vec3(0.0f, -1.0f, 0.0f); // Default direction

        if (m_celestialLightManager) {
            // CelestialLightManager's Update() should have been called in the main loop (GridDemo::Run)
            // to update its internal time and properties.
            // GetActiveLightDirection() will return the latest calculated direction.
            vec3 activeDir = m_celestialLightManager->GetActiveLightDirection(); //
            if (length(activeDir) > 0.001f) { // Ensure direction is not zero
                lightDir = normalize(activeDir);
            }
            // Configure the main 'light' object. This updates its internal state.
            // The light->UseLight() call later will use these updated values.
            if(light) {
                m_celestialLightManager->ConfigureLight(light.get()); //
            }
        }
        // If m_celestialLightManager is null, lightDir will remain the default,
        // and the 'light' object will use its last configured or initial state.

        // Define the extents of the light's view volume for orthographic projection
        float near_plane_light = 1.0f;
        float far_plane_light = 2500.0f;
        float orthoSize = 800.0f; // Adjust to fit your scene elements

        mat4 lightProjection = Angel::Ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane_light, far_plane_light);

        vec3 sceneFocusPoint = vec3((GRID_SIZE * grid->GetWorldScale()) / 2.0f, 0.0f, (GRID_SIZE * grid->GetWorldScale()) / 2.0f);
        vec3 lightActualPos = sceneFocusPoint - lightDir * 1000.0f; // Position light source based on direction

        mat4 lightView = Angel::LookAt(lightActualPos, sceneFocusPoint, vec3(0.0, 1.0, 0.0));
        m_lightSpaceMatrix = lightProjection * lightView;


        // ====== PASS 1: Render Scene to Depth Map (from light's perspective) ======
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        if (m_depthShader && m_depthShader->isValid()) {
            m_depthShader->use();
            m_depthShader->setUniform("lightSpaceMatrix", m_lightSpaceMatrix);

            glCullFace(GL_FRONT); // Mitigate Peter Panning
            if (objectManager) {
                objectManager->RenderAllForDepthPass(*m_depthShader);
            }
            glCullFace(GL_BACK); // Reset culling
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ====== PASS 2: Render Scene Normally (from camera's perspective) ======
        int currentWindowWidth, currentWindowHeight;
        glfwGetFramebufferSize(window->getHandle(), &currentWindowWidth, &currentWindowHeight);
        glViewport(0, 0, currentWindowWidth, currentWindowHeight);
        
        vec3 currentSkyColor = vec3(0.0f);
        if (m_celestialLightManager) {
            currentSkyColor = m_celestialLightManager->GetCurrentSkyColor(); //
            // The light object is already configured above by m_celestialLightManager->ConfigureLight(light.get());
        }
        
        glClearColor(currentSkyColor.x, currentSkyColor.y, currentSkyColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!shader || !shader->isValid()) {
            std::cerr << "Main shader is not valid!" << std::endl;
            return;
        }
        shader->use();

        mat4 viewProjMatrix = camera->GetViewProjMatrix(); //
        shader->setUniform("gVP", viewProjMatrix);
        shader->setUniform("gViewPosition_world", camera->GetPosition()); //
        shader->setUniform("lightSpaceMatrix", m_lightSpaceMatrix);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);
        shader->setUniform("shadowMap", 5);
        shader->setUniform("u_shadowBias", 0.005f);

        if (light) {
            GLuint shaderID = shader->getProgramID();
            GLint ambientIntensityLoc = glGetUniformLocation(shaderID, "directionalLight.ambientIntensity");
            GLint ambientColorLoc     = glGetUniformLocation(shaderID, "directionalLight.color");
            GLint diffuseIntensityLoc = glGetUniformLocation(shaderID, "directionalLight.diffuseIntensity");
            GLint directionLoc        = glGetUniformLocation(shaderID, "directionalLight.direction");
            light->UseLight(ambientIntensityLoc, ambientColorLoc, diffuseIntensityLoc, directionLoc); //
        }

        shader->setUniform("u_isTerrain", true);
        if (m_terrainMaterial) {
            GLuint shaderID = shader->getProgramID();
            GLint specularIntensityLoc = glGetUniformLocation(shaderID, "material.specularIntensity");
            GLint shininessLoc = glGetUniformLocation(shaderID, "material.shininess");
            m_terrainMaterial->UseMaterial(specularIntensityLoc, shininessLoc); //
        }
        shader->setUniform("gMinHeight", m_minTerrainHeight);
        shader->setUniform("gMaxHeight", m_maxTerrainHeight);
        mat4 terrainModelMatrix = mat4(1.0f);
        shader->setUniform("gModelMatrix", terrainModelMatrix);

        for (size_t i = 0; i < m_terrainTextures.size(); ++i) {
            if (m_terrainTextures[i] && i < MAX_SHADER_TEXTURE_LAYERS) {
                GLenum texUnitEnum = GL_TEXTURE0 + static_cast<GLenum>(i);
                m_terrainTextures[i]->Bind(texUnitEnum); //
                shader->setUniform("gTextureHeight" + std::to_string(i), static_cast<int>(i));
                shader->setUniform("gHeight" + std::to_string(i), m_terrainTextureTransitionHeights[i]);
            }
        }
        if (grid) {
            grid->Render();
        }

        shader->setUniform("u_isTerrain", false);
        
        if (gameObject && gameObject->isInPlacement) {
            vec3 intersectionPointTerrain;
            // The GetTerrainIntersection method is part of your Camera class's interface,
            // its implementation might use ScreenToWorldRay and other logic.
            if (camera->GetTerrainIntersection(mouseX, mouseY, grid.get(), intersectionPointTerrain)) {
                float halfWidth = gameObject->GetWidth() / 2.0f;
                float halfDepth = gameObject->GetDepth() / 2.0f;
                gameObject->SetPosition(vec4(intersectionPointTerrain.x - halfDepth,
                                           intersectionPointTerrain.y,
                                           intersectionPointTerrain.z - halfWidth, 1.0f));
            }
        }
        
        if (objectManager) {
            objectManager->RenderAll();
        }
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
                    obj->load("/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/Objects/Cat/cat.obj", {0});
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
                    obj->load("/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/Objects/Tree/Tree1.obj", {0});
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
        objectLoader->load("/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/Objects/Cottage/cottage_obj.obj", {4});
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
    
    void InitShadows()
    {
        // 1. Load the depth shader
        auto& shaderManager = ShaderManager::getInstance();
        m_depthShader = shaderManager.loadShader("depthShader",
                                                 "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/shaders/depth_vshader.glsl",
                                                 "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/shaders/depth_fshader.glsl");
        if (!m_depthShader) {
            std::cerr << "Failed to load depth shader!" << std::endl;
            // Handle error
        } 

        // 2. Create the depth texture
        glGenTextures(1, &m_shadowMapTexture);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Or GL_LINEAR for softer shadows with PCF
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // Or GL_LINEAR
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // Avoid sampling outside texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Areas outside shadow map are lit
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // 3. Create and configure the FBO
        glGenFramebuffers(1, &m_shadowMapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTexture, 0);
        glDrawBuffer(GL_NONE); // We don't need to draw color data
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer not complete!" << std::endl;
            // Handle error
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind
    }

    void InitShader()
    {
        auto& shaderManager = ShaderManager::getInstance();
        shader = shaderManager.loadShader("unifiedShader",
                                          "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/shaders/vshader.glsl",
                                          "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/shaders/fshader.glsl");
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
        TerrainGrid::TerrainType terrainType = TerrainGrid::TerrainType::FLAT;
        float maxEdgeHeightForGenerator = 120.0f;
        float centralFlatRatioForGenerator = 0.25f;

        grid->Init(GRID_SIZE, GRID_SIZE, worldScale, textureScale,
                    terrainType, maxEdgeHeightForGenerator, centralFlatRatioForGenerator);

        m_minTerrainHeight = grid->GetMinHeight();
        m_maxTerrainHeight = grid->GetMaxHeight();

        const auto& layerPercentages = grid->GetLayerInfo();
        std::vector<std::string> texturePaths = {
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/resources/textures/grass.jpg",
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/resources/textures/dirt.jpg",
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/resources/textures/rock.jpg",
            "/Users/ahmetnecc/Desktop/MY COMP410 PROJECTS/TerrainDemo/TerrainDemo/resources/textures/snow.jpg"
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
    std::unique_ptr<CelestialLightManager> m_celestialLightManager; // <<< ADD LIGHT MANAGER MEMBER
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
