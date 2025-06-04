#ifndef LIGHT_H
#define LIGHT_H

#include "Angel.h"

class Light {
public:
    Light();

    Light(GLfloat red, GLfloat green, GLfloat blue, GLfloat aIntensity,
          GLfloat xDirection, GLfloat yDirection, GLfloat zDirection, GLfloat dIntensity);

    void UseLight(GLfloat ambientIntensityLocation, GLfloat ambientColorLocation,
                  GLfloat diffuseIntensityLocation, GLfloat directionLocation);
    void SetDirection(const vec3& dir);
    void SetColor(const vec3& col);
    void SetAmbientIntensity(GLfloat intensity);
    void SetDiffuseIntensity(GLfloat intensity);

    ~Light();

private:
    vec3 color;
    GLfloat ambientIntensity;
    vec3 direction;
    GLfloat diffuseIntensity;
};

#endif // LIGHT_H
