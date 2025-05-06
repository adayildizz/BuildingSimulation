#include "TerrainGrid.h"
#include "GridMesh.h"
#include <fstream>
#include <cmath>
#include <cassert>
#include <iostream>

TerrainGrid::TerrainGrid() : BaseGrid(), m_heightFunc(DefaultHeightFunc) 
{
}

TerrainGrid::~TerrainGrid() 
{
}

void TerrainGrid::Init(int width, int depth, float worldScale, float textureScale, 
                       TerrainType terrainType, float maxMountainHeight, float craterRadiusRatio)
{
    // First, prepare the heightmap data
    m_heightMap.resize(width * depth);
    
    // Store dimensions for the height function to use
    m_width = width;
    m_depth = depth;
    
    // Set terrain type based on parameter (this will generate heights)
    switch (terrainType) {
        case TerrainType::CRATER:
            SetCrater(maxMountainHeight, craterRadiusRatio);
            break;
        case TerrainType::FLAT:
        default:
            SetFlat();
            break;
    }
    
    // Now initialize the base grid with height data ready
    BaseGrid::Init(width, depth, worldScale, textureScale);
}

float TerrainGrid::GetHeight(int x, int z) const 
{
    // Check bounds
    if (x < 0 || x >= m_width || z < 0 || z >= m_depth) {
        return 0.0f;
    }
    
    // Get index in heightmap
    int index = z * m_width + x;
    
    // Return height
    return m_heightMap[index];
}

void TerrainGrid::SetHeightFunction(HeightFunction func)
{
    if (func) {
        m_heightFunc = func;
        
        // Regenerate heights with new function
        if (m_heightMap.size() > 0) {
            GenerateHeights();
        }
    }
}

void TerrainGrid::SetFlat()
{
    SetHeightFunction(DefaultHeightFunc);
}

void TerrainGrid::SetCrater(float maxMountainHeight, float craterRadiusRatio)
{
    SetHeightFunction([this, maxMountainHeight, craterRadiusRatio](int x, int z) -> float {
        float centerX = (m_width - 1) / 2.0f;
        float centerZ = (m_depth - 1) / 2.0f;

        float distFromCenter = std::sqrt(std::pow(x - centerX, 2) + std::pow(z - centerZ, 2));

        // Calculate the maximum possible distance from the center to a corner
        float maxDist = std::sqrt(std::pow(centerX, 2) + std::pow(centerZ, 2));
        float craterRadius = maxDist * craterRadiusRatio;

        float height = 0.0f;

        if (distFromCenter > craterRadius) {
            // Calculate falloff factor (0 at craterRadius, 1 at maxDist)
            float falloff = (distFromCenter - craterRadius) / (maxDist - craterRadius);
            // Clamp falloff just in case due to floating point inaccuracies
            falloff = std::max(0.0f, std::min(1.0f, falloff)); 
            falloff = std::pow(falloff, 2.0f); // Square falloff for steeper slopes

            // Simple noise (replace with Perlin/Simplex later for better results)
            // Seed rand() for different results each run (optional, add srand(time(0)) in main)
            float noise = (rand() % 1000) / 1000.0f; // Random value between 0 and 1

            height = falloff * noise * maxMountainHeight;
        }

        return height;
    });
}

bool TerrainGrid::LoadHeightMap(const char* filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open heightmap file: " << filename << std::endl;
        return false;
    }
    
    // Read dimensions
    int width, depth;
    file.read(reinterpret_cast<char*>(&width), sizeof(int));
    file.read(reinterpret_cast<char*>(&depth), sizeof(int));
    
    // Check if dimensions match
    if (width != m_width || depth != m_depth) {
        std::cerr << "Heightmap dimensions don't match grid dimensions" << std::endl;
        file.close();
        return false;
    }
    
    // Read heightmap data
    file.read(reinterpret_cast<char*>(m_heightMap.data()), sizeof(float) * m_heightMap.size());
    
    file.close();
    return true;
}

bool TerrainGrid::SaveHeightMap(const char* filename) const
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create heightmap file: " << filename << std::endl;
        return false;
    }
    
    // Write dimensions
    file.write(reinterpret_cast<const char*>(&m_width), sizeof(int));
    file.write(reinterpret_cast<const char*>(&m_depth), sizeof(int));
    
    // Write heightmap data
    file.write(reinterpret_cast<const char*>(m_heightMap.data()), sizeof(float) * m_heightMap.size());
    
    file.close();
    return true;
}

void TerrainGrid::GenerateHeights()
{
    // Generate height for each vertex
    for (int z = 0; z < m_depth; z++) {
        for (int x = 0; x < m_width; x++) {
            int index = z * m_width + x;
            m_heightMap[index] = m_heightFunc(x, z);
        }
    }
}

float TerrainGrid::DefaultHeightFunc(int x, int z)
{
    // Flat grid: constant height of 0
    return 0.0f;
}
