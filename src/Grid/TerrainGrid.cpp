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

void TerrainGrid::createLake(int centerX, int centerZ, float radius, float depth, float smoothness) {
    if (radius <= 0 || depth <= 0) return;

    // Random number generator for natural variation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> radiusVar(-radius * 0.2f, radius * 0.2f);
    std::uniform_real_distribution<float> angleVar(-0.2f, 0.2f);

    // Create multiple overlapping ellipses for natural shape
    struct Ellipse {
        float a, b;  // Major and minor axes
        float rotation;  // Rotation in radians
        float x, z;  // Center offset
        float influence;  // How much this feature influences the shape
    };

    std::vector<Ellipse> ellipses;
    
    // Main lake shape
    float mainRadius = radius + radiusVar(gen);
    float mainEccentricity = 0.7f + (gen() % 100) / 200.0f;  // Random between 0.7 and 1.2
    float mainRotation = (gen() % 360) * M_PI / 180.0f;  // Random rotation
    ellipses.push_back({mainRadius, mainRadius * mainEccentricity, mainRotation, 0.0f, 0.0f, 1.0f});

    // Add random outgrowths and indentations
    int numFeatures = 2 + (gen() % 3);  // 2-4 features
    for (int i = 0; i < numFeatures; i++) {
        float angle = (gen() % 360) * M_PI / 180.0f + angleVar(gen);
        float distance = radius * (0.4f + (gen() % 100) / 200.0f);  // 0.4 to 0.9 of radius
        float featureSize = radius * (0.25f + (gen() % 100) / 300.0f);  // 0.25 to 0.58 of radius
        float featureEccentricity = 0.5f + (gen() % 100) / 150.0f;  // 0.5 to 1.17
        float featureRotation = (gen() % 360) * M_PI / 180.0f;
        float influence = 0.8f + (gen() % 100) / 200.0f;  // 0.8 to 1.3 influence

        ellipses.push_back({
            featureSize,
            featureSize * featureEccentricity,
            featureRotation,
            distance * cos(angle),
            distance * sin(angle),
            influence
        });
    }

    // Calculate the maximum radius to ensure we cover all features
    float maxRadius = radius * 1.5f;
    int minX = std::max(0, centerX - static_cast<int>(maxRadius));
    int maxX = std::min(m_width - 1, centerX + static_cast<int>(maxRadius));
    int minZ = std::max(0, centerZ - static_cast<int>(maxRadius));
    int maxZ = std::min(m_depth - 1, centerZ + static_cast<int>(maxRadius));

    // Modify terrain in the affected region
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            float dx = x - centerX;
            float dz = z - centerZ;
            float distSquared = dx * dx + dz * dz;
            
            if (distSquared <= maxRadius * maxRadius) {
                // Calculate the combined shape
                float finalRadius = 0.0f;
                bool isInside = false;

                for (const auto& ellipse : ellipses) {
                    // Transform point to ellipse space
                    float dx = x - (centerX + ellipse.x);
                    float dz = z - (centerZ + ellipse.z);
                    float rotX = dx * cos(ellipse.rotation) - dz * sin(ellipse.rotation);
                    float rotZ = dx * sin(ellipse.rotation) + dz * cos(ellipse.rotation);
                    
                    // Calculate distance from ellipse center
                    float dist = sqrt(rotX * rotX + rotZ * rotZ);
                    float angle = atan2(rotZ, rotX);
                    
                    // Calculate radius at this angle
                    float r = (ellipse.a * ellipse.b) / 
                             sqrt(ellipse.b * ellipse.b * cos(angle) * cos(angle) + 
                                  ellipse.a * ellipse.a * sin(angle) * sin(angle));

                    if (ellipse.x == 0.0f && ellipse.z == 0.0f) {
                        isInside = dist < r;
                    }

                    if (dist < r * 1.2f) {
                        float blend = 1.0f - (dist / (r * 1.2f)) * (dist / (r * 1.2f));
                        finalRadius = std::max(finalRadius, r * (1.0f + blend * 0.8f * ellipse.influence));
                    }
                }

                if (isInside) {
                    // Get current height
                    float currentHeight = GetHeight(x, z);
                    
                    // Calculate new height using smooth falloff
                    float dist = sqrt(dx * dx + dz * dz);
                    float falloff = 1.0f - (dist / finalRadius);
                    falloff = std::pow(falloff, smoothness);  // Adjust smoothness of the transition
                    
                    // Set new height
                    float newHeight = currentHeight - depth * falloff;
                    SetHeight(x, z, newHeight);
                }
            }
        }
    }

    // Update the mesh after all modifications
    UpdateMesh();
}

void TerrainGrid::SetHeight(int x, int z, float height) {
    // Check bounds
    if (x < 0 || x >= m_width || z < 0 || z >= m_depth) {
        return;
    }
    
    // Update height in heightmap
    size_t index = static_cast<size_t>(z) * m_width + x;
    if (index < m_heightMap.size()) {
        m_heightMap[index] = height;
        
        // Update min/max heights if needed
        if (height < m_minHeight) m_minHeight = height;
        if (height > m_maxHeight) m_maxHeight = height;
    }
}

void TerrainGrid::UpdateMesh() {
    if (m_gridMesh) {
        // Recreate the mesh with updated heights
        m_gridMesh->CreateMesh(m_width, m_depth, this);
    }
}

std::vector<float> TerrainGrid::getLakeEdgeHeights(int centerX, int centerZ, float radius, int numSamples) {
    std::vector<float> edgeHeights;
    edgeHeights.reserve(numSamples);

    // Sample heights at regular intervals around the lake's edge
    for (int i = 0; i < numSamples; i++) {
        // Calculate angle for this sample point
        float angle = (2.0f * M_PI * i) / numSamples;
        
        // Calculate position on the edge
        int x = static_cast<int>(centerX + radius * cos(angle));
        int z = static_cast<int>(centerZ + radius * sin(angle));
        
        // Get the height at this point
        float height = GetHeight(x, z);
        edgeHeights.push_back(height);
    }

    return edgeHeights;
}
