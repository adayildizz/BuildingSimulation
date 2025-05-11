#include "TerrainGenerator.h"
#include <cmath>
#include <random>
#include <vector>
#include <algorithm> // For std::min/max, std::fill
#include <numeric>   // For std::iota (if needed later)
#include <iostream>  // For debugging, remove in production

// Constructor
TerrainGenerator::TerrainGenerator(int width, int depth) : m_width(width), m_depth(depth) {
    if (m_width <= 0 || m_depth <= 0) {
        // Potentially throw an error or log, for now, ensure they are at least 1
        m_width = std::max(1, m_width);
        m_depth = std::max(1, m_depth);
    }
}

// Destructor
TerrainGenerator::~TerrainGenerator() {}

// Main dispatcher function
std::vector<float> TerrainGenerator::GenerateHeightmap(TerrainType type, float param1, float param2, 
                                                       int iterations, float filterFactor, float faultDisplacementScale) {
    switch (type) {
        case TerrainType::FLAT:
            return GenerateFlatTerrain();
        case TerrainType::CRATER:
            return GenerateCraterTerrain(param1, param2); // param1=maxHeight, param2=radiusRatio
        case TerrainType::FAULT_FORMATION:
            return GenerateFaultFormationTerrain(param1, iterations, filterFactor); // param1=maxHeight
        case TerrainType::VOLCANIC_CALDERA:
            return GenerateVolcanicCalderaTerrain(param1, param2, iterations, faultDisplacementScale, filterFactor); // param1=maxEdgeHeight, param2=centralFlatRadiusRatio
        default:
            // Fallback to flat terrain or throw an error
            std::cerr << "Unknown terrain type, defaulting to FLAT." << std::endl;
            return GenerateFlatTerrain();
    }
}

// Flat terrain
std::vector<float> TerrainGenerator::GenerateFlatTerrain() {
    std::vector<float> heightMap(m_width * m_depth, 0.0f);
    return heightMap;
}

// Crater terrain
std::vector<float> TerrainGenerator::GenerateCraterTerrain(float maxMountainHeight, float craterRadiusRatio) {
    std::vector<float> heightMap(m_width * m_depth);
    float centerX = (m_width - 1) / 2.0f;
    float centerZ = (m_depth - 1) / 2.0f;
    float maxDist = std::sqrt(std::pow(centerX, 2) + std::pow(centerZ, 2));
    float craterRadius = maxDist * craterRadiusRatio;

    // Use a consistent seed for reproducibility during a single generation pass if desired
    // std::mt19937 rng(some_fixed_seed); 
    // std::uniform_real_distribution<float> noise_dist(0.0f, 1.0f);

    for (int z = 0; z < m_depth; ++z) {
        for (int x = 0; x < m_width; ++x) {
            float distFromCenter = std::sqrt(std::pow(x - centerX, 2) + std::pow(z - centerZ, 2));
            float height = 0.0f;

            if (distFromCenter > craterRadius) {
                float falloff = (distFromCenter - craterRadius) / (maxDist - craterRadius);
                falloff = std::max(0.0f, std::min(1.0f, falloff));
                falloff = std::pow(falloff, 2.0f);
                
                // Simple noise, consider replacing with Perlin/Simplex for better results
                // float noise = noise_dist(rng); 
                float noise = (rand() % 1000) / 1000.0f; // Original simple noise
                height = falloff * noise * maxMountainHeight;
            }
            heightMap[z * m_width + x] = height;
        }
    }
    return heightMap;
}


