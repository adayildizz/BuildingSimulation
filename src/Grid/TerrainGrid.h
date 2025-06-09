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

    // Get height at world coordinates with interpolation
    float GetHeightAtWorldPos(float worldX, float worldZ) const;

    // Getters for terrain properties
    TerrainType GetTerrainType() const;
    const TerrainLayerInfo& GetLayerInfo() const;
    float GetMinHeight() const; // Will need to calculate this
    float GetMaxHeight() const; // Will need to calculate this
    
    // Texture painting methods
    void PaintTexture(float worldX, float worldZ, int textureLayer, float brushRadius, float brushStrength);
    
    std::vector<std::pair<int, int>> Flatten(float worldX, float worldZ, float brushRadius, float brushStrength); // New flatten function
    std::vector<vec3> Dig(float worldX, float worldZ, float brushRadius, float brushStrength); // Modified to return dug points
    void RaiseTerrain(float worldX, float worldZ, float height, float brushRadius, float brushStrength); // New function to raise terrain
    void ResetFlatteningState(); // Reset the flattening state for new operations
    void UpdateMesh(); // Force mesh update after painting
    
private:
    // Heightmap data
    std::vector<float> m_heightMap;
    TerrainType m_terrainType;
    TerrainLayerInfo m_layerInfo;
    float m_minHeight;
    float m_maxHeight;

    // Flattening state
    float m_flattenTargetHeight;
    bool m_isFirstFlattenClick;
    std::vector<std::pair<int, int>> m_lastFlattenedPoints; 

    void CalculateMinMaxHeights(); // Helper to calculate and store min/max
    void NormalizeSplatWeights(int x, int z); // Helper to normalize weights after painting
};