#include "BaseGrid.h"
#include "GridMesh.h"
#include <cassert>
#include <iostream>

// Constructor and destructor implementations
BaseGrid::BaseGrid() : m_gridMesh(nullptr) {}

BaseGrid::~BaseGrid() {
    if (m_gridMesh) {
        delete m_gridMesh;
        m_gridMesh = nullptr;
    }
}

void BaseGrid::Init(int width, int depth, float worldScale, float textureScale)
{
    m_width = width;
    m_depth = depth;
    m_worldScale = worldScale;
    m_textureScale = textureScale;
    
    // Create a new mesh if needed
    if (!m_gridMesh) {
        m_gridMesh = new GridMesh();
    }

    // Initialize the mesh
    // The GridMesh will call back to the virtual GetHeight() of the derived class.
    m_gridMesh->CreateMesh(width, depth, this);
}

void BaseGrid::Render()
{
    if (m_gridMesh) {
        // The responsibility of binding textures and setting shader uniforms 
        // for terrain blending now lies outside BaseGrid.
        // This Render function simply draws the mesh.
        m_gridMesh->Render();
    }
}
