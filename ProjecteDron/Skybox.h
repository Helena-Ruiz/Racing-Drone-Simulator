#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include "Shader.h"
class Skybox {
public:
    Skybox();
    ~Skybox();

    void initSkybox(std::vector<std::string> faces);

    void draw(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int m_skyboxVAO, m_skyboxVBO;
    unsigned int m_textureID;
    Shader* m_shader;

    // Funció auxiliar de debug
    void checkOpenGLError(const std::string& location);
};