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
    glEnableVertexAttribArray(vPos);
    glVertexAttribPointer(vPos, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // Tex coord vbo
    glGenBuffers(1, &waterVBO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, meshTexCoords.size() * sizeof(vec2), meshTexCoords.data(), GL_STATIC_DRAW);

    GLuint vTex = glGetAttribLocation(waterProgram->getProgramID(), "vTexCoord");
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
        // We generate a checkerboard pattern
        const int texWidth = 2, texHeight = 2;
        unsigned char checker[texWidth * texHeight * 3] = {
            255, 255, 255,   0, 0, 0,
            0, 0, 0,   255, 255, 255
        };

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0,
            GL_RGB, GL_UNSIGNED_BYTE, checker);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Loaded texture: " << width << "x" << height
            << ", channels: " << channels << std::endl;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(image);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}


GLuint Water::createFBO(GLuint& texture, int width, int height)
{
    GLuint fbo;
    GLuint drb;

    //generating framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);


    // we need to bind our texture 
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);


    // generating depth renderbuffer
    glGenRenderbuffers(1, &drb);
    glBindRenderbuffer(GL_RENDERBUFFER, drb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, drb);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind when done

    return fbo;
}

void Water::renderToFBO(GLuint fbo, int width, int height, GLuint shaderProgram, GLuint vao, int vertexCount) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount); // or however many vertices

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Water::createRefraction(Camera* camera, Shader* mainShader, float waterHeight)
{
    glBindFramebuffer(GL_FRAMEBUFFER, refractionFBO);
    glViewport(0, 0, 512, 512); // hardcoded
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get view and projection matrices from camera
    mat4 view = camera->GetViewMatrix();
    mat4 projection = camera->GetProjMatrix();
    mat4 viewProjMatrix = view * projection;

    mainShader->use();

    // setup for clipPlane - this will clip everything above water
    vec4 clipPlaneRefraction = vec4(0.0f, -1.0f, 0.0f, waterHeight); 
    mainShader->setUniform("clipPlane", clipPlaneRefraction);

    // Set view-projection matrix for the scene
    mainShader->setUniform("gVP", viewProjMatrix);

    // Render terrain
    mainShader->setUniform("gModelMatrix", mat4(1.0f));  // Identity matrix for terrain
    mainShader->setUniform("u_isTerrain", true);
    // Note: The terrain rendering will be handled by the caller

    // Render objects
    mainShader->setUniform("u_isTerrain", false);
    // Note: The object rendering will be handled by the caller

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to screen
}