// Fault formation terrain generation
std::vector<float> TerrainGenerator::GenerateFaultFormationTerrain(float maxHeight, int iterations, float filterFactor) {
    std::vector<float> heightMap(m_width * m_depth, 0.0f);

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> distHeight(-maxHeight * 0.05f, maxHeight * 0.05f);

    for (int i = 0; i < iterations; ++i) {
        float p1x = dist01(rng) * m_width;
        float p1z = dist01(rng) * m_depth;
        float p2x = dist01(rng) * m_width;
        float p2z = dist01(rng) * m_depth;
        float displacement = distHeight(rng);

        for (int z = 0; z < m_depth; ++z) {
            for (int x = 0; x < m_width; ++x) {
                float side = (p2x - p1x) * (z - p1z) - (p2z - p1z) * (x - p1x);
                if (side > 0) {
                    heightMap[z * m_width + x] += displacement;
                } else {
                    heightMap[z * m_width + x] -= displacement;
                }
            }
        }
    }

    NormalizeHeightmap(heightMap, maxHeight);
    if (filterFactor > 0.0f && filterFactor < 1.0f && m_width > 2 && m_depth > 2) {
        SmoothHeightmap(heightMap, filterFactor);
    }
    return heightMap;
}

// Volcanic caldera terrain generation
std::vector<float> TerrainGenerator::GenerateVolcanicCalderaTerrain(float maxEdgeHeight, float centralFlatRadiusRatio, 
                                                                  int faultIterations, float faultDisplacementScale, float filterFactor) {
    std::vector<float> heightMap(m_width * m_depth, 0.0f);
    std::vector<float> rawFaultMap(m_width * m_depth, 0.0f);

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> distFaultHeight(-maxEdgeHeight * faultDisplacementScale, maxEdgeHeight * faultDisplacementScale);

    // Step 1: Generate raw fault map
    for (int i = 0; i < faultIterations; ++i) {
        float p1x = dist01(rng) * m_width;
        float p1z = dist01(rng) * m_depth;
        float p2x = dist01(rng) * m_width;
        float p2z = dist01(rng) * m_depth;
        float displacement = distFaultHeight(rng);

        for (int z_coord = 0; z_coord < m_depth; ++z_coord) {
            for (int x_coord = 0; x_coord < m_width; ++x_coord) {
                float side = (p2x - p1x) * (z_coord - p1z) - (p2z - p1z) * (x_coord - p1x);
                rawFaultMap[z_coord * m_width + x_coord] += (side > 0) ? displacement : -displacement;
            }
        }
    }
    
    // Normalize rawFaultMap to [0, 1]
    float minRawH = rawFaultMap[0];
    float maxRawH = rawFaultMap[0];
    for(size_t i = 0; i < rawFaultMap.size(); ++i) {
        if (rawFaultMap[i] < minRawH) minRawH = rawFaultMap[i];
        if (rawFaultMap[i] > maxRawH) maxRawH = rawFaultMap[i];
    }

    if (maxRawH > minRawH) {
        for (float& h : rawFaultMap) {
            h = (h - minRawH) / (maxRawH - minRawH);
        }
    } else {
        std::fill(rawFaultMap.begin(), rawFaultMap.end(), 0.5f);
    }

    // Step 2: Apply distance-based profile
    float centerX = (m_width - 1) / 2.0f;
    float centerZ = (m_depth - 1) / 2.0f;
    float flatRadius = std::min(centerX, centerZ) * centralFlatRadiusRatio;
    float maxDistToCorner = std::sqrt(centerX * centerX + centerZ * centerZ);
    if (maxDistToCorner <= flatRadius) maxDistToCorner = flatRadius + 1.0f; // Avoid division by zero if flatRadius is too large

    for (int z_coord = 0; z_coord < m_depth; ++z_coord) {
        for (int x_coord = 0; x_coord < m_width; ++x_coord) {
            float distFromCenter = std::sqrt(std::pow(x_coord - centerX, 2) + std::pow(z_coord - centerZ, 2));
            float profileScale = 0.0f;

            if (distFromCenter <= flatRadius) {
                profileScale = 0.0f;
            } else if (distFromCenter < maxDistToCorner) { // Use < to ensure ramp up to edge
                profileScale = (distFromCenter - flatRadius) / (maxDistToCorner - flatRadius);
                 profileScale = std::max(0.0f, std::min(1.0f, profileScale));
            } else {
                profileScale = 1.0f;
            }
            heightMap[z_coord * m_width + x_coord] = rawFaultMap[z_coord * m_width + x_coord] * profileScale;
        }
    }

    // Step 3: Normalize final heightMap to [0, maxEdgeHeight]
    NormalizeHeightmap(heightMap, maxEdgeHeight);
    
    // Re-ensure center is flat (important if normalization shifted values)
    if (flatRadius > 0) { // Only if there is a flat center
        for (int z_coord = 0; z_coord < m_depth; ++z_coord) {
            for (int x_coord = 0; x_coord < m_width; ++x_coord) {
                float distFromCenter = std::sqrt(std::pow(x_coord - centerX, 2) + std::pow(z_coord - centerZ, 2));
                if (distFromCenter <= flatRadius) {
                     // Find the minimum height in the non-flat region to set the flat region correctly
                     // Or simply set to 0 if the caldera floor should be at the base.
                     // For simplicity now, let's assume it should be at the normalized map's minimum (which became 0).
                     heightMap[z_coord * m_width + x_coord] = 0.0f;
                }
            }
        }
        // Re-Normalize again if setting the center to 0 changed min/max significantly
        // This can get complex. A better approach might be to normalize the ramp part separately.
        // For now, we rely on the initial rawFaultMap[0,1] * profileScale[0,1] giving [0,1], then scaling.
    }


    // Step 4: Apply smoothing filter
    if (filterFactor > 0.0f && filterFactor < 1.0f && m_width > 2 && m_depth > 2) {
        SmoothHeightmap(heightMap, filterFactor);
    }

    return heightMap;
}

