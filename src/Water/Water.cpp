#include "Water.h"
#include <iostream>

Water::Water(Shader* program) : waterProgram(program) {
    // Initialize buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &NBO);

    // Load water textures
    m_dudvMap = std::make_shared<Texture>(GL_TEXTURE_2D, "resources/textures/dudvMap.jpg");
    m_normalMap = std::make_shared<Texture>(GL_TEXTURE_2D, "resources/textures/normalMap.jpg");
}

void Water::initMesh(const std::vector<vec4>& positions, 
                    const std::vector<vec2>& texCoords,
                    const std::vector<GLuint>& indices) {
    m_indices = indices;  // Store indices for later use
    
    glBindVertexArray(VAO);

    // Position buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(vec4), positions.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // Texture coordinates buffer
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(vec2), texCoords.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Water::draw(Texture& reflectionTexture, Texture& refractionTexture) const {
    if (!waterProgram || !waterProgram->isValid()) {
        std::cerr << "Error: Invalid water shader program" << std::endl;
        return;
    }

    waterProgram->use();

    // Bind reflection and refraction textures
    reflectionTexture.Bind(GL_TEXTURE0);
    refractionTexture.Bind(GL_TEXTURE1);
    m_dudvMap->Bind(GL_TEXTURE2);

    // Set texture uniforms
    waterProgram->setUniform("reflectionTexMap", 0);
    waterProgram->setUniform("refractionTexMap", 1);
    waterProgram->setUniform("dudvMap", 2);

    // Set time uniform for water animation
    static float time = 0.0f;
    time += 0.016f;  // Assuming 60 FPS
    waterProgram->setUniform("uTime", time);

    // Draw water mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glUseProgram(0);
}

