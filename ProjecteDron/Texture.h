#pragma once
#include <string>
#include <GL/glew.h>

class Texture
{
public:
    Texture(const std::string& filePath, bool srgb = false);
    ~Texture();

    void bind(unsigned int unit = 0) const;
    void unbind() const;

    GLuint getID() const { return m_ID; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getChannels() const { return m_nrChannels; }
    std::string getPath() const { return m_path; }

private:
    GLuint m_ID;
    int m_width, m_height, m_nrChannels;
    std::string m_path;
};