// New method implementation
TerrainGenerator::TerrainLayerInfo TerrainGenerator::GetLayerInfo(TerrainType type) const {
    TerrainLayerInfo info;
    // Default values, can be customized per terrain type
    // These are the values from the user's main.cpp snippet
    info.layer1_percentage = 0.25f; // Low grass/dirt
    info.layer2_percentage = 0.50f; // Rocky slopes
    info.layer3_percentage = 0.75f; // Higher rocky/snowy slopes

    switch (type) {
        case TerrainType::FLAT:
            // Flat terrain might have very different layering, e.g., mostly one type
            info.layer1_percentage = 0.0f; // Example: mostly dirt/grass
            info.layer2_percentage = 0.0f; // Almost no rock
            info.layer3_percentage = 0.05f; // Almost no snow
            break;
        case TerrainType::CRATER:
            // Craters might have more rock exposed
            info.layer1_percentage = 0.05f; // Less low grass/dirt
            info.layer2_percentage = 0.25f; // More rocky slopes
            info.layer3_percentage = 0.50f; // Potentially some higher variation
            break;
        case TerrainType::FAULT_FORMATION:
            // Faults could expose a mix, similar to default or more dramatic
            info.layer1_percentage = 0.15f;
            info.layer2_percentage = 0.30f;
            info.layer3_percentage = 0.75f;
            break;
        case TerrainType::VOLCANIC_CALDERA:
            // Volcanic areas: more ash, rock, possibly snow at high peaks
            info.layer1_percentage = 0.10f; // Ash/barren low areas
            info.layer2_percentage = 0.25f; // Extensive rocky/cooled lava slopes
            info.layer3_percentage = 0.50f; // Higher chance of snow on caldera rim
            break;
        // Add cases for other terrain types as they are defined
        default:
            // Uses the default initialized values above
            break;
    }
    return info;
}

