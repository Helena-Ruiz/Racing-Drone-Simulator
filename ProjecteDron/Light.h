#pragma once
#include <glm/glm.hpp>


class Light
{
public:
    enum class LIGHT_TYPE {
        DIRECTIONAL,
        POINT,
        SPOT
    };

    Light(LIGHT_TYPE t = LIGHT_TYPE::DIRECTIONAL,
        glm::vec3 col = glm::vec3(1.0f),
        float inten = 1.0f,
        glm::vec3 pos = glm::vec3(0.0f),
        glm::vec3 dir = glm::vec3(-1.0f, -1.0f, -1.0f));

    void applyToShader(const class Shader& shader, int index = 0) const;

    LIGHT_TYPE getType() const { return m_type; }
    glm::vec3 getPosition() const { return m_position; }
	glm::vec3 getDirection() const { return m_direction; }
    glm::vec3 getColor() const { return m_color; }
    float getIntensity() const { return m_intensity; }
    void setColor(const glm::vec3& color) {
        m_color = color;
    }

    void setIntensity(float intensity) {
        m_intensity = intensity;
    }

    void setDirection(const glm::vec3& direction) {
        m_direction = direction;
    }



private:
    glm::vec3 m_position;
    glm::vec3 m_color;
    glm::vec3 m_direction;
    float m_intensity;

    LIGHT_TYPE m_type;

};
