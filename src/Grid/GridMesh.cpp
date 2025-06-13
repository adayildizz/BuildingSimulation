#include "GridMesh.h"
#include "BaseGrid.h"
#include "TerrainGrid.h"
#include <cassert>
#include <algorithm>

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
    int NORMAL_LOC = 2;
    int SPLAT_WEIGHTS_LOC = 3; // First 4 weights (sand, grass, dirt, rock)
    int SPLAT_WEIGHT5_LOC = 4; // Fifth weight (snow)
    size_t CurrentOffset = 0;
    
    // Enable position attribute
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)CurrentOffset);
    CurrentOffset += sizeof(vec3);
    
    // Enable texture coordinate attribute
    glEnableVertexAttribArray(TEX_LOC);
    glVertexAttribPointer(TEX_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)CurrentOffset);
    CurrentOffset += sizeof(vec2);

    // Enable normal attribute
    glEnableVertexAttribArray(NORMAL_LOC);
    glVertexAttribPointer(NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)CurrentOffset);
    CurrentOffset += sizeof(vec3);
    
    // Enable splat weights attribute - first 4 weights (sand, grass, dirt, rock)
    glEnableVertexAttribArray(SPLAT_WEIGHTS_LOC);
    glVertexAttribPointer(SPLAT_WEIGHTS_LOC, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)CurrentOffset);
    CurrentOffset += sizeof(float) * 4;
    
    // Enable fifth splat weight (snow) as a separate float
    glEnableVertexAttribArray(SPLAT_WEIGHT5_LOC);
    glVertexAttribPointer(SPLAT_WEIGHT5_LOC, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)CurrentOffset);
    CurrentOffset += sizeof(float);
}

void GridMesh::PopulateBuffers(const BaseGrid* baseGrid)
{
    // Ensure m_vertices is the member variable
    m_vertices.resize(m_width * m_depth); // m_width and m_depth are GridMesh members
    InitVertices(baseGrid, m_vertices);    // Pass the member m_vertices
    
    // Create indices
    std::vector<unsigned int> indices;
    int numQuads = (m_width - 1) * (m_depth - 1);
    indices.resize(numQuads * 6); // 2 triangles per quad, 3 indices per triangle
    InitIndices(indices);
    
    // Send vertex data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_vb); // Bind m_vb before glBufferData
    if (!m_vertices.empty()) {
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);
    } else {
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW); // Handle empty case
    }
    
    // Send index data to GPU
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ib); // Bind m_ib before glBufferData
    if (!indices.empty()) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
    } else {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
    }
}

void GridMesh::InitVertices(const BaseGrid* baseGrid, std::vector<Vertex>& vertices_ref)
{
    const TerrainGrid* terrainGrid = dynamic_cast<const TerrainGrid*>(baseGrid);
    float minHeight = terrainGrid ? terrainGrid->GetMinHeight() : 0.0f;
    float maxHeight = terrainGrid ? terrainGrid->GetMaxHeight() : 1.0f;

    int index = 0;
    
    for (int z = 0; z < m_depth; z++) { // Use GridMesh's m_depth
        for (int x = 0; x < m_width; x++) { // Use GridMesh's m_width
            if (index < vertices_ref.size()) { // Check bounds
                vertices_ref[index].InitPosAndTex(baseGrid, x, z);
                vertices_ref[index].InitSplatWeights(baseGrid, x, z, minHeight, maxHeight);
            }
            index++;
        }
    }
    
    // After all positions are set, calculate normals using the same vertices_ref
    CalculateNormals(baseGrid, vertices_ref);
}