// Helper: Normalize heightmap to [0, targetMaxHeight]
void TerrainGenerator::NormalizeHeightmap(std::vector<float>& heightMap, float targetMaxHeight) {
    if (heightMap.empty()) return;

    float minH = heightMap[0];
    float maxH = heightMap[0];
    for (float h : heightMap) {
        if (h < minH) minH = h;
        if (h > maxH) maxH = h;
    }

    if (maxH > minH) {
        for (float& h : heightMap) {
            h = targetMaxHeight * (h - minH) / (maxH - minH);
        }
    } else { // All heights are the same
        // If all same, set to targetMaxHeight / 2 or 0 depending on context
        // For terrain, usually means a flat plane, could be at 0 or mid-height.
        // Here, if maxH == minH, then (h - minH) is 0, so all h become 0.
        // If we want them at targetMaxHeight, they should be set to that.
        // If we want them at targetMaxHeight/2 for a mid-plane:
        // std::fill(heightMap.begin(), heightMap.end(), targetMaxHeight / 2.0f);
        // If the desired output is 0 to targetMaxHeight, and all are same, they effectively map to 0.
        // If minH was positive, they'd map to 0. If we want to preserve their original single value scaled:
        // std::fill(heightMap.begin(), heightMap.end(), (minH / maxH) * targetMaxHeight); // if maxH != 0
        // The current logic makes a flat terrain (where minH=maxH) become all 0s, unless targetMaxHeight is 0.
        // If targetMaxHeight is not 0, and minH=maxH, then all h are set to 0.
        // This is usually fine for terrain generation starting from faults.
        // If we want a flat plane at a specific height, that should be handled by GenerateFlatTerrain(targetHeight).
        // For now, if all heights are equal, this makes them all 0.
        // To make them all targetMaxHeight if they were non-zero, or all 0 if they were zero:
        float valToSet = (minH == 0.0f && maxH == 0.0f) ? 0.0f : targetMaxHeight;
        if (minH == maxH) { // Check again to be sure
             std::fill(heightMap.begin(), heightMap.end(), valToSet);
        }
        // The original code logic implied: if maxH > minH, normalize. Else (maxH <= minH, i.e. maxH == minH)
        // fill with targetMaxHeight / 2.0f. Let's stick to that for less surprise.
        // Actually, the original fault formation code was:
        // else { std::fill(m_heightMap.begin(), m_heightMap.end(), maxHeight / 2.0f); }
        // So, let's ensure this behavior:
        if (!(maxH > minH)) { // If maxH == minH
             std::fill(heightMap.begin(), heightMap.end(), targetMaxHeight / 2.0f);
        }


    }
}

// Helper: Smooth heightmap using a simple box filter
void TerrainGenerator::SmoothHeightmap(std::vector<float>& heightMap, float filterFactor) {
    if (heightMap.empty() || m_width <= 2 || m_depth <= 2 || filterFactor <= 0.0f || filterFactor >= 1.0f) {
        return;
    }

    std::vector<float> smoothedHeightMap = heightMap; // Work on a copy
    for (int z = 1; z < m_depth - 1; ++z) {
        for (int x = 1; x < m_width - 1; ++x) {
            float sum = 0;
            sum += heightMap[(z - 1) * m_width + (x - 1)]; sum += heightMap[(z - 1) * m_width + x]; sum += heightMap[(z - 1) * m_width + (x + 1)];
            sum += heightMap[z * m_width + (x - 1)];       sum += heightMap[z * m_width + x];       sum += heightMap[z * m_width + (x + 1)];
            sum += heightMap[(z + 1) * m_width + (x - 1)]; sum += heightMap[(z + 1) * m_width + x]; sum += heightMap[(z + 1) * m_width + (x + 1)];
            
            smoothedHeightMap[z * m_width + x] = heightMap[z * m_width + x] * (1.0f - filterFactor) + (sum / 9.0f) * filterFactor;
        }
    }
    heightMap = smoothedHeightMap; // Copy back
}

// DefaultHeightFunc - not strictly needed if GenerateFlatTerrain fills with 0s
// but kept for conceptual completeness or if a lambda-style approach was to be revived.
float TerrainGenerator::DefaultHeightFunc(int x, int z) {
    return 0.0f;
} 