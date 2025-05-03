#include "FlatGrid.h"
#include "BaseGrid.h"
#include <cassert>

FlatGrid::FlatGrid()
{
}

FlatGrid::~FlatGrid()
{
}

void FlatGrid::CreateFlatGrid(int Width, int Depth, const BaseGrid* baseGrid)
{
    m_width = Width;
    m_depth = Depth;

    // Create OpenGL state
    CreateGLState();

    // Populate buffers
    PopulateBuffers(baseGrid);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void FlatGrid::CreateGLState()
{
    // Create vertex array object
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    // Create vertex buffer
    glGenBuffers(1, &m_vb);
    glBindBuffer(GL_ARRAY_BUFFER, m_vb);
    
    // Create index buffer
    glGenBuffers(1, &m_ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ib);
    
    int POS_LOC = 0;
    size_t NumFloats = 0;
    // Enable position attribute
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(NumFloats * sizeof(float)));
    NumFloats += 3;
}

void FlatGrid::PopulateBuffers(const BaseGrid* baseGrid)
{
    // Create vertices
    std::vector<Vertex> vertices;
    vertices.resize(m_width * m_depth);
    InitVertices(baseGrid, vertices);
    
    // Create indices
    std::vector<unsigned int> indices;
    int numQuads = (m_width - 1) * (m_depth - 1);
    indices.resize(numQuads * 6); // 2 triangles per quad, 3 indices per triangle
    InitIndices(indices);
    
    // Send vertex data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
    
    // Send index data to GPU
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), &indices[0], GL_STATIC_DRAW);
}

void FlatGrid::InitVertices(const BaseGrid* baseGrid, std::vector<Vertex>& vertices)
{
    int index = 0;
    
    for (int z = 0; z < m_depth; z++) {
        for (int x = 0; x < m_width; x++) {
            assert(index < (int)vertices.size());
            vertices[index].InitVertex(baseGrid, x, z);
            index++;
        }
    }
    
    assert(index == (int)vertices.size());
}

void FlatGrid::Vertex::InitVertex(const BaseGrid* baseGrid, int x, int z)
{
    // Flat grid at y=0
    position = vec3(x * baseGrid->GetWorldScale(), 0.0f, z * baseGrid->GetWorldScale());
}

void FlatGrid::InitIndices(std::vector<unsigned int>& indices)
{
    int index = 0;
    
    for (int z = 0; z < m_depth - 1; z++) {
        for (int x = 0; x < m_width - 1; x++) {
            unsigned int indexBottomLeft = z * m_width + x;
            unsigned int indexTopLeft = (z + 1) * m_width + x;
            unsigned int indexTopRight = (z + 1) * m_width + x + 1;
            unsigned int indexBottomRight = z * m_width + x + 1;
            
            // Add top left triangle
            assert(index < (int)indices.size());
            indices[index++] = indexBottomLeft;
            assert(index < (int)indices.size());
            indices[index++] = indexTopLeft;
            assert(index < (int)indices.size());
            indices[index++] = indexTopRight;
            
            // Add bottom right triangle
            assert(index < (int)indices.size());
            indices[index++] = indexBottomLeft;
            assert(index < (int)indices.size());
            indices[index++] = indexTopRight;
            assert(index < (int)indices.size());
            indices[index++] = indexBottomRight;
        }
    }
    
    assert(index == (int)indices.size());
}

void FlatGrid::Render()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, (m_width - 1) * (m_depth - 1) * 6, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
}