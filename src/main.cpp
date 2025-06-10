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
#include "UI/UIRenderer.h"
#include "UI/UIButton.h"
#include "UI/UIDropdownMenu.h"

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
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096; // Shadow map resolution

ObjectLoader* objectLoader;
std::vector<ObjectLoader*> objectLoaders;
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

//CONSTANTS
std::vector<std::pair<std::string, std::vector<unsigned int>>> objectPaths = {
    {"../Objects/Cottage/cottage_obj.obj", {}},
    {"../Objects/Tree/Tree1.obj", {0}},
    {"../Objects/Cat/cat.obj", {}}
};


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
        InitUI(); // Initialize UI system
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
            
            // Update UI system (for animations)
            if (m_uiRenderer) {
                m_uiRenderer->Update(deltaTime);
            }
            
            // Update dropdown menu animations
            if (m_objectMenu) {
                m_objectMenu->Update(deltaTime);
            }

            if (m_objectMenu2) {
                m_objectMenu2->Update(deltaTime);
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

            // The dynamic ortho projection logic from the previous fix is still needed.
            float minOrthoSize = 450.0f;
            float maxOrthoSize = 900.0f;
            float sunElevation = std::clamp(lightDir.y, 0.0f, 1.0f);
            // C++ equivalent of mix()
            float currentOrthoSize = maxOrthoSize * (1.0f - sunElevation) + minOrthoSize * sunElevation;

            mat4 lightProjection = Ortho(-currentOrthoSize, currentOrthoSize,
                                         -currentOrthoSize, currentOrthoSize,
                                         1.0f, 2000.0f);
            
            // --- CORRECTED VIEW MATRIX LOGIC ---
            // 1. Define the point the light should look at.
            vec3 sceneCenter = vec3(625.0f, 0.0f, 625.0f);

            // 2. Position the light's camera far away from the center along the light's direction.
            //    We use '+' here to move the camera "behind" the light.
            vec3 lightPos = sceneCenter + lightDir * 1000.0f; // The distance (1000.0f) should be large enough to be outside the scene.

            // 3. Create the LookAt matrix. The camera is at lightPos, looking at sceneCenter.
            mat4 lightView = LookAt(lightPos, sceneCenter, vec3(0.0, 1.0, 0.0));
            
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
        shader->setUniform("u_shadowsEnabled", true);


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
        
        // --- Render UI ---
        if (m_uiRenderer) {
            m_uiRenderer->RenderAll();
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
        // Get current window size
        int currentWidth, currentHeight;
        glfwGetWindowSize(window->getHandle(), &currentWidth, &currentHeight);
        
        // Store raw coordinates but scale them to match camera's expected dimensions
        mouseX = (static_cast<double>(x) * WINDOW_WIDTH) / currentWidth;
        mouseY = (static_cast<double>(y) * WINDOW_HEIGHT) / currentHeight;
        
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

         // Handle UI mouse move
        if (m_uiRenderer) {
            m_uiRenderer->HandleMouseMove(x, y);
        }
    }

    void MouseCB(int button, int action, int x, int y)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                // Get current window size and scale coordinates
                int currentWidth, currentHeight;
                glfwGetWindowSize(window->getHandle(), &currentWidth, &currentHeight);
                
                // Scale coordinates to match camera's expected dimensions
                mouseX = (static_cast<double>(x) * WINDOW_WIDTH) / currentWidth;
                mouseY = (static_cast<double>(y) * WINDOW_HEIGHT) / currentHeight;
                
                // Check UI first
                if (m_uiRenderer && m_uiRenderer->HandleMouseClick(x, y)) {
                    return; // UI handled the click, don't process 3D interaction
                }
                    
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

    void InitObjects(){
        std::cout << "loading objects.." << std::endl;
        objectManager = new GameObjectManager();
        for(int i = 0; i<3;i++){
            
        }
        for(int i = 0; i<3;i++){
            ObjectLoader* objectLoader = new ObjectLoader(*shader);
            objectLoader->load(objectPaths[i].first, objectPaths[i].second); 
            objectLoaders.push_back(objectLoader);
        }
        
       
        /**int objectIndex = objectManager->CreateNewObject(*objectLoader);
        gameObject = objectManager->GetGameObject(objectIndex);
        gameObject->Scale(5.0f);
        gameObject->isInPlacement = true; // Initially placed
        */
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
                                          "shaders/vshader.glsl",
                                          "shaders/fshader.glsl");
        if (!shader) {
            std::cerr << "Failed to load unified shader (shaders/vshader.glsl, shaders/fshader.glsl)" << std::endl;
            exit(-1);
        }
        std::cout << "Unified shader loaded successfully." << std::endl;

        // Load the shadow shader
        m_shadowShader = shaderManager.loadShader("shadowShader",
                                                   "shaders/shadow_vshader.glsl",
                                                   "shaders/shadow_fshader.glsl");
        if (!m_shadowShader) {
            std::cerr << "Failed to load shadow shader" << std::endl;
            exit(-1);
        }
        std::cout << "Shadow shader loaded successfully." << std::endl;
    }
    
    void InitUI()
    {
        m_uiRenderer = std::make_unique<UIRenderer>();
        if (!m_uiRenderer->Init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
            std::cerr << "Failed to initialize UI renderer" << std::endl;
            return;
        }
        
        // Create main menu buttons in the left corner
        auto menuButton = std::make_shared<UIButton>(20.0f, WINDOW_HEIGHT - 130.0f, 120.0f, 120.0f, "Objects");
        menuButton->SetNormalColor(vec3(1.0f, 1.0f, 1.0f));  // White to show texture clearly
        menuButton->SetHoverColor(vec3(1.2f, 1.2f, 1.2f));   // Slightly brighter on hover
        menuButton->SetPressedColor(vec3(0.8f, 0.8f, 0.8f)); // Darker when pressed
        menuButton->SetTexture("resources/icons/dice.png");  // Add icon texture

        auto menuButton2 = std::make_shared<UIButton>(150.0f, WINDOW_HEIGHT - 130.0f, 120.0f, 120.0f, "Terrain");
        menuButton2->SetNormalColor(vec3(1.0f, 1.0f, 1.0f));  // White to show texture clearly
        menuButton2->SetHoverColor(vec3(1.2f, 1.2f, 1.2f));   // Slightly brighter on hover
        menuButton2->SetPressedColor(vec3(0.8f, 0.8f, 0.8f)); // Darker when pressed
        menuButton2->SetTexture("resources/icons/brush.png"); // Add grass texture
        
        // Create dropdown menus that appears from the top
        m_objectMenu2 = std::make_shared<UIDropdownMenu>(150.0f, WINDOW_HEIGHT - 130.0f, 120.0f, 120.0f);
        m_objectMenu2 -> SetUIRenderer(m_uiRenderer.get());
        m_objectMenu = std::make_shared<UIDropdownMenu>(20.0f, WINDOW_HEIGHT - 130.0f, 120.0f, 120.0f);
        m_objectMenu->SetUIRenderer(m_uiRenderer.get());

            
        m_objectMenu2->AddMenuItem("Rock", [this]() {
            std::cout << "Painting Rock terrain..." << std::endl;
            // You can add terrain modification logic here later
            isTexturePainting = true;
            currentTextureLayer = 3;

        }, "resources/icons/rock.jpg");
        
        m_objectMenu2->AddMenuItem("Grass", [this]() {
            std::cout << "Painting Grass terrain..." << std::endl;
            // You can add terrain modification logic here later
            isTexturePainting = true;
            currentTextureLayer = 2;
        },"resources/icons/grass1.jpg");
        
        m_objectMenu2->AddMenuItem("Dirt", [this]() {
            std::cout << "Painting Dirt terrain..." << std::endl;
            // You can add terrain modification logic here later
            isTexturePainting = true;
            currentTextureLayer = 1;
        },"resources/icons/dirt.jpg");
        // Add menu items for different objects
        m_objectMenu->AddMenuItem("Cat", [this]() {
            std::cout << "Loading Cat..." << std::endl;
            //ObjectLoader* obj = new ObjectLoader(*shader);
            //obj->load("../Objects/Cat/cat.obj", {0});
            int index = objectManager->CreateNewObject(*objectLoaders[2]);
            GameObject* newGameObject = objectManager->GetGameObject(index);
            if (newGameObject) {
                newGameObject->Scale(0.7f);
                newGameObject->RotateX(-90.0f);
                newGameObject->isInPlacement = true;
                gameObject = newGameObject;
            }
            isTexturePainting = false;
        }, "resources/icons/cat.png");
        
        m_objectMenu->AddMenuItem("Tree", [this]() {
            std::cout << "Loading Tree..." << std::endl;
            //ObjectLoader* obj = new ObjectLoader(*shader);
            //obj->load("../Objects/Tree/Tree1.obj", {0});
            int index = objectManager->CreateNewObject(*objectLoaders[1]);
            GameObject* newGameObject = objectManager->GetGameObject(index);
            if (newGameObject) {
                newGameObject->Scale(10.0f);
                newGameObject->isInPlacement = true;
                gameObject = newGameObject;
            }
            isTexturePainting = false;
        }, "resources/icons/tree.png");
        
        m_objectMenu->AddMenuItem("Cottage", [this]() {
            std::cout << "Loading Cottage..." << std::endl;
            //ObjectLoader* obj = new ObjectLoader(*shader);
            //obj->load("../Objects/Cottage/cottage_obj.obj");
            int index = objectManager->CreateNewObject(*objectLoaders[0]);
            GameObject* newGameObject = objectManager->GetGameObject(index);
            if (newGameObject) {
                newGameObject->Scale(5.0f);
                newGameObject->isInPlacement = true;
                gameObject = newGameObject;
            }
            isTexturePainting = false;
        },"resources/icons/cottage.png");
        
        // Set button callback to toggle menu
        menuButton->SetOnClickCallback([this]() {
            std::cout << "Menu button clicked! Toggling dropdown..." << std::endl;
            m_objectMenu->ToggleOpen();
            std::cout << "Dropdown is now: " << (m_objectMenu->IsOpen() ? "OPEN" : "CLOSED") << std::endl;
        });

        menuButton2->SetOnClickCallback([this]() {
            std::cout << "Menu button 2 clicked! Toggling dropdown..." << std::endl;
            m_objectMenu2->ToggleOpen();
            std::cout << "Dropdown 2 is now: " << (m_objectMenu2->IsOpen() ? "OPEN" : "CLOSED") << std::endl;
            if(!m_objectMenu2->IsOpen()) isTexturePainting = false;

        });
        
        m_uiRenderer->AddUIElement(menuButton);
        m_uiRenderer->AddUIElement(menuButton2);
        
        std::cout << "UI system initialized with dropdown menu" << std::endl;
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
    std::unique_ptr<CelestialLightManager> m_celestialLightManager;
    std::unique_ptr<ShadowMap> m_shadowMap; // Shadow map member
    bool m_isWireframe = false;
    float m_minTerrainHeight = 0.0f;
    float m_maxTerrainHeight = 1.0f;

    std::vector<std::shared_ptr<Texture>> m_terrainTextures;
    std::vector<float> m_terrainTextureTransitionHeights;
    std::unique_ptr<UIRenderer> m_uiRenderer;
    std::shared_ptr<UIDropdownMenu> m_objectMenu, m_objectMenu2;
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
