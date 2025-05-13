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

    ~Light();

private:
    vec3 color;
    GLfloat ambientIntensity;
    vec3 direction;
    GLfloat diffuseIntensity;
};

#endif // LIGHT_H
