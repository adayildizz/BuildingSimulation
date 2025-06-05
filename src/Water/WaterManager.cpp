#include "WaterManager.h"

WaterManager::WaterManager(std::shared_ptr<Shader> waterShader, int screenWidth, int screenHeight)
    : waterShader(waterShader), screenWidth(screenWidth), screenHeight(screenHeight) {}

void WaterManager::addWaterAt(const vec3& position, float scale) {
    auto newWater = std::make_unique<Water>(waterShader.get());
    newWater->dudvTexture = newWater->loadTexture("resources/textures/dudvMap.jpg");
    newWater->generateMesh();
    newWater->createWaterMeshVAO();
    newWater->reflectionFBO = newWater->createFBO(newWater->reflectionTexture, screenWidth, screenHeight);
    newWater->refractionFBO = newWater->createFBO(newWater->refractionTexture, screenWidth, screenHeight);

    waters.push_back({ std::move(newWater), position, scale });
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

        // REFRACTION
        glBindFramebuffer(GL_FRAMEBUFFER, water->refractionFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderSceneFunc(vec4(0, -1, 0, instance.position.y));

        // RENDER WATER SURFACE
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
