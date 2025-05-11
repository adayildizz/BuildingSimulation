#include "TerrainGrid.h"
#include "TerrainGenerator.h"
#include "GridMesh.h"
#include <fstream>
#include <cmath>
#include <cassert>
#include <iostream>
#include <random>     // Added for std::mt19937 and std::uniform_real_distribution
#include <algorithm>  // Added for std::min/max

TerrainGrid::TerrainGrid() : BaseGrid(), m_terrainType(TerrainType::FLAT), m_minHeight(0.0f), m_maxHeight(0.0f) 
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
