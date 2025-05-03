#include "BaseGrid.h"
#include <cassert>

// Add constructor and destructor implementations
BaseGrid::BaseGrid() {}

BaseGrid::~BaseGrid() {}

void BaseGrid::Init(int width, int depth, float worldScale)
{
    m_width = width;
    m_depth = depth;
    m_worldScale = worldScale;

    // LoadHeightMap("heightmap.save");

    m_flatGrid.CreateFlatGrid(width, depth, this);
}

void BaseGrid::Render()
{
    m_flatGrid.Render();
}

void BaseGrid::LoadHeightMap(const char* filename)
{
    // Load the height map from the file
}