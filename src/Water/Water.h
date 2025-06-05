#pragma once

#include "Angel.h"
#include "Core/Shader.h"
#include "Core/Texture.h"
#include <vector>

class Water {
public:
    float GRID_SIZE = 100;
    Water(Shader* program);
    void generateMesh();
    void createWaterMeshVAO();
    void createDummy();
    GLuint createFBO(GLuint& texture, int width, int height);   
    GLuint loadTexture(const std::string& texturePath);
    void renderToFBO(GLuint fbo, int width, int height, GLuint shaderProgram, GLuint vao, int vertexCount); 


    Shader* waterProgram;
    
    // OpenGL buffers
    GLuint waterVAO, waterVBO[2], waterEBO;

    std::vector<vec4> meshVertices;
    std::vector<GLuint> meshIndices;
    std::vector<vec2> meshTexCoords;
    // DUMMY
    GLuint dummyTexture;
    GLuint dummyFBO;
    GLuint dummyVAO, dummyVBO;
    static const int DUMMY_VERTICES_SIZE = 12; // 3 vertices * 4 floats per vertex
    float dummyVertices[DUMMY_VERTICES_SIZE] = {
        // Positions     // TexCoords
        -1.0f, -1.0f,     0.0f, 0.0f,
        3.0f, -1.0f,     2.0f, 0.0f,
        -1.0f,  3.0f,     0.0f, 2.0f
    };

   
    
};

