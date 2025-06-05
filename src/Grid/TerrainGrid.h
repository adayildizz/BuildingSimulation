#pragma once

#include "BaseGrid.h"
#include "TerrainGenerator.h"
#include <vector>

// Terrain grid implementation with height mapping
class TerrainGrid : public BaseGrid {
public:
    // Use TerrainGenerator's TerrainType and TerrainLayerInfo
    using TerrainType = TerrainGenerator::TerrainType;
    using TerrainLayerInfo = TerrainGenerator::TerrainLayerInfo;
    
    TerrainGrid();
    virtual ~TerrainGrid();
    
    // Init will now take parameters for the TerrainGenerator
    virtual void Init(int width, int depth, float worldScale, float textureScale, 
                     TerrainType terrainType = TerrainType::FLAT,
                     float genParam1 = 200.0f, // Generic parameter 1 for generator (e.g. max height)
                     float genParam2 = 0.2f,   // Generic parameter 2 for generator (e.g. radius ratio)
                     int genIterations = 100,   // Generic iterations for generator
                     float genFilterFactor = 0.5f, // Generic filter factor
                     float genFaultDisplacementScale = 0.05f); // Generic fault displacement scale
    
    // Implementation of the pure virtual method from BaseGrid
    virtual float GetHeight(int x, int z) const override;

    // New methods for terrain modification
    void SetHeight(int x, int z, float height);
    void UpdateMesh();
    void createLake(int centerX, int centerZ, float radius, float depth, float smoothness = 0.5f);
    
    // Get heights around the edge of a lake
    std::vector<float> getLakeEdgeHeights(int centerX, int centerZ, float radius, int numSamples = 36);

    // Getters for terrain properties
    TerrainType GetTerrainType() const;
    const TerrainLayerInfo& GetLayerInfo() const;
    float GetMinHeight() const; // Will need to calculate this
    float GetMaxHeight() const; // Will need to calculate this
    
private:
    // Heightmap data
    std::vector<float> m_heightMap;
    TerrainType m_terrainType;
    TerrainLayerInfo m_layerInfo;
    float m_minHeight;
    float m_maxHeight;

    void CalculateMinMaxHeights(); // Helper to calculate and store min/max
}; 