#pragma once

#include "Angel.h"
#include <string>

class Texture
{
public:
    Texture(GLenum TextureTarget, const std::string& FileName);

    bool Load();

    bool LoadRawData(int width, int height, int bpp, unsigned char* data);

    void Bind(GLenum TextureUnit);

private:
    std::string m_fileName;
    GLenum m_textureTarget;
    GLuint m_textureObj = 0;
    
};
