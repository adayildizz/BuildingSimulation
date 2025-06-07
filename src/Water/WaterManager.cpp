#include "WaterManager.h"

WaterManager::WaterManager(std::shared_ptr<Shader> waterShader, int screenWidth, int screenHeight)
    : waterShader(waterShader), screenWidth(screenWidth), screenHeight(screenHeight) {}

void WaterManager::addWaterAt(const vec3& position, float scale) {
   std::cout << "Adding new water instance at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

    CheckGLError("Start of addWaterAt"); // New: Check context immediately
    auto newWater = std::make_unique<Water>(waterShader.get());
    CheckGLError("After new Water() creation"); // New: Check context after object creation

    newWater->dudvTexture = newWater->loadTexture("resources/textures/dudvMap.jpg");
    CheckGLError("After loading dudvTexture in addWaterAt"); // This is where 502 appears

    newWater->generateMesh();
    CheckGLError("After generateMesh in addWaterAt"); // New
    newWater->createWaterMeshVAO();
    CheckGLError("After createWaterMeshVAO in addWaterAt"); // New

    std::cout << "Creating reflection FBO for new water instance..." << std::endl;
    newWater->reflectionFBO = newWater->createFBO(newWater->reflectionTexture, screenWidth, screenHeight);
    std::cout << "Reflection FBO created: " << newWater->reflectionFBO << std::endl;
    CheckGLError("After reflection FBO creation in addWaterAt"); // New

    std::cout << "Creating refraction FBO for new water instance..." << std::endl;
    newWater->refractionFBO = newWater->createFBO(newWater->refractionTexture, screenWidth, screenHeight);
    std::cout << "Refraction FBO created: " << newWater->refractionFBO << std::endl;
    CheckGLError("After refraction FBO creation in addWaterAt"); // New

    waters.push_back({ std::move(newWater), position, scale });
    std::cout << "Total water instances: " << waters.size() << std::endl;
    CheckGLError("End of addWaterAt"); // New
}
// In main.cpp or a suitable utility header
void WaterManager::CheckGLError(const std::string& location) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error at " << location << ": " << std::hex << error << std::dec << std::endl;
        // You can add more detailed error string mappings if you want, e.g.:
        // if (error == GL_INVALID_ENUM) std::cerr << "GL_INVALID_ENUM" << std::endl;
        // ... etc.
    }
}

void WaterManager::renderAll(const mat4& viewProjMatrix, const Camera* camera, Shader* worldShader,
                             std::function<void(vec4 clipPlane)> renderSceneFunc) {
    time += 0.016f;

    for (auto& instance : waters) {
        auto& water = instance.water;

        // REFLECTION
        glBindFramebuffer(GL_FRAMEBUFFER, water->reflectionFBO);
        glViewport(0, 0, screenWidth, screenHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderSceneFunc(vec4(0, 1, 0, -instance.position.y));
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // REFRACTION
        glBindFramebuffer(GL_FRAMEBUFFER, water->refractionFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderSceneFunc(vec4(0, -1, 0, instance.position.y));
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // RENDER WATER SURFACE
        glViewport(0, 0, screenWidth, screenHeight);
        waterShader->use();

        mat4 model = Translate(instance.position) * Scale(instance.scale, 1.0f, instance.scale);
        waterShader->setUniform("ModelView", model);
        waterShader->setUniform("Projection", viewProjMatrix);
        waterShader->setUniform("eyePosition", vec4(camera->GetPosition(), 1.0f));
        waterShader->setUniform("uTime", time);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, water->dudvTexture);
        waterShader->setUniform("dudvMap", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, water->refractionTexture);
        waterShader->setUniform("refractionTexMap", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, water->reflectionTexture);
        waterShader->setUniform("reflectionTexMap", 2);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(water->waterVAO);
        glDrawElements(GL_TRIANGLES, water->meshIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
    }
}
