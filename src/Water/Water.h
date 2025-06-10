#pragma once

#include "Angel.h"
#include "Core/Shader.h"
#include "Core/Texture.h"
#include <vector>
#include "Core/Camera.h"

class Water {
public:
    float GRID_SIZE = 100;
   
    Water(Shader* program);
    void generateMesh();
    void createWaterMeshVAO();
    
    GLuint createFBO(GLuint& texture, int width, int height);   
    GLuint loadTexture(const std::string& texturePath);
    void renderToFBO(GLuint fbo, int width, int height, GLuint shaderProgram, GLuint vao, int vertexCount); 
   
    void CheckGLError(const std::string& location) ;
    Shader* waterProgram;
    
    // OpenGL buffers
    GLuint waterVAO, waterVBO[2], waterEBO;

    std::vector<vec4> meshVertices;
    std::vector<GLuint> meshIndices;
    std::vector<vec2> meshTexCoords;
    
    GLuint dudvTexture;
    // For Reflection FBO
    GLuint reflectionFBO;
    GLuint reflectionTexture;
    GLuint reflectionDepthBuffer;
    // For Refraction FBO
    GLuint refractionFBO;
    GLuint refractionTexture;
    GLuint refractionDepthBuffer;

    
};

