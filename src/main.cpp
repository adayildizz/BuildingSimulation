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
#include <ctime> 
#include "ObjectLoader/ObjectLoader.h"  // Added for time


//global mouse pos
double mouseX = 0.0f;
double mouseY = 0.0f;
float objectPosX = 500.0f;
float ObjectPosY = 10.0f;
float ObjectPosZ = 600.0f;
// Constants
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const int GRID_SIZE = 250; // Size of the grid

// Forward declarations of callback functions
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
// Program ID
GLuint program;
GLuint ModelView, Projection;
//Objects
ObjectLoader* objectLoader;
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
        InitObjects();
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
        // Get the combined view-projection matrix from the camera once per frame
        mat4 viewProjMatrix = camera->GetViewProjMatrix(); 

        // Terrain Rendering
        if (terrainShader) { 
            terrainShader->use();
            // Set the View-Projection matrix for the terrain
            terrainShader->setUniform("gVP", viewProjMatrix); 
            
            terrainShader->setUniform("gMinHeight", m_minTerrainHeight);
            terrainShader->setUniform("gMaxHeight", m_maxTerrainHeight);
            
            for (size_t i = 0; i < m_terrainTextures.size(); ++i) {
                if (m_terrainTextures[i] && i < MAX_SHADER_TEXTURE_LAYERS) {
                    m_terrainTextures[i]->Bind(GL_TEXTURE0 + static_cast<GLenum>(i));
                    terrainShader->setUniform("gTextureHeight" + std::to_string(i), static_cast<int>(i));
                    terrainShader->setUniform("gHeight" + std::to_string(i), m_terrainTextureTransitionHeights[i]);
                }
            }
        }
        else{
            std::cout <<"terrain shader is missing." << std::endl;
        }

        grid->Render(); // Render the terrain grid
        
            
        // Object Rendering
        std::shared_ptr<Shader> currentObjectShader = ShaderManager::getInstance().getShader("Object");
        
        if (!currentObjectShader) {
            std::cerr << "Error: Object shader not found in RenderScene!" << std::endl;
        } else {
            currentObjectShader->use(); // Activate the object's shader program

            // Define a fixed world position for the object - moved further away

            vec3 fixedObjectWorldPos = vec3(objectPosX, ObjectPosY, ObjectPosZ); 
            float scale = 100.0f;
            float normX = (mouseX / WINDOW_WIDTH) * 2.0f - 1.0f;
            float normY = 1.0f - (mouseY / WINDOW_HEIGHT) * 2.0f;

            normX *= scale;
            normY *= scale;
            
            mat4 translation_from_cursor = Translate(fixedObjectWorldPos.x + normX, fixedObjectWorldPos.y,fixedObjectWorldPos.z + normY);
            

            

            mat4 objectScaleMatrix = Scale(5.0f,5.0f, 5.0f);
            mat4 objectModelMatrix = translation_from_cursor*objectScaleMatrix;
            // mat4 objectRotateMatrix = RotateY(45.0f); // Example: Rotate 45 degrees around Y
            // mat4 objectModelMatrix = objectTranslateMatrix * objectRotateMatrix * objectScaleMatrix;
            //mat4 objectModelMatrix = translation_from_cursor * objectScaleMatrix;
            
            // 2. Get the camera's View and Projection matrices
            
            mat4 viewMatrix = camera->GetViewMatrix(); 

            //Set the projection for the objects shader
            mat4 projMatrix = camera->GetProjMatrix(); 
            GLuint Projection = glGetUniformLocation(currentObjectShader->getProgramID(),"Projection");
            glUniformMatrix4fv(Projection,1,GL_TRUE, projMatrix); // Use this instead of the one from top of function for clarity here

            // modelview matrix
            mat4 mvpMatrix = viewMatrix * objectModelMatrix; 

            if (objectLoader) { // Ensure objectLoader is not null
                // ObjectLoader::render will now just bind VAO and draw, not set matrix uniforms
                objectLoader->render(mvpMatrix); // mvpMatrix, objectModelMatrix potentially unused by render now
            }
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

            std::cout << "X pos: " << objectPosX << "Y pos: " << ObjectPosY <<  "ObjectPos Z " << ObjectPosZ << std::endl;
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

    void InitObjects(){
        std::cout << "loading objects.." << std::endl;
      
        // Ensure the "Object" shader is loaded and get its program ID
        std::shared_ptr<Shader> objShader = ShaderManager::getInstance().getShader("Object");
        if (!objShader) {
            std::cerr << "Error: Object shader not found during InitObjects!" << std::endl;
            return; // Or handle error appropriately
        }
        GLuint objectShaderProgramID = objShader->getProgramID();

        objectLoader = new ObjectLoader(objectShaderProgramID); 
        if (objectLoader) {
            // Load only mesh 4 (assuming this is the main house model)
            std::vector<unsigned int> meshesToLoad = {4};
            if (!objectLoader->load("../Objects/model.obj", meshesToLoad)) {
                std::cerr << "Failed to load mesh 4 from model.obj with ObjectLoader." << std::endl;
            } else {
                std::cout << "Successfully called load for mesh 4 from model.obj." << std::endl;
            }
        } else {
            std::cerr << "Failed to create ObjectLoader instance." << std::endl;
        }
    }

    void InitCamera()
    {
        // Start slightly higher to see the crater mountains
        vec3 cameraPos = vec3(625.0f, 150.0f, 625.0f); // Increased Y for better view of crater
        // Point slightly downwards initially
        vec3 cameraTarget = vec3(0.0f, -0.3f, 1.0f); // TargetAdjusted target for new camera height
        vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
        
        float fov = 45.0f;
        float zNear = 1.0f; // Changed zNear back to 0.1f
        float zFar = 1000.0f;
        PersProjInfo persProjInfo = {fov, WINDOW_WIDTH, WINDOW_HEIGHT, zNear, zFar};

        camera = std::make_unique<Camera>(persProjInfo, cameraPos, cameraTarget, cameraUp);
    }

    void InitShader()
    {
        auto& shaderManager = ShaderManager::getInstance();
        
        // Load terrain shader
        terrainShader = shaderManager.loadShader("terrainShader", 
                                             "shaders/vshader.glsl", 
                                             "shaders/fshader.glsl");
        if (!terrainShader) {
            std::cerr << "Failed to load terrain shader (shaders/vshader.glsl, shaders/fshader.glsl)" << std::endl;
            exit(-1);
        }

        // Load object shader - Corrected paths
        objectShader = shaderManager.loadShader("Object", // Name for ShaderManager
                                             "shaders/vshader2.glsl", 
                                             "shaders/fshader2.glsl");
        if (!objectShader) {
            std::cerr << "Critical: Failed to load object shader (shaders/vshader2.glsl, shaders/fshader2.glsl). Please ensure these files exist in the 'build/shaders' directory. Exiting." << std::endl;
            exit(-1); 
        }
        program = objectShader->getProgramID();
        
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
    std::shared_ptr<Shader> terrainShader; // Renamed from shader
    std::shared_ptr<Shader> objectShader;  // For loaded objects
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
    mouseX=x;
    mouseY=y;
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
    //glFrontFace(GL_CCW);
    //glCullFace(GL_BACK);
    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    g_app->Run();

    delete g_app;
    return 0;
}