#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "Angel.h"

class ShadowMap
{
public:
    ShadowMap();
    ~ShadowMap();

    virtual bool Init(unsigned int width, unsigned int height);

    // Bind the FBO for writing depth information
    virtual void Write();

    // Bind the depth texture for reading/sampling
    virtual void Read(GLenum textureUnit);

    unsigned int GetShadowWidth() const { return m_shadowWidth; }
    unsigned int GetShadowHeight() const { return m_shadowHeight; }

protected:
    GLuint m_fbo;
    GLuint m_shadowMap; // The texture ID for the depth map
    unsigned int m_shadowWidth;
    unsigned int m_shadowHeight;
};

#endif // SHADOWMAP_H
