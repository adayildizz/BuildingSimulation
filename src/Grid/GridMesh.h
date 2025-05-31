#pragma once

#include "Angel.h"
#include <vector>

class BaseGrid; // Forward declaration

class GridMesh {
public:
    GridMesh();
    ~GridMesh();

    void CreateMesh(int width, int depth, const BaseGrid* baseGrid);
    void Render();

private:
    // Structure for vertices
    struct Vertex {
        vec3 position;
        vec2 texCoord;
        vec3 normal;   // Vertex normal

        // InitVertex will now only initialize position and texCoord.
        // Normals will be calculated in a separate step.
        void InitPosAndTex(const BaseGrid* baseGrid, int x, int z);
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
