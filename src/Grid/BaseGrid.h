#pragma once

#include "../../include/Angel.h"
// #include "../Core/Texture.h" // Texture.h might not be needed directly by BaseGrid anymore
#include <vector>
#include <memory> // Still useful for m_gridMesh if it were a smart pointer, but it's raw now.
// #include <array> // No longer needed for m_terrainTextureLayers

// Forward declarations
class GridMesh;

// Removed: const int MAX_TERRAIN_TEXTURES = 4;

// Removed: Struct TerrainTextureLayer 
// struct TerrainTextureLayer {
// std::shared_ptr<Texture> texture = nullptr;
// float transitionHeight = 0.0f; 
// };

// Abstract interface for grid implementations
class BaseGrid {
public:
    BaseGrid();
    virtual ~BaseGrid();

    virtual void Init(int width, int depth, float worldScale, float textureScale);
    virtual void Render();
    
    // Accessors
    float GetWorldScale() const { return m_worldScale; }
    float GetTextureScale() const { return m_textureScale; }
    int GetWidth() const { return m_width; }
    int GetDepth() const { return m_depth; }
    
    // Pure virtual method to get height at a position
    virtual float GetHeight(int x, int z) const = 0;
    
    // Removed: Methods to set up terrain textures for GPU blending
    // void AddTerrainTextureLayer(const std::string& texturePath, float transitionHeight);
    // const std::array<TerrainTextureLayer, MAX_TERRAIN_TEXTURES>& GetTerrainTextureLayers() const { return m_terrainTextureLayers; }
    // int GetNumTerrainTextureLayers() const { return m_numTerrainTextureLayers; }

protected:
    // Grid dimensions
    int m_width = 0;
    int m_depth = 0;
    float m_worldScale = 1.0f;
    float m_textureScale = 1.0f;
    
    // Grid mesh for rendering
    GridMesh* m_gridMesh = nullptr; // If GridMesh also becomes more generic or is handled by derived classes, this might change.
    
    // Removed: Store multiple texture layers for GPU blending
    // std::array<TerrainTextureLayer, MAX_TERRAIN_TEXTURES> m_terrainTextureLayers;
    // int m_numTerrainTextureLayers = 0;
};