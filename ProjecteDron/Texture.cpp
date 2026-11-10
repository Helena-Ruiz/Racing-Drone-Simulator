#include "Texture.h"
#include "stb_image.h"
#include <iostream>

Texture::Texture(const std::string& filePath, bool srgb)
    : m_ID(0), m_width(0), m_height(0), m_nrChannels(0), m_path(filePath)
{
    std::cout << "[TEXTURE] Intentant carregar: '" << filePath << "' ... ";


    unsigned char* data = stbi_load(filePath.c_str(), &m_width, &m_height, &m_nrChannels, 0);

    if (!data)
    {
        std::cout << "FALLADA!" << std::endl;
        std::cerr << "   >>> ERROR CRITIC: No s'ha pogut obrir el fitxer." << std::endl;
        std::cerr << "   >>> Motiu STB: " << stbi_failure_reason() << std::endl;
        return;
    }

    // [DEBUG] ÈXIT
    std::cout << "CORRECTE! (" << m_width << "x" << m_height << "px, canals: " << m_nrChannels << ")" << std::endl;

    GLenum format = GL_RGB;
    GLenum internalFormat = srgb ? GL_SRGB : GL_RGB;

    if (m_nrChannels == 1)
        format = internalFormat = GL_RED;
    else if (m_nrChannels == 3)
        format = GL_RGB, internalFormat = srgb ? GL_SRGB : GL_RGB;
    else if (m_nrChannels == 4)
        format = GL_RGBA, internalFormat = srgb ? GL_SRGB_ALPHA : GL_RGBA;

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (glewIsSupported("GL_EXT_texture_filter_anisotropic"))
    {
        GLfloat maxAnisotropy = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }

    stbi_image_free(data);
}

Texture::~Texture()
{
    if (m_ID != 0)
        glDeleteTextures(1, &m_ID);
}

void Texture::bind(unsigned int unit) const
{
    if (unit > 31)
        std::cerr << "[Texture] Warning: unit > 31" << std::endl;

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

void Texture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}
