#pragma once

#include "BaseGrid.h"
#include <vector>
#include <functional>

// Terrain grid implementation with height mapping
class TerrainGrid : public BaseGrid {
public:
    // Type definition for height generation functions
    using HeightFunction = std::function<float(int x, int z)>;
    
    // Enum for terrain type
    enum class TerrainType {
        FLAT,
        CRATER
    };
    
    TerrainGrid();
    virtual ~TerrainGrid();
    
    // Override Init to create heightmap, with terrain type parameter
    virtual void Init(int width, int depth, float worldScale, float textureScale, 
                     TerrainType terrainType = TerrainType::FLAT,
                     float maxMountainHeight = 200.0f, float craterRadiusRatio = 0.4f);
    
    // Implementation of the pure virtual method from BaseGrid
    virtual float GetHeight(int x, int z) const override;
    
    // Set a custom height function
    void SetHeightFunction(HeightFunction func);
    
    // Load heightmap from file
    bool LoadHeightMap(const char* filename);
    
    // Save heightmap to file
    bool SaveHeightMap(const char* filename) const;
    
    // Set to flat terrain (convenience method)
    void SetFlat();
    
    // Set terrain to a crater shape with noise
    void SetCrater(float maxMountainHeight = 50.0f, float craterRadiusRatio = 0.4f);
    
private:
    // Generate heights using the current height function
    void GenerateHeights();
    
    // Default height generation function
    static float DefaultHeightFunc(int x, int z);
    
    // Heightmap data
    std::vector<float> m_heightMap;
    
    // Current height generation function
    HeightFunction m_heightFunc;
}; 