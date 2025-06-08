#include "ShadowMap.h"
#include <iostream>

ShadowMap::ShadowMap()
{
    m_fbo = 0;
    m_shadowMap = 0;
    m_shadowWidth = 0;
    m_shadowHeight = 0;
}

ShadowMap::~ShadowMap()
{
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
    }
    if (m_shadowMap != 0) {
        glDeleteTextures(1, &m_shadowMap);
    }
}

bool ShadowMap::Init(unsigned int width, unsigned int height)
{
    m_shadowWidth = width;
    m_shadowHeight = height;

    // Create the FBO
    glGenFramebuffers(1, &m_fbo);

    // Create the depth texture
    glGenTextures(1, &m_shadowMap);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_shadowWidth, m_shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Bind the FBO and attach the depth texture
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMap, 0);

    // We don't need to draw or read color data
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check for errors
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer Error: " << status << std::endl;
        return false;
    }

    // Unbind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void ShadowMap::Write()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void ShadowMap::Read(GLenum textureUnit)
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap);
}
