#pragma once

#include "Angel.h"
#include "Core/Shader.h"
#include "Core/Texture.h"
#include <vector>

class Water {
public:
    Water(Shader* program);
    void initMesh(const std::vector<vec4>& positions, 
                 const std::vector<vec2>& texCoords,
                 const std::vector<GLuint>& indices);
    void draw(Texture& reflectionTexture, Texture& refractionTexture) const;

private:
    Shader* waterProgram;
    
    // OpenGL buffers
    GLuint VAO;
    GLuint VBO; 
    GLuint EBO;
    GLuint NBO;

    // Water textures
    std::shared_ptr<Texture> m_dudvMap;
    std::shared_ptr<Texture> m_normalMap;

    // Store indices for drawing
    std::vector<GLuint> m_indices;
};

