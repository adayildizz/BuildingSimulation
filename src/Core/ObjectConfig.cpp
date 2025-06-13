#include "ObjectConfig.h"

// Function to parse int vector from string like "{0}" or "{}"
std::vector<unsigned int> parseIntVector(const std::string& str) {
    std::vector<unsigned int> result;
    if (str == "{}") {
        return result; // Empty vector
    }
    // Remove braces and parse numbers
    std::string cleaned = str.substr(1, str.length() - 2); // Remove { and }
    if (!cleaned.empty()) {
        std::stringstream ss(cleaned);
        std::string item;
        while (std::getline(ss, item, ',')) {
            result.push_back(std::stoi(item));
        }
    }
    return result;
}

std::string extractDisplayName(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string filename = filepath.substr(lastSlash + 1);
        // Remove .obj extension
        size_t lastDot = filename.find_last_of('.');
        if (lastDot != std::string::npos) {
            filename = filename.substr(0, lastDot);
        }
        for (char& c : filename) {
            if (c == '_') c = ' ';
        }
        if (!filename.empty()) {
            filename[0] = std::toupper(filename[0]);
        }
        return filename;
    }
    return "Unknown";
}

// Function to load object configurations from file
std::vector<ObjectConfig> loadObjectConfigs(const std::string& configFile) {
    std::vector<ObjectConfig> configs;
    std::ifstream file(configFile);
    
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open object configuration file: " << configFile << std::endl;
        return configs;
    }
    
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
        
        std::stringstream ss(line);
        std::string token;
        ObjectConfig config;
        
        // Parse comma-separated values
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 7) {
            try {
                config.filepath = tokens[0];
                config.intVector = parseIntVector(tokens[1]);
                config.scale = std::stof(tokens[2]);
                config.rotX = std::stof(tokens[3]);
                config.rotY = std::stof(tokens[4]);
                config.rotZ = std::stof(tokens[5]);
                config.iconPath = tokens[6];
                config.displayName = extractDisplayName(config.filepath);
                configs.push_back(config);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing line " << lineNumber << " in " << configFile << ": " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Warning: Line " << lineNumber << " in " << configFile << " has insufficient data (expected 7 fields, got " << tokens.size() << ")" << std::endl;
        }
    }
    
    std::cout << "Loaded " << configs.size() << " object configurations from " << configFile << std::endl;
    return configs;
}
