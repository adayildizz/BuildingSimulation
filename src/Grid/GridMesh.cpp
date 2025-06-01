#include "GridMesh.h"
#include "BaseGrid.h"
#include <cassert>

GridMesh::GridMesh()
{
}

GridMesh::~GridMesh()
{
    // Cleanup OpenGL resources
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vb);
    glDeleteBuffers(1, &m_ib);
}

void GridMesh::CreateMesh(int width, int depth, const BaseGrid* baseGrid)
{
    m_width = width;
    m_depth = depth;

    // Create OpenGL state
    CreateGLState();

    // Populate buffers
    PopulateBuffers(baseGrid);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GridMesh::CreateGLState()
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
    int TEX_LOC = 1;
    int NORMAL_LOC = 2; // Location for normals (matches shader)
    size_t CurrentOffset = 0; // Use an offset variable for clarity
    
    // Enable position attribute
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)CurrentOffset);
    CurrentOffset += sizeof(vec3);
    
    // Enable texture coordinate attribute
    glEnableVertexAttribArray(TEX_LOC);
    glVertexAttribPointer(TEX_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)CurrentOffset);
    CurrentOffset += sizeof(vec2);

    // Enable normal attribute // <<< ADDED BLOCK
    glEnableVertexAttribArray(NORMAL_LOC);
    glVertexAttribPointer(NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)CurrentOffset);
    // CurrentOffset += sizeof(vec3); // If there were more attributes after normal
}

void GridMesh::PopulateBuffers(const BaseGrid* baseGrid)
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

void GridMesh::InitVertices(const BaseGrid* baseGrid, std::vector<Vertex>& vertices)
{
    int index = 0;
    
    for (int z = 0; z < m_depth; z++) {
        for (int x = 0; x < m_width; x++) {
            assert(index < (int)vertices.size());
            vertices[index].InitPosAndTex(baseGrid, x, z); // Call renamed method
            index++;
        }
    }
    
    assert(index == (int)vertices.size());

    // After all positions are set, calculate normals
    CalculateNormals(baseGrid, vertices);
}


void GridMesh::CalculateNormals(const BaseGrid* baseGrid, std::vector<Vertex>& vertices)
{
    if (!baseGrid) return;

    for (int z = 0; z < m_depth; z++) {
        for (int x = 0; x < m_width; x++) {
            // Heights of neighboring points for finite difference
            // Handle boundaries by clamping coordinates
            float hL = baseGrid->GetHeight(std::max(0, x - 1), z);
            float hR = baseGrid->GetHeight(std::min(m_width - 1, x + 1), z);
            float hD = baseGrid->GetHeight(x, std::max(0, z - 1));
            float hU = baseGrid->GetHeight(x, std::min(m_depth - 1, z + 1));

            // If on an edge, the difference is only one-sided for that axis
            // For x: if x is 0, hL is height at x=0. if x is m_width-1, hR is height at m_width-1.
            // For z: if z is 0, hD is height at z=0. if z is m_depth-1, hU is height at m_depth-1.
            // This means for edges, the tangent might not be centered, which is acceptable for this method.

            // Create two tangent vectors on the surface.
            // The '2.0f * worldScale' factor accounts for the distance between hL/hR and hD/hU
            // If x is at a boundary, x+1 or x-1 might be the same as x due to clamping.
            // For true edge conditions, one of the height differences (e.g. hR-hL) might become zero if x is on an edge and its neighbor is clamped to itself.
            // A more robust edge handling would be to use one-sided differences at edges.
            // For simplicity, we use the clamped values.

            vec3 tangentX, tangentZ;

            if (x == 0) { // Left edge
                 tangentX = vec3(baseGrid->GetWorldScale(), baseGrid->GetHeight(x + 1, z) - baseGrid->GetHeight(x, z), 0.0f);
            } else if (x == m_width - 1) { // Right edge
                 tangentX = vec3(baseGrid->GetWorldScale(), baseGrid->GetHeight(x, z) - baseGrid->GetHeight(x - 1, z), 0.0f);
            } else { // Interior X
                 tangentX = vec3(2.0f * baseGrid->GetWorldScale(), hR - hL, 0.0f);
            }

            if (z == 0) { // Bottom edge
                tangentZ = vec3(0.0f, baseGrid->GetHeight(x, z + 1) - baseGrid->GetHeight(x, z), baseGrid->GetWorldScale());
            } else if (z == m_depth - 1) { // Top edge
                tangentZ = vec3(0.0f, baseGrid->GetHeight(x, z) - baseGrid->GetHeight(x, z - 1), baseGrid->GetWorldScale());
            } else { // Interior Z
                tangentZ = vec3(0.0f, hU - hD, 2.0f * baseGrid->GetWorldScale());
            }
            
            // The cross product gives the normal. Order matters for direction (Y-up).
            // cross(tangentZ, tangentX) typically for Y-up if X is right, Z is forward.
            vec3 normal = normalize(cross(tangentZ, tangentX));
            
            // If at any point normal calculation results in a zero vector (e.g. flat area with simple finite difference),
            // default to up. This check is crude; better methods exist for flat areas if this becomes an issue.
            if (length(normal) < 0.0001f) {
                normal = vec3(0.0f, 1.0f, 0.0f);
            }

            vertices[z * m_width + x].normal = normal;
        }
    }
}

void GridMesh::Vertex::InitPosAndTex(const BaseGrid* baseGrid, int x, int z)
{
    // Get the height from the grid at this position
    float y = baseGrid->GetHeight(x, z);
    
    // Set the position using the world scale and height
    position = vec3(x * baseGrid->GetWorldScale(), y, z * baseGrid->GetWorldScale());
    
    // Set texture coordinates
    texCoord = vec2(
        static_cast<float>(x) / (baseGrid->GetWidth() - 1) * baseGrid->GetTextureScale(),
        static_cast<float>(z) / (baseGrid->GetDepth() - 1) * baseGrid->GetTextureScale()
    );
    
    // Normal will be calculated later
    normal = vec3(0.0f, 1.0f, 0.0f); // Initialize to a default, e.g., pointing up
}


void GridMesh::InitIndices(std::vector<unsigned int>& indices) // Ensure it's unsigned int
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

void GridMesh::Render()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, (m_width - 1) * (m_depth - 1) * 6, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
} 
