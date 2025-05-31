#ifndef OBJECT_LOADER_H
#define OBJECT_LOADER_H

#include "Angel.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <iostream> // For error messages

class ObjectLoader {
public:
    ObjectLoader(GLuint shaderProgram);
    ~ObjectLoader();

    bool load(const std::string& filename, const std::vector<unsigned int>& meshesToLoadIndices = {});
    void render(const mat4& modelViewMatrix); // Modified to accept ModelView matrix

private:
    void createDefaultWhiteTexture();
    void cleanup(); // Helper for destructor and potential re-load

    GLuint program; // Shader program ID
    std::vector<GLuint> vaos, vbos, ebos;
    std::vector<int> indexCounts;
    std::vector<GLuint> meshTextureIDs; // Stores texture IDs for each mesh
    // meshColors is not directly used for rendering if textures are primary, 
    // but could be if shaders support vertex colors or untextured materials.
    // For now, assuming texture or default white.
    // std::vector<vec4> meshColors; 

    GLuint defaultWhiteTextureID;

    // Store ModelView uniform location, fetched in constructor
    GLuint modelViewUniformLocation; 
};

#endif // OBJECT_LOADER_H
