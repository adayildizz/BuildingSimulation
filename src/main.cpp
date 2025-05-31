#include "Core/Window.h"
#include "Core/Shader.h"
#include "Core/ShaderManager.h"
#include "Core/Camera.h"
#include "Grid/TerrainGrid.h"
#include "Core/Texture.h"
#include "light.h"
#include "Material.h"
#include "ObjectLoader/ObjectLoader.h"
#include "Angel.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
 

//global mouse pos
double mouseX = 0.0f;
double mouseY = 0.0f;
float objectPosX = 500.0f;
float ObjectPosY = 10.0f;
float ObjectPosZ = 600.0f;
// GLuint program, Projection; // REMOVE: No longer needed as global for separate object shader

// Constants
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int GRID_SIZE = 250; // Size of the grid
ObjectLoader* objectLoader;

// Forward declarations of callback functions
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Material pointer
std::unique_ptr<Material> m_terrainMaterial;
// You might want a separate material for objects if their properties differ significantly
// std::unique_ptr<Material> m_objectMaterial;

float m_timeOfDay = 0.0f;           // Current time, progresses from 0 upwards
float m_dayCycleSpeed = 0.3f;    // How fast m_timeOfDay increases (adjust for desired speed)

vec3 m_daySkyColor = vec3(0.529f, 0.808f, 0.922f); // Your current nice sky blue
vec3 m_nightSkyColor = vec3(0.0f, 0.0f, 0.0f);   // Pitch black for night

// Properties for SUNLIGHT (Daytime)
vec3 m_sunDayColor = vec3(1.0f, 1.0f, 0.95f); // Bright, slightly warm white
GLfloat m_sunDayAmbientIntensity = 0.3f;
GLfloat m_sunDayDiffuseIntensity = 0.85f;

