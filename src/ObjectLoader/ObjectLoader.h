#ifndef OBJECT_LOADER_H
#define OBJECT_LOADER_H

#include "Angel.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <iostream> 
#include "../Core/Shader.h" //For error messages

class ObjectLoader {
public:
    ObjectLoader(Shader& shaderProgram);
    ~ObjectLoader();

    bool load(const std::string& filename, const std::vector<unsigned int>& meshesToLoadIndices = {});
    void render();
    void RenderMeshOnly();
    Shader program;
    
    // Bounding box methods
    vec3 GetBoundingBoxSize() const;
    vec3 GetBoundingBoxMin() const;
    vec3 GetBoundingBoxMax() const;

private:
    void createDefaultWhiteTexture();
    void cleanup(); // Helper for destructor and potential re-load
    void calculateBoundingBox(const aiScene* scene, const std::vector<unsigned int>& meshesToLoadIndices);

     // Shader program ID
    std::vector<GLuint> vaos, vbos, ebos;
    std::vector<int> indexCounts;
    std::vector<GLuint> meshTextureIDs;
     // Stores texture IDs for each mesh
    // meshColors is not directly used for rendering if textures are primary, 
    // but could be if shaders support vertex colors or untextured materials.
    // For now, assuming texture or default white.
    // std::vector<vec4> meshColors; 
    
    GLuint defaultWhiteTextureID;
    
    // Bounding box data
    vec3 boundingBoxMin;
    vec3 boundingBoxMax;
    bool boundingBoxCalculated;

};

#endif // OBJECT_LOADER_H
