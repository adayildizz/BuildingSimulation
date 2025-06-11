#include "ObjectLoader.h"
#include "../../include/stb/stb_image.h"
#include <iostream>
#include <algorithm>
#include "../Core/Shader.h"

// Constructor
ObjectLoader::ObjectLoader(Shader& shaderProgram) {
    createDefaultWhiteTexture();
    boundingBoxCalculated = false;
    boundingBoxMin = vec3(0.0f);
    boundingBoxMax = vec3(0.0f);
}

// Destructor
ObjectLoader::~ObjectLoader() {
    cleanup();
}

void ObjectLoader::cleanup() {
    for (GLuint vao : vaos) glDeleteVertexArrays(1, &vao);
    for (GLuint vbo : vbos) glDeleteBuffers(1, &vbo);
    for (GLuint ebo : ebos) glDeleteBuffers(1, &ebo);
    for (GLuint texID : meshTextureIDs) glDeleteTextures(1, &texID);
    if (defaultWhiteTextureID != 0) glDeleteTextures(1, &defaultWhiteTextureID);

    vaos.clear();
    vbos.clear();
    ebos.clear();
    indexCounts.clear();
    meshTextureIDs.clear();
}

void ObjectLoader::createDefaultWhiteTexture() {
    glGenTextures(1, &defaultWhiteTextureID);
    glBindTexture(GL_TEXTURE_2D, defaultWhiteTextureID);
    unsigned char whitePixel[] = {255, 255, 255, 255}; // RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind after configuration
    
    std::cout << "Created default white texture with ID: " << defaultWhiteTextureID << std::endl;
}

bool ObjectLoader::load(const std::string& filename, const std::vector<unsigned int>& specificMeshesToLoad) {
    cleanup();
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename,
        aiProcess_Triangulate | aiProcess_GenNormals);

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Assimp load error for '" << filename << "': " << importer.GetErrorString() << std::endl;
        return false;
    }

    std::vector<unsigned int> meshesToLoadIndices = specificMeshesToLoad;
    if (meshesToLoadIndices.empty()) {
        for(unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            meshesToLoadIndices.push_back(i);
        }
    }

    for (unsigned int targetMeshIdx : meshesToLoadIndices) {
        if (targetMeshIdx >= scene->mNumMeshes) continue;
        
        aiMesh* mesh = scene->mMeshes[targetMeshIdx];
        std::vector<vec4> positions;
        std::vector<vec3> normals;
        std::vector<vec2> texCoords;
        std::vector<unsigned int> indices;
        GLuint currentMeshTextureID = 0; // Start with 0, will use defaultWhiteTextureID if no texture is loaded

        if (scene->HasMaterials() && mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                std::string modelDir = "";
                size_t lastSlash = filename.find_last_of("/");
                if (lastSlash != std::string::npos) {
                    modelDir = filename.substr(0, lastSlash + 1);
                }
                std::string fullTexPath = modelDir + texturePath.C_Str();
                std::cout <<  "reading texture from" << fullTexPath << std::endl;

                int texWidth, texHeight, texChannels;
                unsigned char *data = stbi_load(fullTexPath.c_str(), &texWidth, &texHeight, &texChannels, 0);
                if (data) {
                    GLuint textureID;
                    glGenTextures(1, &textureID);
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    GLenum format = (texChannels == 4) ? GL_RGBA : GL_RGB;
                    glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                    glBindTexture(GL_TEXTURE_2D, 0); // Unbind from the current texture unit (likely GL_TEXTURE0)
                    currentMeshTextureID = textureID;
                } else {
                    std::cerr << "Failed to load texture: " << fullTexPath << " - " << stbi_failure_reason() << std::endl;
                }
            }
        }

        // Vertex data
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D p = mesh->mVertices[i];
            positions.push_back(vec4(p.x, p.y, p.z,1.0f));
            aiVector3D n = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0, 1, 0);
            normals.push_back(vec3(n.x, n.y, n.z));
            texCoords.push_back(mesh->HasTextureCoords(0) ? vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : vec2(0.0f, 0.0f));
        }

        // Indices
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
                indices.push_back(mesh->mFaces[i].mIndices[j]);
        }

        std::vector<float> interleaved;
        for (size_t i = 0; i < positions.size(); ++i) {
            interleaved.insert(interleaved.end(), {
                positions[i].x, positions[i].y, positions[i].z, 1.0f,
                texCoords[i].x, texCoords[i].y,
                normals[i].x, normals[i].y, normals[i].z,
                1.0f, 1.0f, 1.0f, 1.0f // Default white color
            });
        }

        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float), interleaved.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        GLsizei stride = sizeof(float) * (4 + 2 + 3 + 4);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 4));
        
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 6));
        
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 9));

        glBindVertexArray(0);

        vaos.push_back(vao);
        vbos.push_back(vbo);
        ebos.push_back(ebo);
        indexCounts.push_back(indices.size());
        
        // Use defaultWhiteTextureID if no texture was loaded for this mesh
        if (currentMeshTextureID == 0) {
            currentMeshTextureID = defaultWhiteTextureID;
        }
        meshTextureIDs.push_back(currentMeshTextureID);
    }
    
    calculateBoundingBox(scene, meshesToLoadIndices);
    return true; 
}

