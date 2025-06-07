#include "Water.h"
#include <iostream>
#include "../include/stb/stb_image.h"

Water::Water(Shader* program) : waterProgram(program) {
    

}

void Water::generateMesh() {
    
    meshVertices.clear();
    meshIndices.clear();
    meshTexCoords.clear();

    for (int z = 0; z < GRID_SIZE; ++z) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            float xPos = float(x) / (GRID_SIZE - 1) - 0.5f;
            float zPos = float(z) / (GRID_SIZE - 1) - 0.5f;
            meshVertices.push_back(vec4(xPos, 0.0f, zPos, 1.0f));

            float u = float(x) / (GRID_SIZE - 1);
            float v = float(z) / (GRID_SIZE - 1);
            meshTexCoords.push_back(vec2(u, v));
        }
    }

    for (int z = 0; z < GRID_SIZE - 1; ++z) {
        for (int x = 0; x < GRID_SIZE - 1; ++x) {
            int topLeft = z * GRID_SIZE + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * GRID_SIZE + x;
            int bottomRight = bottomLeft + 1;

            // First triangle
            meshIndices.push_back(topLeft);
            meshIndices.push_back(bottomLeft);
            meshIndices.push_back(topRight);

            // Second triangle
            meshIndices.push_back(topRight);
            meshIndices.push_back(bottomLeft);
            meshIndices.push_back(bottomRight);
        }
    }
}

void Water::createWaterMeshVAO() {
    glGenVertexArrays(1, &waterVAO);
    glBindVertexArray(waterVAO);

    // position vbo
    glGenBuffers(1, &waterVBO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(vec4), meshVertices.data(), GL_STATIC_DRAW);

    GLuint vPos = glGetAttribLocation(waterProgram->getProgramID(), "vPosition");
    std::cout << "vPos: " << vPos << std::endl;
    glEnableVertexAttribArray(vPos);
    glVertexAttribPointer(vPos, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // Tex coord vbo
    glGenBuffers(1, &waterVBO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, meshTexCoords.size() * sizeof(vec2), meshTexCoords.data(), GL_STATIC_DRAW);

    GLuint vTex = glGetAttribLocation(waterProgram->getProgramID(), "vTexCoord");
    std::cout << "vTex: " << vTex << std::endl;
    glEnableVertexAttribArray(vTex);
    glVertexAttribPointer(vTex, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Index Buffer
    glGenBuffers(1, &waterEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(GLuint), meshIndices.data(), GL_STATIC_DRAW);
}

GLuint Water::loadTexture(const std::string& texturePath) {
    GLuint texture;
    int width, height, channels;
    unsigned char* image = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb);

    if (!image) {
        std::cerr << "ERROR: Could not load texture file '" << texturePath << "'" << std::endl;
        // ... (checkerboard fallback)
        glGenTextures(1, &texture);
        CheckGLError("loadTexture - glGenTextures (fallback)"); // Debug
        glBindTexture(GL_TEXTURE_2D, texture);
        CheckGLError("loadTexture - glBindTexture (fallback)"); // Debug
        // ...
    }
    else {
        std::cout << "Loaded texture: " << width << "x" << height << ", channels: " << channels << std::endl;

        glGenTextures(1, &texture);
        CheckGLError("loadTexture - glGenTextures"); // Debug: MOST LIKELY PLACE FOR 502
        glBindTexture(GL_TEXTURE_2D, texture);
        CheckGLError("loadTexture - glBindTexture"); // Debug
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        CheckGLError("loadTexture - glTexImage2D"); // Debug
        glGenerateMipmap(GL_TEXTURE_2D);
        CheckGLError("loadTexture - glGenerateMipmap"); // Debug
        stbi_image_free(image);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckGLError("loadTexture - glTexParameteri"); // Debug

    return texture;
    }



GLuint Water::createFBO(GLuint& texture, int width, int height) {
    GLuint fbo;
    GLuint drb;

    glGenFramebuffers(1, &fbo);
    CheckGLError("After glGenFramebuffers"); // Debug
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    CheckGLError("After glBindFramebuffer (FBO creation)"); // Debug

    glGenTextures(1, &texture);
    CheckGLError("After glGenTextures (FBO texture)"); // Debug
    glBindTexture(GL_TEXTURE_2D, texture);
    CheckGLError("After glBindTexture (FBO texture)"); // Debug
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    CheckGLError("After glTexImage2D (FBO texture)"); // Debug
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    CheckGLError("After glFramebufferTexture2D"); // Debug

    glGenRenderbuffers(1, &drb);
    CheckGLError("After glGenRenderbuffers"); // Debug
    glBindRenderbuffer(GL_RENDERBUFFER, drb);
    CheckGLError("After glBindRenderbuffer"); // Debug
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    CheckGLError("After glRenderbufferStorage"); // Debug
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, drb);
    CheckGLError("After glFramebufferRenderbuffer"); // Debug

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;
    }
    CheckGLError("After glCheckFramebufferStatus"); // Debug (though the cerr is already there)
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind when done
    CheckGLError("After glBindFramebuffer (unbinding FBO creation)"); // Debug

    return fbo;
}

// In main.cpp or a suitable utility header
void Water::CheckGLError(const std::string& location) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error at " << location << ": " << std::hex << error << std::dec << std::endl;
        // You can add more detailed error string mappings if you want, e.g.:
        // if (error == GL_INVALID_ENUM) std::cerr << "GL_INVALID_ENUM" << std::endl;
        // ... etc.
    }
}