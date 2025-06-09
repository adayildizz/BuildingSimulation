#include "TerrainGrid.h"
#include "TerrainGenerator.h"
#include "GridMesh.h"
#include <fstream>
#include <cmath>
#include <cassert>
#include <iostream>
#include <random>     // Added for std::mt19937 and std::uniform_real_distribution
#include <algorithm>  // Added for std::min/max

TerrainGrid::TerrainGrid() : BaseGrid(), m_terrainType(TerrainType::FLAT), m_minHeight(0.0f), m_maxHeight(0.0f),
    m_flattenTargetHeight(0.0f), m_isFirstFlattenClick(true)
{
    // m_layerInfo will be default constructed, then set in Init
}

TerrainGrid::~TerrainGrid() 
{
}

void TerrainGrid::Init(int width, int depth, float worldScale, float textureScale, 
                       TerrainType terrainType, float genParam1, float genParam2,
                       int genIterations, float genFilterFactor, float genFaultDisplacementScale)
{
    m_width = width;
    m_depth = depth;
    m_terrainType = terrainType; // Store terrain type

    TerrainGenerator generator(width, depth);
    m_heightMap = generator.GenerateHeightmap(terrainType, genParam1, genParam2, 
                                              genIterations, genFilterFactor, genFaultDisplacementScale);
    
    m_layerInfo = generator.GetLayerInfo(terrainType); // Store layer info

    CalculateMinMaxHeights(); // Calculate and store min/max heights from m_heightMap

    BaseGrid::Init(width, depth, worldScale, textureScale);
}

void TerrainGrid::CalculateMinMaxHeights() {
    if (m_heightMap.empty()) {
        m_minHeight = 0.0f;
        m_maxHeight = 0.0f;
        return;
    }

    m_minHeight = m_heightMap[0];
    m_maxHeight = m_heightMap[0];
    for (float h : m_heightMap) {
        if (h < m_minHeight) m_minHeight = h;
        if (h > m_maxHeight) m_maxHeight = h;
    }
}

float TerrainGrid::GetHeight(int x, int z) const 
{
    // Check bounds (using m_width, m_depth which should be set by Init)
    if (x < 0 || x >= m_width || z < 0 || z >= m_depth) {
        // Consider logging an error or returning a specific error code/value
        return 0.0f; // Default height for out-of-bounds
    }
    
    // Get index in heightmap
    // Ensure m_heightMap is not empty and index is within bounds
    size_t index = static_cast<size_t>(z) * m_width + x;
    if (index < m_heightMap.size()) {
        return m_heightMap[index];
    }
    
    // Fallback if index is somehow out of bounds despite coordinate checks (should not happen)
    // This might indicate an issue with m_width/m_depth or m_heightMap sizing.
    std::cerr << "Error: Heightmap access out of bounds! x:" << x << ", z:" << z << std::endl;
    return 0.0f;
}

// Implementation of new getters
TerrainGrid::TerrainType TerrainGrid::GetTerrainType() const {
    return m_terrainType;
}

const TerrainGrid::TerrainLayerInfo& TerrainGrid::GetLayerInfo() const {
    return m_layerInfo;
}

float TerrainGrid::GetMinHeight() const {
    return m_minHeight;
}

float TerrainGrid::GetMaxHeight() const {
    return m_maxHeight;
}

float TerrainGrid::GetHeightAtWorldPos(float worldX, float worldZ) const {
    // Convert world coordinates to grid coordinates
    float gridX = worldX / GetWorldScale();
    float gridZ = worldZ / GetWorldScale();
    
    // Get the integer grid coordinates
    int x0 = static_cast<int>(floor(gridX));
    int z0 = static_cast<int>(floor(gridZ));
    int x1 = x0 + 1;
    int z1 = z0 + 1;
    
    // Check bounds
    if (x0 < 0 || x1 >= GetWidth() || z0 < 0 || z1 >= GetDepth()) {
        return 0.0f; // Return 0 height for out-of-bounds coordinates
    }
    
    // Get fractional parts for interpolation
    float fx = gridX - x0;
    float fz = gridZ - z0;
    
    // Get heights at four corners
    float h00 = GetHeight(x0, z0);
    float h10 = GetHeight(x1, z0);
    float h01 = GetHeight(x0, z1);
    float h11 = GetHeight(x1, z1);
    
    // Bilinear interpolation
    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;
    float height = h0 * (1.0f - fz) + h1 * fz;
    
    return height;
}

