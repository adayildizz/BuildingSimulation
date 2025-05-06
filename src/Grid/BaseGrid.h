#pragma once

#include "../../include/Angel.h"
#include <vector>

// Forward declarations
class GridMesh;

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

protected:
    // Grid dimensions
    int m_width = 0;
    int m_depth = 0;
    float m_worldScale = 1.0f;
    float m_textureScale = 1.0f;
    
    // Grid mesh for rendering
    GridMesh* m_gridMesh = nullptr;
};