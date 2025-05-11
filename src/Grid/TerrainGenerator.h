#pragma once

#include <vector>
#include <functional> // For std::function if we decide to keep a similar pattern

// Forward declaration if TerrainGrid needs to be known (e.g. for friend class or parameters)
// class TerrainGrid; // Not strictly needed if it only returns a heightmap

class TerrainGenerator {
public:
    // Enum for terrain type, can be moved here or shared
    enum class TerrainType {
        FLAT,
        CRATER,
        FAULT_FORMATION,
        VOLCANIC_CALDERA
        // Potentially add more types here later e.g. ROLLING_HILLS, MOUNTAINOUS
    };

    struct TerrainLayerInfo {
        float layer1_percentage; // e.g., 0.05f for Low grass/dirt
        float layer2_percentage; // e.g., 0.10f for Rocky slopes
        float layer3_percentage; // e.g., 0.20f for Higher rocky/snowy slopes
        // Add more layers or other material properties as needed
    };

    TerrainGenerator(int width, int depth);
    ~TerrainGenerator();

    // Generates and returns a heightmap based on the specified type
    std::vector<float> GenerateHeightmap(TerrainType type, 
                                         float param1, // e.g., maxMountainHeight or maxEdgeHeight
                                         float param2, // e.g., craterRadiusRatio or centralFlatRadiusRatio
                                         int iterations = 100, // For fault-based generations
                                         float filterFactor = 0.5f, // For smoothing
                                         float faultDisplacementScale = 0.05f); // For caldera specific faulting

    // New method to get layer information based on terrain type
    TerrainLayerInfo GetLayerInfo(TerrainType type) const;

    // Individual generation methods, could be public or private
    // If public, TerrainGrid could call them directly if it knew parameters.
    // If private, GenerateHeightmap would be the sole interface.
// private: // Making them public for now for flexibility, can be refactored to private later
    std::vector<float> GenerateFlatTerrain();
    std::vector<float> GenerateCraterTerrain(float maxMountainHeight, float craterRadiusRatio);
    std::vector<float> GenerateFaultFormationTerrain(float maxHeight, int iterations, float filterFactor);
    std::vector<float> GenerateVolcanicCalderaTerrain(float maxEdgeHeight, float centralFlatRadiusRatio, 
                                                  int faultIterations, float faultDisplacementScale, float filterFactor);

private:
    int m_width;
    int m_depth;

    // Helper for fault formation logic (if needed internally and shared)
    void ApplyFaults(std::vector<float>& heightMap, float displacementRange, int iterations);
    // Helper for normalization
    void NormalizeHeightmap(std::vector<float>& heightMap, float targetMaxHeight);
    // Helper for smoothing
    void SmoothHeightmap(std::vector<float>& heightMap, float filterFactor);

    // Default height function if needed for flat or initial state
    static float DefaultHeightFunc(int x, int z); // May not be needed if GenerateFlatTerrain just fills with 0
}; 