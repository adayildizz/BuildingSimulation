#pragma once

#include "Angel.h"
#include <vector>
#include <array>

class BaseGrid; // Forward declaration

class GridMesh {
public:
    GridMesh();
    ~GridMesh();

    void CreateMesh(int width, int depth, const BaseGrid* baseGrid);
    void Render();

    // Constants for texture layers - updated to 5 for sand, grass, dirt, rock, snow
    static const int MAX_TEXTURE_LAYERS = 5;

private:
    // Structure for vertices
    struct Vertex {
        vec3 position;
        vec2 texCoord;
        vec3 normal;   // Vertex normal
        std::array<float, MAX_TEXTURE_LAYERS> splatWeights; // Weights for texture blending

        // InitVertex will now only initialize position and texCoord.
        // Normals will be calculated in a separate step.
        void InitPosAndTex(const BaseGrid* baseGrid, int x, int z);
        
        // Initialize splat weights based on height
        void InitSplatWeights(const BaseGrid* baseGrid, int x, int z, float minHeight, float maxHeight);
        
        // Constructor to initialize splat weights to zero
        Vertex() : splatWeights{0.0f, 0.0f, 0.0f, 0.0f, 0.0f} {}
    };
    
    // Initialize OpenGL state
    void CreateGLState();
    
    // Populate buffers with vertices and indices
    void PopulateBuffers(const BaseGrid* baseGrid);
    
    // Initialize vertices (positions, texCoords) and then calculate normals
    void InitVertices(const BaseGrid* baseGrid, std::vector<Vertex>& vertices);
    void CalculateNormals(const BaseGrid* baseGrid, std::vector<Vertex>& vertices); // <<< ADDED: For calculating normals
    void InitIndices(std::vector<unsigned int>& indices); // Changed uint to unsigned int for consistency
    
    // Grid dimensions
    int m_width = 0;
    int m_depth = 0;
    
    // OpenGL state
    GLuint m_vao;
    GLuint m_vb;
    GLuint m_ib;
};