void GridMesh::CalculateNormals(const BaseGrid* baseGrid, std::vector<Vertex>& vertices_ref)
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
                 tangentX = vec3(2.0f * baseGrid->GetWorldScale(), baseGrid->GetHeight(std::min(m_width - 1, x + 1), z) - baseGrid->GetHeight(std::max(0, x - 1), z), 0.0f);
            }

            if (z == 0) { // Bottom edge
                tangentZ = vec3(0.0f, baseGrid->GetHeight(x, z + 1) - baseGrid->GetHeight(x, z), baseGrid->GetWorldScale());
            } else if (z == m_depth - 1) { // Top edge
                tangentZ = vec3(0.0f, baseGrid->GetHeight(x, z) - baseGrid->GetHeight(x, z - 1), baseGrid->GetWorldScale());
            } else { // Interior Z
                tangentZ = vec3(0.0f, baseGrid->GetHeight(x, std::min(m_depth - 1, z + 1)) - baseGrid->GetHeight(x, std::max(0, z - 1)), 2.0f * baseGrid->GetWorldScale());
            }
            
            // The cross product gives the normal. Order matters for direction (Y-up).
            // cross(tangentZ, tangentX) typically for Y-up if X is right, Z is forward.
            vec3 normal = normalize(cross(tangentZ, tangentX));
            
            // If at any point normal calculation results in a zero vector (e.g. flat area with simple finite difference),
            // default to up. This check is crude; better methods exist for flat areas if this becomes an issue.
            if (length(normal) < 0.0001f) {
                normal = vec3(0.0f, 1.0f, 0.0f);
            }

            if ((z * m_width + x) < vertices_ref.size()){ // Check bounds
                vertices_ref[z * m_width + x].normal = normal;
            }
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

void GridMesh::Vertex::InitSplatWeights(const BaseGrid* baseGrid, int x, int z, float minHeight, float maxHeight)
{
    // Initialize weights to zero
    splatWeights.fill(0.0f);
    
    // Get current height at this position
    float currentHeight = baseGrid->GetHeight(x, z);
    
    // Calculate height-based weights for 5 textures: sand, grass, dirt, rock, snow
    float heightRange = maxHeight - minHeight;
    if (heightRange <= 1e-5f) {
        // If terrain is flat, default to first texture (sand)
        splatWeights[0] = 1.0f;
        return;
    }
    C
    // Calculate transition heights for 5 textures
    float transitionHeight1 = minHeight + heightRange * 0.20f; // sand -> grass
    float transitionHeight2 = minHeight + heightRange * 0.40f; // grass -> dirt
    float transitionHeight3 = minHeight + heightRange * 0.60f; // dirt -> rock
    float transitionHeight4 = minHeight + heightRange * 0.80f; // rock -> snow
    float transitionHeight5 = maxHeight; // final snow
    
    // Apply blending logic for 5 textures
    if (currentHeight <= transitionHeight1) {
        splatWeights[0] = 1.0f; // Pure sand
    } else if (currentHeight <= transitionHeight2) {
        // Blend between sand and grass
        float range = transitionHeight2 - transitionHeight1;
        float blendFactor = (range > 0.0001f) ? (currentHeight - transitionHeight1) / range : 0.0f;
        blendFactor = std::clamp(blendFactor, 0.0f, 1.0f);
        splatWeights[0] = 1.0f - blendFactor; // sand
        splatWeights[1] = blendFactor;        // grass
    } else if (currentHeight <= transitionHeight3) {
        // Blend between grass and dirt
        float range = transitionHeight3 - transitionHeight2;
        float blendFactor = (range > 0.0001f) ? (currentHeight - transitionHeight2) / range : 0.0f;
        blendFactor = std::clamp(blendFactor, 0.0f, 1.0f);
        splatWeights[1] = 1.0f - blendFactor; // grass
        splatWeights[2] = blendFactor;        // dirt
    } else if (currentHeight <= transitionHeight4) {
        // Blend between dirt and rock
        float range = transitionHeight4 - transitionHeight3;
        float blendFactor = (range > 0.0001f) ? (currentHeight - transitionHeight3) / range : 0.0f;
        blendFactor = std::clamp(blendFactor, 0.0f, 1.0f);
        splatWeights[2] = 1.0f - blendFactor; // dirt
        splatWeights[3] = blendFactor;        // rock
    } else if (currentHeight <= transitionHeight5) {
        // Blend between rock and snow
        float range = transitionHeight5 - transitionHeight4;
        float blendFactor = (range > 0.0001f) ? (currentHeight - transitionHeight4) / range : 0.0f;
        blendFactor = std::clamp(blendFactor, 0.0f, 1.0f);
        splatWeights[3] = 1.0f - blendFactor; // rock
        splatWeights[4] = blendFactor;        // snow
    } else {
        splatWeights[4] = 1.0f; // Pure snow
    }
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

void GridMesh::UpdateVertexBuffer()
{
    glBindBuffer(GL_ARRAY_BUFFER, m_vb);
    if (!m_vertices.empty()) { // Check if m_vertices is empty
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * m_vertices.size(), m_vertices.data());
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
} 