void TerrainGrid::PaintTexture(float worldX, float worldZ, int textureLayer, float brushRadius, float brushStrength)
{
    // Convert world coordinates to grid coordinates
    int centerX = static_cast<int>(worldX / m_worldScale);
    int centerZ = static_cast<int>(worldZ / m_worldScale);
    
    // Calculate brush radius in grid units
    int radiusInGrid = static_cast<int>(brushRadius / m_worldScale);
    
    // Clamp texture layer to valid range
    textureLayer = std::clamp(textureLayer, 0, GridMesh::MAX_TEXTURE_LAYERS - 1);
    
    // Iterate over the brush area
    for (int z = centerZ - radiusInGrid; z <= centerZ + radiusInGrid; z++) {
        for (int x = centerX - radiusInGrid; x <= centerX + radiusInGrid; x++) {
            // Skip if outside grid bounds
            if (x < 0 || x >= m_width || z < 0 || z >= m_depth) continue;
            
            // Calculate distance from brush center
            float dx = (x - centerX) * m_worldScale;
            float dz = (z - centerZ) * m_worldScale;
            float distance = sqrt(dx * dx + dz * dz);
            
            // Skip if outside brush radius
            if (distance > brushRadius) continue;
            
            // Calculate falloff based on distance
            float falloff = 1.0f - (distance / brushRadius);
            float strength = brushStrength * falloff;
            
            // Get current vertex
            int vertexIndex = z * m_width + x;
            auto& vertex = m_gridMesh->GetVertex(vertexIndex);
            
            // Increase weight for selected texture layer
            vertex.splatWeights[textureLayer] += strength;
            
            // Normalize weights
            NormalizeSplatWeights(x, z);
        }
    }
    
    // Update the mesh to reflect changes
    UpdateMesh();
}

void TerrainGrid::Flatten(float worldX, float worldZ, float brushRadius, float brushStrength)
{
    // Convert world coordinates to grid coordinates
    int centerX = static_cast<int>(worldX / m_worldScale);
    int centerZ = static_cast<int>(worldZ / m_worldScale);
    
    // On first click, store the target height
    if (m_isFirstFlattenClick) {
        m_flattenTargetHeight = GetHeight(centerX, centerZ);
        m_isFirstFlattenClick = false;
        std::cout << "Initial target height set to: " << m_flattenTargetHeight << std::endl;
    }
    
    // Calculate brush radius in grid units
    int radiusInGrid = static_cast<int>(brushRadius / m_worldScale);
    
    // Iterate over the brush area
    for (int z = centerZ - radiusInGrid; z <= centerZ + radiusInGrid; z++) {
        for (int x = centerX - radiusInGrid; x <= centerX + radiusInGrid; x++) {
            // Skip if outside grid bounds
            if (x < 0 || x >= m_width || z < 0 || z >= m_depth) continue;
            
            // Calculate distance from brush center
            float dx = (x - centerX) * m_worldScale;
            float dz = (z - centerZ) * m_worldScale;
            float distance = sqrt(dx * dx + dz * dz);
            
            // Skip if outside brush radius
            if (distance > brushRadius) continue;
            
            // Calculate falloff based on distance
            float falloff = 1.0f - (distance / brushRadius);
            
            // Get current height
            float currentHeight = GetHeight(x, z);
            
            // Calculate new height - interpolate between current height and initial target height
            float newHeight = currentHeight + (m_flattenTargetHeight - currentHeight) * falloff * brushStrength;
            
            // Update height in heightmap
            m_heightMap[z * m_width + x] = newHeight;
        }
    }
    
    // Update min/max heights
    CalculateMinMaxHeights();
    
    // Update the mesh to reflect changes
    UpdateMesh();
}

