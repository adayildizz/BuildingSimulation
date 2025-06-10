#pragma once

#include "Angel.h"
#include <vector>
#include <array>

class BaseGrid; // Forward declaration

class GridMesh {
public:
    // Constants for texture layers - updated to 5 for sand, grass, dirt, rock, snow
    static const int MAX_TEXTURE_LAYERS = 5;

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
        Vertex() : splatWeights{} { splatWeights.fill(0.0f); }
    };

    GridMesh();
    ~GridMesh();

    void CreateMesh(int width, int depth, const BaseGrid* baseGrid);
    void Render();
    void UpdateVertexBuffer(); // Add method to update vertex buffer
    void CalculateNormals(const BaseGrid* baseGrid, std::vector<Vertex>& vertices);

    // Access vertex data
    Vertex& GetVertex(int index) { return m_vertices[index]; }
    const Vertex& GetVertex(int index) const { return m_vertices[index]; }
    std::vector<Vertex>& GetVertices() { return m_vertices; }

private:
    // Initialize OpenGL state
    void CreateGLState();
    
    // Populate buffers with vertices and indices
    void PopulateBuffers(const BaseGrid* baseGrid);
    
    // Initialize vertices (positions, texCoords) and then calculate normals
    void InitVertices(const BaseGrid* baseGrid, std::vector<Vertex>& vertices);
    void InitIndices(std::vector<unsigned int>& indices);
    
    // Grid dimensions
    int m_width = 0;
    int m_depth = 0;
    
    // OpenGL state
    GLuint m_vao;
    GLuint m_vb;
    GLuint m_ib;

    // Vertex data
    std::vector<Vertex> m_vertices;
};
