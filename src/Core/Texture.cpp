#include "../include/stb/stb_image.h"
#include "Texture.h"

Texture::Texture(GLenum TextureTarget, const std::string& FileName)
{
    m_textureTarget = TextureTarget;
    m_fileName = FileName;
}

bool Texture::Load()
{
    stbi_set_flip_vertically_on_load(1);
    int width= 0, height= 0, bpp= 0;
    unsigned char* image_data = stbi_load(m_fileName.c_str(), &width, &height, &bpp, 0);
    if (!image_data)
    {
        std::cerr << "Error in loading the texture " << m_fileName << std::endl;
        return false;
    }

    glGenTextures(1, &m_textureObj);
    glBindTexture(m_textureTarget, m_textureObj);
    if (m_textureTarget == GL_TEXTURE_2D) 
    {
        GLenum format = GL_RGB;
        GLenum internalFormat = GL_RGB;
        if (bpp == 4) {
            format = GL_RGBA;
            internalFormat = GL_RGBA;
        }
        glTexImage2D(m_textureTarget, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, image_data);
    }
    
    else
    {
        std::cerr << "Unsupported texture target" << std::endl;
        return false;
    }

    glTexParameterf(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(m_textureTarget, 0);

    stbi_image_free(image_data);
    return true;
}

bool Texture::LoadRawData(int width, int height, int bpp, unsigned char* data) {
    glGenTextures(1, &m_textureObj);
    if (m_textureObj == 0) {
        std::cerr << "Error generating texture object for '" << m_fileName << "'" << std::endl;
        return false;
    }
    glBindTexture(m_textureTarget, m_textureObj);

    GLenum internalFormat = GL_RGB;
    GLenum format = GL_RGB;

    if (bpp == 4) {
        internalFormat = GL_RGBA;
        format = GL_RGBA;
    } else if (bpp != 3) {
        std::cerr << "Unsupported BPP in LoadRawData: " << bpp << " for texture '" << m_fileName << "'. Defaulting to RGB." << std::endl;
    }

    if (m_textureTarget == GL_TEXTURE_2D) {
        // Allow nullptr here — it's valid for framebuffer targets
        glTexImage2D(m_textureTarget, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        std::cerr << "Unsupported texture target in LoadRawData for '" << m_fileName << "'" << std::endl;
        glBindTexture(m_textureTarget, 0);
        glDeleteTextures(1, &m_textureObj);
        m_textureObj = 0;
        return false;
    }

    // Texture parameters — mipmaps only if data was provided
    glTexParameterf(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (data) {
        glGenerateMipmap(m_textureTarget); // Only generate mipmaps if actual image data
    }

    glBindTexture(m_textureTarget, 0);

    std::cout << "Successfully created raw texture '" << m_fileName << "' (" << width << "x" << height << ", " << bpp << "bpp)" << std::endl;
    return true;
}

void Texture::Bind(GLenum TextureUnit)
{
    glActiveTexture(TextureUnit);
    glBindTexture(m_textureTarget, m_textureObj);
}
