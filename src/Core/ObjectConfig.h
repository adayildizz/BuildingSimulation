#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// Structure to hold object configuration data
struct ObjectConfig {
    std::string filepath;
    std::vector<unsigned int> intVector;
    float scale;
    float rotX;
    float rotY;
    float rotZ;
    std::string iconPath;
    std::string displayName; // Will be derived from filename
};

// Function to parse int vector from string like "{0}" or "{}"
std::vector<unsigned int> parseIntVector(const std::string& str);

// Function to extract display name from filepath
std::string extractDisplayName(const std::string& filepath);

// Function to load object configurations from file
std::vector<ObjectConfig> loadObjectConfigs(const std::string& configFile);
