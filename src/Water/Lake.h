#ifndef LAKE_H
#define LAKE_H
#include <vector>
#include "Angel.h"
#include "Water.h"

class Lake {
    public:
    std::vector<vec3> gridPoints;
    std::vector<vec3> Points;
    float maxDepth;
    float waterLevel;
    Water waterSurface;

    void Init(std::vector<vec3> gridPoints, float maxDepth, float waterLevel, Shader* program) {
        this->gridPoints = gridPoints;
        this->maxDepth = maxDepth;
        this->waterLevel = waterLevel;
        this->waterSurface = Water(program);
    }


}
#endif