void ObjectLoader::render(Shader& program) {
    GLint isTerrainLoc = glGetUniformLocation(program.getProgramID(), "u_isTerrain");
    if (isTerrainLoc != -1) {
        glUniform1i(isTerrainLoc, 0);
    }
    
    // Save current active texture unit
    GLint currentActiveTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &currentActiveTexture);
    
    // Activate texture unit 4 for object textures
    glActiveTexture(GL_TEXTURE4);
    
    for (size_t i = 0; i < vaos.size(); ++i) {
        GLint objectTexLoc = glGetUniformLocation(program.getProgramID(), "objectTexture");
        if (objectTexLoc != -1) {
            GLuint textureToUseFromMeshVector = meshTextureIDs[i];
            
            // Bind the appropriate texture to unit 4
            if (textureToUseFromMeshVector == 0) {
                glBindTexture(GL_TEXTURE_2D, defaultWhiteTextureID);
                //std::cout << "Mesh " << i << " USING defaultWhiteTextureID: " << defaultWhiteTextureID << std::endl;
            } else {
                glBindTexture(GL_TEXTURE_2D, textureToUseFromMeshVector);
                //std::cout << "Mesh " << i << " USING loaded texture ID: " << textureToUseFromMeshVector << std::endl;
            }
            glUniform1i(objectTexLoc, 4); // Tell shader to use texture unit 4
        }

        glBindVertexArray(vaos[i]);
        glDrawElements(GL_TRIANGLES, indexCounts[i], GL_UNSIGNED_INT, 0);
    }
    
    // Restore previous active texture unit to avoid disrupting other systems
    glActiveTexture(currentActiveTexture);
    glBindVertexArray(0); // Unbind VAO
}

void ObjectLoader::calculateBoundingBox(const aiScene* scene, const std::vector<unsigned int>& meshesToLoadIndices) {
    if (!scene || meshesToLoadIndices.empty()) {
        boundingBoxCalculated = false;
        return;
    }
    
    bool firstVertex = true;
    
    for (unsigned int targetMeshIdx : meshesToLoadIndices) {
        if (targetMeshIdx >= scene->mNumMeshes) continue;
        
        aiMesh* mesh = scene->mMeshes[targetMeshIdx];
        
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D vertex = mesh->mVertices[i];
            vec3 v(vertex.x, vertex.y, vertex.z);
            
            if (firstVertex) {
                boundingBoxMin = v;
                boundingBoxMax = v;
                firstVertex = false;
            } else {
                boundingBoxMin.x = std::min(boundingBoxMin.x, v.x);
                boundingBoxMin.y = std::min(boundingBoxMin.y, v.y);
                boundingBoxMin.z = std::min(boundingBoxMin.z, v.z);
                
                boundingBoxMax.x = std::max(boundingBoxMax.x, v.x);
                boundingBoxMax.y = std::max(boundingBoxMax.y, v.y);
                boundingBoxMax.z = std::max(boundingBoxMax.z, v.z);
            }
        }
    }
    
    boundingBoxCalculated = true;
    vec3 size = GetBoundingBoxSize();
    std::cout << "Calculated bounding box: Min(" << boundingBoxMin.x << ", " << boundingBoxMin.y << ", " << boundingBoxMin.z << ") "
              << "Max(" << boundingBoxMax.x << ", " << boundingBoxMax.y << ", " << boundingBoxMax.z << ") "
              << "Size(" << size.x << ", " << size.y << ", " << size.z << ")" << std::endl;
}

vec3 ObjectLoader::GetBoundingBoxSize() const {
    if (!boundingBoxCalculated) {
        return vec3(1.0f, 1.0f, 1.0f);
    }
    return boundingBoxMax - boundingBoxMin;
}

vec3 ObjectLoader::GetBoundingBoxMin() const {
    return boundingBoxMin;
}

vec3 ObjectLoader::GetBoundingBoxMax() const {
    return boundingBoxMax;
}
