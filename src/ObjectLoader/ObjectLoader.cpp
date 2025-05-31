#include "ObjectLoader.h"
#include "../../include/stb/stb_image.h"
#include <iostream>
#include <algorithm> // Required for std::find

// Constructor
ObjectLoader::ObjectLoader(GLuint shaderProgram) : program(shaderProgram) {
    createDefaultWhiteTexture();
    // Get ModelView uniform location once
    modelViewUniformLocation = glGetUniformLocation(program, "ModelView");
    if (modelViewUniformLocation == -1) {
        std::cerr << "Warning: ModelView uniform not found in shader program." << std::endl;
    }
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
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind
}

bool ObjectLoader::load(const std::string& filename, const std::vector<unsigned int>& specificMeshesToLoad) {
    cleanup(); // Clean up any previously loaded model data

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename, 
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Assimp load error for '" << filename << "': " << importer.GetErrorString() << std::endl;
        return false;
    }

    std::vector<unsigned int> meshesToLoadIndices = specificMeshesToLoad;
    if (meshesToLoadIndices.empty()) { // If no specific meshes are requested, load all
        for(unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            meshesToLoadIndices.push_back(i);
        }
    }

    for (unsigned int targetMeshIdx : meshesToLoadIndices) {
        if (targetMeshIdx >= scene->mNumMeshes) {
            std::cout << "Skipping mesh index " << targetMeshIdx << " as it's out of bounds for '" << filename << "'." << std::endl;
            continue;
        }
        aiMesh* mesh = scene->mMeshes[targetMeshIdx];
        std::vector<vec4> positions;
        std::vector<vec3> normals;
        std::vector<vec2> texCoords;
        std::vector<unsigned int> indices;
        vec4 currentMeshMaterialColor(0.8f, 0.8f, 0.8f, 1.0f); // Default color
        GLuint currentMeshTextureID = defaultWhiteTextureID;

        // Material and Texture loading (simplified for brevity, adapt from your main.cpp)
        if (scene->HasMaterials() && mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiColor4D diffuseColorProperty;
            if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColorProperty)) {
                currentMeshMaterialColor = vec4(diffuseColorProperty.r, diffuseColorProperty.g, diffuseColorProperty.b, diffuseColorProperty.a);
                if (currentMeshMaterialColor.w < 0.01f) currentMeshMaterialColor.w = 1.0f;
            }

            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                std::string texPathStr = texturePath.C_Str();
                // Basic path handling: assume textures are relative to the model or in a known dir.
                // You might need to adjust this path based on your project structure.
                // For example, if model is in 'models/' and textures in 'models/textures/'
                // std::string fullTexPath = parent_directory_of_model_file + "/" + texPathStr;
                
                // Check if texturePath is relative and prepend model's directory if needed
                std::string modelDir = "";
                size_t lastSlash = filename.find_last_of("/");
                if (lastSlash != std::string::npos) {
                    modelDir = filename.substr(0, lastSlash + 1);
                }
                std::string fullTexPath = modelDir + texPathStr;

                int texWidth, texHeight, texChannels;
                unsigned char *data = stbi_load(fullTexPath.c_str(), &texWidth, &texHeight, &texChannels, 0);
                if (data) {
                    GLuint textureID;
                    glGenTextures(1, &textureID);
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    GLenum format = GL_RGB;
                    if (texChannels == 1) format = GL_RED;
                    else if (texChannels == 3) format = GL_RGB;
                    else if (texChannels == 4) format = GL_RGBA;
                    glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                    currentMeshTextureID = textureID;
                } else {
                    std::cerr << "Failed to load texture: " << fullTexPath << " - " << stbi_failure_reason() << std::endl;
                }
            }
        }

        // Vertex data
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D p = mesh->mVertices[i];
            positions.push_back(vec4(p.x, p.y, p.z, 1.0));
            aiVector3D n = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0, 1, 0); // Default normal if not present
            normals.push_back(vec3(n.x, n.y, n.z));
            if (mesh->HasTextureCoords(0)) {
                aiVector3D tc = mesh->mTextureCoords[0][i];
                texCoords.push_back(vec2(tc.x, tc.y));
            } else {
                texCoords.push_back(vec2(0.0f, 0.0f));
            }
        }

        // Indices
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                indices.push_back(face.mIndices[j]);
        }

        // Interleaved data
        std::vector<float> interleaved;
        for (size_t i = 0; i < positions.size(); ++i) {
            interleaved.insert(interleaved.end(), {
                positions[i].x, positions[i].y, positions[i].z, positions[i].w,
                normals[i].x, normals[i].y, normals[i].z,
                currentMeshMaterialColor.x, currentMeshMaterialColor.y, currentMeshMaterialColor.z, currentMeshMaterialColor.w, // Using loaded/default material color
                texCoords[i].x, texCoords[i].y
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

        GLsizei stride = sizeof(float) * (4 + 3 + 4 + 2); // Pos(4) + Normal(3) + Color(4) + TexCoord(2)

        GLuint vPosition = glGetAttribLocation(program, "vPosition");
        if (vPosition != -1) {
            glEnableVertexAttribArray(vPosition);
            glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
        }
        GLuint vNormal = glGetAttribLocation(program, "vNormal");
        if (vNormal != -1) {
            glEnableVertexAttribArray(vNormal);
            glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 4));
        }
        GLuint vColorLocation = glGetAttribLocation(program, "vColor");
        if (vColorLocation != -1) {
            glEnableVertexAttribArray(vColorLocation);
            glVertexAttribPointer(vColorLocation, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 7));
        }
        GLuint vTexCoordLocation = glGetAttribLocation(program, "vTexCoord_attr");
        if (vTexCoordLocation != -1) {
            glEnableVertexAttribArray(vTexCoordLocation);
            glVertexAttribPointer(vTexCoordLocation, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 11));
        } else {
             std::cerr << "Warning: vTexCoord_attr not found in shader for mesh " << targetMeshIdx << std::endl;
        }

        glBindVertexArray(0);

        vaos.push_back(vao);
        vbos.push_back(vbo);
        ebos.push_back(ebo);
        indexCounts.push_back(indices.size());
        meshTextureIDs.push_back(currentMeshTextureID);
    }
    return true;
}

void ObjectLoader::render(const mat4& modelViewMatrix) {

    glUseProgram(program);
    // Set the ModelView matrix passed from the caller
    if (modelViewUniformLocation != -1) {
        glUniformMatrix4fv(modelViewUniformLocation, 1, GL_TRUE, modelViewMatrix);
    }

    for (size_t i = 0; i < vaos.size(); ++i) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, meshTextureIDs[i]);
        glUniform1i(glGetUniformLocation(program, "diffuseTexture"), 0);

        glBindVertexArray(vaos[i]);
        glDrawElements(GL_TRIANGLES, indexCounts[i], GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}