std::vector<vec3> TerrainGrid::Dig(float worldX, float worldZ, float brushRadius, float brushStrength)
{
    std::vector<vec3> dugPoints; 
    
    // Convert world coordinates to grid coordinates
    int centerX = static_cast<int>(worldX / m_worldScale);
    int centerZ = static_cast<int>(worldZ / m_worldScale);
    
    // Calculate brush radius in grid units
    int radiusInGrid = static_cast<int>(brushRadius / m_worldScale);
    
    // Iterate over the brush area
    for (int z = centerZ - radiusInGrid; z <= centerZ + radiusInGrid; z++) {
        for (int x = centerX - radiusInGrid; x <= centerX + radiusInGrid; x++) {
            // Skip if outside grid bounds
            if (x < 0 || x >= m_width || z < 0 || z >= m_depth) continue;
            
            // Calculate distance from brush center
            float dx = (x - centerX) * m_worldScale;
            float dz = (z - centerZ) * m_worldScale;
            float distance = sqrt(dx * dx + dz * dz);
            
            // Skip if outside brush radius
            if (distance > brushRadius) continue;
            
            // Calculate falloff based on distance - using quadratic falloff for bowl shape
            float normalizedDistance = distance / brushRadius;
            float falloff = (1.0f - normalizedDistance * normalizedDistance);
            
            // Get current height
            float currentHeight = GetHeight(x, z);
            
            // Calculate new height - create bowl shape by lowering height more at center
            float depth = brushStrength * falloff;
            float newHeight = currentHeight - depth;
            
            // Update height in heightmap
            m_heightMap[z * m_width + x] = newHeight;
            
            // Store the point that was dug (in world coordinates)
            vec3 dugPoint(x * m_worldScale, newHeight, z * m_worldScale);
            dugPoints.push_back(dugPoint);
        }
    }
    
    // Update min/max heights
    CalculateMinMaxHeights();
    
    // Update the mesh to reflect changes
    UpdateMesh();
    
    return dugPoints;
}

void TerrainGrid::ResetFlatteningState()
{
    m_isFirstFlattenClick = true;
}

void TerrainGrid::NormalizeSplatWeights(int x, int z)
{
    int vertexIndex = z * m_width + x;
    auto& vertex = m_gridMesh->GetVertex(vertexIndex);
    
    // Calculate sum of weights
    float sum = 0.0f;
    for (int i = 0; i < GridMesh::MAX_TEXTURE_LAYERS; i++) {
        sum += vertex.splatWeights[i];
    }
    
    // Normalize if sum is not zero
    if (sum > 0.0f) {
        for (int i = 0; i < GridMesh::MAX_TEXTURE_LAYERS; i++) {
            vertex.splatWeights[i] /= sum;
        }
    }
}

void TerrainGrid::UpdateMesh()
{
    // Update vertex positions based on new heights
    for (int z = 0; z < m_depth; z++) {
        for (int x = 0; x < m_width; x++) {
            int vertexIndex = z * m_width + x;
            auto& vertex = m_gridMesh->GetVertex(vertexIndex);
            
            // Update position with new height
            float y = m_heightMap[vertexIndex];
            vertex.position = vec3(x * m_worldScale, y, z * m_worldScale);
        }
    }
    
    // Recalculate normals after height changes
    m_gridMesh->CalculateNormals(this, m_gridMesh->GetVertices());
    
    // Update the vertex buffer on the GPU
    m_gridMesh->UpdateVertexBuffer();
}

void TerrainGrid::RaiseTerrain(float worldX, float worldZ, float height, float brushRadius, float brushStrength)
{
    // Convert world coordinates to grid coordinates
    int centerX = static_cast<int>(worldX / m_worldScale);
    int centerZ = static_cast<int>(worldZ / m_worldScale);
    
    // Calculate brush radius in grid units
    int radiusInGrid = static_cast<int>(brushRadius / m_worldScale);
    
    // Iterate over the brush area
    for (int z = centerZ - radiusInGrid; z <= centerZ + radiusInGrid; z++) {
        for (int x = centerX - radiusInGrid; x <= centerX + radiusInGrid; x++) {
            // Skip if outside grid bounds
            if (x < 0 || x >= m_width || z < 0 || z >= m_depth) continue;
            
            // Calculate distance from brush center
            float dx = (x - centerX) * m_worldScale;
            float dz = (z - centerZ) * m_worldScale;
            float distance = sqrt(dx * dx + dz * dz);
            
            // Skip if outside brush radius
            if (distance > brushRadius) continue;
            
            // Calculate falloff based on distance - using quadratic falloff for smooth transition
            float normalizedDistance = distance / brushRadius;
            float falloff = (1.0f - normalizedDistance * normalizedDistance);
            
            // Get current height
            float currentHeight = GetHeight(x, z);
            
            // Calculate new height - raise height more at center
            float raiseAmount = height * falloff * brushStrength;
            float newHeight = currentHeight + raiseAmount;
            
            // Update height in heightmap
            m_heightMap[z * m_width + x] = newHeight;
        }
    }
    
    // Update min/max heights
    CalculateMinMaxHeights();
    
    // Update the mesh to reflect changes
    UpdateMesh();
}
