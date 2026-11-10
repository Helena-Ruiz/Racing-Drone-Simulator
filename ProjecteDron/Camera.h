#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Defines.h"

class Camera
{
public:


    Camera();
    Camera(float aspectRatio, Type type);

    void applyVibration(float throttle, float deltaTime);

    void updateView();
    void updateProjection();
    void updateFollow(const glm::vec3& targetPos, const glm::mat4& droneRotation);
    void updateFreeCamera(float deltaTime, GLFWwindow* window);

    void sendToShader(Shader& shader, const char* uniform);

    // Setters
    void setPosition(const glm::vec3& pos) { m_position = pos; updateView();};
    void setOrientation(const glm::vec3& dir) { m_orientation = dir; };
    void setProjectionMode(bool perspective) { m_isPerspective = perspective; };
    void setType(Type type);
    void setAspectRatio(float ratio) { m_aspectRatio = ratio; };

    // Getters
    glm::mat4 getViewMatrix() const { return m_view; }
    glm::mat4 getProjectionMatrix() const { return m_projection; }
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getOrientation() const { return m_orientation; }
	Type getType() const { return m_type; }
    void setdeltatime(float delta) { m_deltaTime = delta; }
    void setreplay(bool modo){ m_replay = modo; }

private:
	// Propietats bàsiques
    glm::vec3 m_position;
    glm::vec3 m_orientation;
    glm::vec3 m_up;

    bool m_replay = false;

    // Paràmetres de projecció
    float m_deltaTime;
    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;
    bool m_isPerspective;
	Type m_type;

    // Matrius
    glm::mat4 m_view;
    glm::mat4 m_projection;
    
    bool m_debugFreeCamera = false;   // Activa la cámara libre
    float m_yaw = -90.0f;             // Ángulo inicial mirando hacia -Z
    float m_pitch = 0.0f;
    float m_mouseSensitivity = 0.1f;

    double m_lastMouseX = 400;        // Inicializar en el centro de la ventana
    double m_lastMouseY = 300;
    bool m_firstMouse = true;

    float m_freeCameraSpeed = 5.0f;   // Velocidad de movimiento
};
