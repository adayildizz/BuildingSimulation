#include "light.h"

Light::Light()
{
    color = vec3(1.0f, 1.0f, 1.0f);
    ambientIntensity = 0.3f;

    direction = vec3(0.0f, 1.0f, 0.0f);
    diffuseIntensity = 0.7f;
}

Light::Light(GLfloat red, GLfloat green, GLfloat blue, GLfloat aIntensity,
             GLfloat xDirection, GLfloat yDirection, GLfloat zDirection, GLfloat dIntensity)
{
    color = vec3(red, green, blue);
    ambientIntensity = aIntensity;
    direction = vec3(xDirection, yDirection, zDirection);
    diffuseIntensity = dIntensity;
}

void Light::UseLight(GLfloat ambientIntensityLocation, GLfloat ambientColorLocation,
                     GLfloat diffuseIntensityLocation, GLfloat directionLocation)
{
    glUniform3f(ambientColorLocation, color.x, color.y, color.z);
    glUniform1f(ambientIntensityLocation, ambientIntensity);
    glUniform3f(directionLocation, direction.x, direction.y, direction.z);
    glUniform1f(diffuseIntensityLocation, diffuseIntensity);
}

void Light::SetDirection(const vec3& dir) {
    direction = dir; // Assumes 'direction' is a vec3 member
}

void Light::SetColor(const vec3& col) {
    color = col; // Assumes 'color' is a vec3 member
}

void Light::SetAmbientIntensity(GLfloat intensity) {
    ambientIntensity = intensity; // Assumes 'ambientIntensity' is a GLfloat member
}

void Light::SetDiffuseIntensity(GLfloat intensity) {
    diffuseIntensity = intensity; // Assumes 'diffuseIntensity' is a GLfloat member
}

Light::~Light() {}
