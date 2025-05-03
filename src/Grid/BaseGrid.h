#pragma once

#include "../../include/Angel.h"
#include "../Core/Shader.h"
#include "../Core/Camera.h"
#include "FlatGrid.h"
#include <vector>

class BaseGrid {
public:
    BaseGrid();
    ~BaseGrid();

    void Init(int width, int depth, float worldScale);
    void Render();    
    float GetWorldScale() const { return m_worldScale; }
    
private:
    void LoadHeightMap(const char* filename);

    // Grid dimensions
    int m_width = 0;
    int m_depth = 0;
    float m_worldScale = 1.0f;
    FlatGrid m_flatGrid;
};