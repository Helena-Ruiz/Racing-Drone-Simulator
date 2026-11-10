#include "Light.h"
#include "Shader.h"
#include <string>

Light::Light(LIGHT_TYPE t, glm::vec3 col, float inten, glm::vec3 pos, glm::vec3 dir)
{
    m_type = t;
    m_color = col;
    m_intensity = inten;
    m_position = pos;
	m_direction = dir;
}

void Light::applyToShader(const Shader& shader, int index) const
{
    std::string prefix;

    switch (m_type) {
    case LIGHT_TYPE::DIRECTIONAL:
        prefix = "light";

        glm::vec3 ambient = m_color * m_intensity * 0.2f;
        glm::vec3 diffuse = m_color * m_intensity;
        glm::vec3 specular = m_color * m_intensity;

        shader.setVec3(prefix + ".direction", m_direction);
        shader.setVec3(prefix + ".ambient", ambient);
        shader.setVec3(prefix + ".diffuse", diffuse);
        shader.setVec3(prefix + ".specular", specular);

        break;

    case LIGHT_TYPE::POINT:
        prefix = "pointLights[" + std::to_string(index) + "]";
        shader.setVec3(prefix + ".position", m_position);
        shader.setVec3(prefix + ".color", m_color);
        shader.setFloat(prefix + ".intensity", m_intensity);
        break;

    case LIGHT_TYPE::SPOT:
        prefix = "spotLights[" + std::to_string(index) + "]";
        shader.setVec3(prefix + ".position", m_position);
        shader.setVec3(prefix + ".direction", m_direction);
        shader.setVec3(prefix + ".color", m_color);
        shader.setFloat(prefix + ".intensity", m_intensity);
        break;
    }
}
