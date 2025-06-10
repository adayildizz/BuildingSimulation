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
    if (!data) {
        std::cerr << "Error in LoadRawData: data pointer is null for texture '" << m_fileName << "'" << std::endl;
        return false;
    }

    glGenTextures(1, &m_textureObj);
    if (m_textureObj == 0) { // Check if glGenTextures failed
        std::cerr << "Error generating texture object for '" << m_fileName << "'" << std::endl;
        return false;
    }
    glBindTexture(m_textureTarget, m_textureObj);

    GLenum internalFormat = GL_RGB;
    GLenum format = GL_RGB;
    // Add more cases if you support other BPP values (e.g., 4 for RGBA)
    if (bpp == 4) {
        internalFormat = GL_RGBA;
        format = GL_RGBA;
    } else if (bpp != 3) {
        std::cerr << "Unsupported BPP in LoadRawData: " << bpp << " for texture '" << m_fileName << "'. Defaulting to RGB." << std::endl;
        // Keep RGB as default but log error
    }

    if (m_textureTarget == GL_TEXTURE_2D) {
        glTexImage2D(m_textureTarget, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        std::cerr << "Unsupported texture target in LoadRawData for '" << m_fileName << "'" << std::endl;
        glBindTexture(m_textureTarget, 0); // Unbind before returning false
        glDeleteTextures(1, &m_textureObj); // Clean up generated texture object
        m_textureObj = 0;
        return false;
    }

    // Set default texture parameters - can be customized further
    glTexParameterf(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_S, GL_REPEAT); // REPEAT is often better for terrain
    glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_T, GL_REPEAT); // REPEAT is often better for terrain
    
    glGenerateMipmap(m_textureTarget); // Generate mipmaps for better quality at distance

    glBindTexture(m_textureTarget, 0); // Unbind texture

    std::cout << "Successfully loaded raw data into texture '" << m_fileName << "' (" << width << "x" << height << ", " << bpp << "bpp)" << std::endl;
    return true;
}

void Texture::Bind(GLenum TextureUnit)
{
    glActiveTexture(TextureUnit);
    glBindTexture(m_textureTarget, m_textureObj);
}