// Properties for MOONLIGHT or very dim ambient (Nighttime)
vec3 m_moonNightColor = vec3(0.4f, 0.45f, 0.6f); // A brighter, but still cool, dim light
GLfloat m_moonNightAmbientIntensity = 0.6f;      // Increased ambient for general visibility
GLfloat m_moonNightDiffuseIntensity = 0.05f;     // Small amount of diffuse for soft moonlight directionality


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
        InitShader();   // Must be after InitMaterial if material needs shader IDs early, but usually not
        InitGrid();
        InitObjects();  // Must be after InitShader so objectLoader gets the correct shader program ID
        InitLight();
    }

    void Run()
    {
        static double lastFrameTime = glfwGetTime();

        while (!window->shouldClose()) {
            double currentFrameTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);
            lastFrameTime = currentFrameTime;

            m_timeOfDay += deltaTime * m_dayCycleSpeed;
            m_timeOfDay = fmod(m_timeOfDay, 2.0f * M_PI);

            RenderScene();

            window->pollEvents();
            window->swapBuffers();
        }
    }

    void RenderScene()
    {
        // --- Calculate Sun's Position for Sky and Light ---
        vec3 calculatedCelestialDirection;
        float celestialAngleRadians = m_timeOfDay;

        calculatedCelestialDirection.x = cos(celestialAngleRadians);
        calculatedCelestialDirection.y = sin(celestialAngleRadians);
        calculatedCelestialDirection.z = 0.2f;

        float dayFactor = fmax(0.0f, calculatedCelestialDirection.y);
        vec3 currentSkyColor = m_nightSkyColor * (1.0f - dayFactor) + m_daySkyColor * dayFactor;

        glClearColor(currentSkyColor.x, currentSkyColor.y, currentSkyColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        vec3 normalizedCelestialDirection = normalize(calculatedCelestialDirection);
        vec3 actualLightDirectionForShader;
        vec3 activeLightColor;
        GLfloat activeAmbientIntensity;
        GLfloat activeDiffuseIntensity;

        if (normalizedCelestialDirection.y > 0.01f) {
            activeLightColor = m_sunDayColor;
            activeAmbientIntensity = m_sunDayAmbientIntensity;
            activeDiffuseIntensity = m_sunDayDiffuseIntensity;
            actualLightDirectionForShader = normalizedCelestialDirection;
        } else {
            activeLightColor = m_moonNightColor;
            activeAmbientIntensity = m_moonNightAmbientIntensity;
            activeDiffuseIntensity = m_moonNightDiffuseIntensity;
            actualLightDirectionForShader.x = normalizedCelestialDirection.x;
            actualLightDirectionForShader.y = abs(normalizedCelestialDirection.y);
            if (actualLightDirectionForShader.y < 0.1f) {
                actualLightDirectionForShader.y = 0.1f;
            }
            actualLightDirectionForShader.z = normalizedCelestialDirection.z;
            actualLightDirectionForShader = normalize(actualLightDirectionForShader);
        }

        if (light) {
            light->SetDirection(actualLightDirectionForShader);
            light->SetColor(activeLightColor);
            light->SetAmbientIntensity(activeAmbientIntensity);
            light->SetDiffuseIntensity(activeDiffuseIntensity);
        }

        // Use the unified shader for all rendering
        shader->use();

        // --- Set Global Uniforms (used by both terrain and objects) ---
        mat4 viewProjMatrix = camera->GetViewProjMatrix();
        shader->setUniform("gVP", viewProjMatrix);
        shader->setUniform("gViewPosition_world", camera->GetPosition());

        // Light Uniforms
        if (light && shader->isValid()) {
            GLuint shaderID = shader->getProgramID();
            GLint ambientIntensityLoc = glGetUniformLocation(shaderID, "directionalLight.ambientIntensity");
            GLint ambientColorLoc     = glGetUniformLocation(shaderID, "directionalLight.color");
            GLint diffuseIntensityLoc = glGetUniformLocation(shaderID, "directionalLight.diffuseIntensity");
            GLint directionLoc        = glGetUniformLocation(shaderID, "directionalLight.direction");
            light->UseLight(ambientIntensityLoc, ambientColorLoc, diffuseIntensityLoc, directionLoc);
        }

        // --- Render Terrain ---
        shader->setUniform("u_isTerrain", true); // Tell shader it's rendering terrain

        // Terrain Material Uniforms
        if (m_terrainMaterial && shader->isValid()) {
             GLuint shaderID = shader->getProgramID();
             GLint specularIntensityLoc = glGetUniformLocation(shaderID, "material.specularIntensity");
             GLint shininessLoc = glGetUniformLocation(shaderID, "material.shininess");
             m_terrainMaterial->UseMaterial(specularIntensityLoc, shininessLoc);
        }
        // Set terrain-specific uniforms
        shader->setUniform("gMinHeight", m_minTerrainHeight);
        shader->setUniform("gMaxHeight", m_maxTerrainHeight);
        mat4 terrainModelMatrix = mat4(1.0f); // Terrain is at origin, not transformed
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
        shader->setUniform("u_isTerrain", false); // Tell shader it's rendering an object

        // Object Material Uniforms (if different from terrain)
        // For now, objects will use the same material properties as terrain.
        // If you had m_objectMaterial, you would call:
        // m_objectMaterial->UseMaterial(specularIntensityLoc, shininessLoc);
        // Or set uniforms directly:
        // shader->setUniform("material.specularIntensity", 0.5f); // Example
        // shader->setUniform("material.shininess", 32.0f);      // Example


        vec3 fixedObjectWorldPos = vec3(objectPosX, ObjectPosY, ObjectPosZ);
        float scale = 100.0f;
        float normX = (mouseX / WINDOW_WIDTH) * 2.0f - 1.0f;
        float normY = 1.0f - (mouseY / WINDOW_HEIGHT) * 2.0f;
        normX *= scale;
        normY *= scale;

        mat4 translation_from_cursor = Translate(fixedObjectWorldPos.x + normX, fixedObjectWorldPos.y, fixedObjectWorldPos.z + normY);
        mat4 objectScaleMatrix = Scale(5.0f,5.0f, 5.0f);
        mat4 objectModelMatrix = translation_from_cursor * objectScaleMatrix;

        shader->setUniform("gModelMatrix", objectModelMatrix); // Set model matrix for the object

        if (objectLoader) {
            // ObjectLoader::render should:
            // 1. Bind the object's VAO.
            // 2. Bind the object's diffuse texture to GL_TEXTURE0.
            //    (because when u_isTerrain is false, fshader.glsl uses gTextureHeight0 for the object's texture)
            // 3. Call glDrawElements/Arrays.
            // It should NOT set its own shaders or matrix uniforms if they conflict with gVP/gModelMatrix.
            // The 'mvpMatrix' argument might be ignored by ObjectLoader or set a uniform not used by vshader.glsl.
            mat4 viewMatrix = camera->GetViewMatrix(); // Only needed if objectLoader computes MVP internally
            mat4 mvpMatrix = viewProjMatrix * objectModelMatrix; // This is the actual MVP for the object
                                                                // but vshader.glsl computes it from gVP and gModelMatrix.
            objectLoader->render(mvpMatrix); // Pass MVP if ObjectLoader expects it, though it might be redundant.
                                             // Ideally, ObjectLoader::render() would not need this.
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
            }
        }
        camera->OnKeyboard(key);
    }

    void PassiveMouseCB(int x, int y)
    {
        mouseX = static_cast<double>(x); // Update global mouse X
        mouseY = static_cast<double>(y); // Update global mouse Y
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

    void InitCamera()
    {
        vec3 cameraPos = vec3(625.0f, 150.0f, 625.0f);
        vec3 cameraTarget = vec3(0.0f, -0.3f, 1.0f);
        vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
        float fov = 45.0f;
        float zNear = 0.1f;
        float zFar = 2000.0f; // Increased zFar for larger scenes
        PersProjInfo persProjInfo = {fov, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT), zNear, zFar};
        camera = std::make_unique<Camera>(persProjInfo, cameraPos, cameraTarget, cameraUp);
    }

    void InitObjects(){
        std::cout << "loading objects using unified shader..." << std::endl;

        // The unified shader (terrain shader) is now used for objects too.
        // Ensure 'shader' is already loaded by InitShader()
        if (!shader) {
            std::cerr << "Error: Unified shader not loaded before InitObjects!" << std::endl;
            return;
        }
        GLuint unifiedShaderProgramID = shader->getProgramID();

        // ObjectLoader is initialized with the program ID of the unified shader.
        // It will use this ID to get attribute/uniform locations if needed.
        objectLoader = new ObjectLoader(unifiedShaderProgramID);
        if (objectLoader) {
            std::vector<unsigned int> meshesToLoad = {4}; // Load specific mesh for cottage
            if (!objectLoader->load("../Objects/cottage_obj.obj", meshesToLoad)) {
                std::cerr << "Failed to load mesh 4 from model.obj with ObjectLoader." << std::endl;
            } else {
                std::cout << "Successfully called load for mesh 4 from model.obj." << std::endl;
            }
        } else {
            std::cerr << "Failed to create ObjectLoader instance." << std::endl;
        }
    }

    void InitMaterial()
    {
        // This material will be used by terrain, and by default, by objects too.
        m_terrainMaterial = std::make_unique<Material>(0.15f, 24.0f);
        // If objects need different material properties:
        // m_objectMaterial = std::make_unique<Material>(/* specular for object */, /* shininess for object */);
    }

    void InitLight()
    {
        GLfloat sunColorRed = 1.0f;
        GLfloat sunColorGreen = 1.0f;
        GLfloat sunColorBlue = 0.95f;
        GLfloat sunAmbientIntensity = 0.3f;
        GLfloat sunDirectionX = 0.4f;
        GLfloat sunDirectionY = 0.8f;
        GLfloat sunDirectionZ = 0.3f;
        GLfloat sunDiffuseIntensity = 0.85f;

        light = std::make_unique<Light>(sunColorRed, sunColorGreen, sunColorBlue, sunAmbientIntensity,
                                       sunDirectionX, sunDirectionY, sunDirectionZ, sunDiffuseIntensity);
        std::cout << "Directional Light initialized." << std::endl;
    }

    void InitShader()
    {
        auto& shaderManager = ShaderManager::getInstance();

        // Load the unified shader (previously terrain shader)
        shader = shaderManager.loadShader("unifiedShader", // Renamed for clarity
                                             "../shaders/vshader.glsl",
                                             "../shaders/fshader.glsl");
        if (!shader) {
            std::cerr << "Failed to load unified shader (shaders/vshader.glsl, shaders/fshader.glsl)" << std::endl;
            exit(-1);
        }
        std::cout << "Unified shader loaded successfully." << std::endl;

        // REMOVE loading of the separate object shader
        // objectShader = shaderManager.loadShader("Object", ...);
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
            "../resources/textures/grass.jpg",
            "../resources/textures/dirt.jpg",
            "../resources/textures/rock.jpg",
            "../resources/textures/snow.jpg"
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
    std::shared_ptr<Shader> shader; // This is now the single, unified shader
    // std::shared_ptr<Shader> objectShader; // REMOVE: No longer needed
    std::unique_ptr<TerrainGrid> grid;
    std::unique_ptr<Light> light;
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
