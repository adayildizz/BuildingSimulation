#pragma once

#include "../../include/Angel.h"
#include <vector>

class BaseGrid;

class FlatGrid {
public:
    FlatGrid();
    ~FlatGrid();

    void CreateFlatGrid(int Width, int Depth, const BaseGrid* baseGrid);
    void Render();

private:

    // Structure for vertices
    struct Vertex {
        vec3 position;
        
        void InitVertex(const BaseGrid* baseGrid, int x, int z);
    };
    
    // Initialize OpenGL state
    void CreateGLState();
    
    // Populate buffers with vertices and indices
    void PopulateBuffers(const BaseGrid* baseGrid);
    
    // Initialize vertices and indices
    void InitVertices(const BaseGrid* baseGrid, std::vector<Vertex>& vertices);
    void InitIndices(std::vector<uint>& indices);
    
    // Grid dimensions
    int m_width = 0;
    int m_depth = 0;
    
    // OpenGL state
    GLuint m_vao;
    GLuint m_vb;
    GLuint m_ib;
};
    
    