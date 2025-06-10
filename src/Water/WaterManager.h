#pragma once

#include "Water.h"
#include "Core/Shader.h"
#include "Core/Camera.h"
#include <memory>
#include <vector>
#include <functional>

class WaterManager {
public:
    WaterManager(std::shared_ptr<Shader> waterShader, int screenWidth, int screenHeight);

    void addWaterAt(const vec3& position, float scale);
    void renderAll(const mat4& viewProjMatrix, Camera* camera, Shader* worldShader,
                  std::function<void(vec4 clipPlane, mat4 viewProjMatrix)> renderSceneFunc);
    void CheckGLError(const std::string& location);
private:
    struct WaterInstance {
        std::unique_ptr<Water> water;
        vec3 position;
        float scale;
    };

    std::vector<WaterInstance> waters;
    std::shared_ptr<Shader> waterShader;
    int screenWidth, screenHeight;
    float time = 0.0f;
};
