#include "Camera.h"

const float SHAKE_BASE_AMPLITUDE = 0.01f;   // Vibració mínima
const float SHAKE_MAX_AMPLITUDE = 0.05f;    // Vibració màxima a tope de gas
float SHAKE_FREQUENCY = 1000.0f;              // Freqüència molt alta (Hz)

Camera::Camera()
    : m_position(0.0f, 0.0f, 0.0f),
    m_orientation(0.0f, 0.0f, -1.0f),
    m_up(0.0f, 1.0f, 0.0f),
    m_fov(60.0f),
    m_aspectRatio(16.0f / 9.0f),
    m_nearPlane(0.1f),
    m_farPlane(1000.0f),
    m_isPerspective(true),
	m_type(Type::FREE)
{
	setType(m_type);
    updateView();
    updateProjection();
}

Camera::Camera(float aspectRatio, Type type)
    : m_position(0.0f, 0.0f, 0.0f),
    m_orientation(0.0f, 0.0f, -1.0f),
    m_up(0.0f, 1.0f, 0.0f),
    m_fov(60.0f),
    m_aspectRatio(aspectRatio),
    m_nearPlane(0.1f),
    m_farPlane(1000.0f),
    m_isPerspective(true),
    m_type(type)
{
	setType(m_type);
    updateView();
    updateProjection();
}

void Camera::applyVibration(float throttle, float deltaTime)
{
    // Limitem el throttle a [0,1]
    //throttle = glm::clamp(throttle, 0.0f, 1.0f);

    // Calcula la intensitat de vibració segons throttle
    //float currentShake = SHAKE_BASE_AMPLITUDE + (SHAKE_MAX_AMPLITUDE - SHAKE_BASE_AMPLITUDE) * throttle;

    // Vibració sin/cos amb nombres primers per evitar patrons repetitius
    //float offsetX = sin(deltaTime * SHAKE_FREQUENCY * 13.0f) * currentShake;
    //float offsetY = cos(deltaTime * SHAKE_FREQUENCY * 17.0f) * currentShake;
    //float offsetZ = sin(deltaTime * SHAKE_FREQUENCY * 19.0f) * currentShake;

    //m_position += glm::vec3(offsetX, offsetY, offsetZ);

    //updateView();
}


void Camera::setType(Type type)
{
    m_type = type;
    switch (type)
    {
    case Type::FPV: m_fov = 115.0f; m_nearPlane = 0.05f; m_farPlane = 300.0f;  break;
    case Type::THIRD_PERSON: m_fov = 65.0f; m_nearPlane = 0.1f; m_farPlane = 500.0f; break;
    case Type::FREE: m_fov = 60.0f; m_nearPlane = 0.1f; m_farPlane = 1000.0f; break;
    }
    updateProjection();
}

void Camera::updateView() 
{
    m_view = glm::lookAt(m_position, m_position + m_orientation, m_up);
}

void Camera::updateProjection() 
{
    if (m_isPerspective)
        m_projection = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
    else
        m_projection = glm::ortho(-10.0f, 10.0f, -10.0f / m_aspectRatio, 10.0f / m_aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::updateFollow(const glm::vec3& targetPos, const glm::mat4& droneRotation)
{
    if (m_type == Type::FPV)
    {
        //Punt de vista dins del dron
        glm::vec3 localOffset(0.0f, 0.2f, 0.1f);
        glm::vec3 worldOffset = glm::vec3(droneRotation * glm::vec4(localOffset, 1.0f));

        m_position = targetPos + worldOffset;

        // Direccions derivades de la rotació del dron
        glm::vec3 forward = glm::normalize(glm::vec3(droneRotation * glm::vec4(0, 0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::vec3(droneRotation * glm::vec4(0, 1, 0, 0)));

        m_orientation = forward;
        m_up = up;

        updateView();
    }
    else if (m_type == Type::THIRD_PERSON)
    {

        if (!m_replay)
        {
            // --- 1. Definir offset de la cámara
            glm::vec3 localOffset(0.0f, 0.2f, -0.8f);
            glm::vec3 worldOffset = glm::vec3(droneRotation * glm::vec4(localOffset, 1.0f));

            glm::vec3 desiredPos = targetPos + worldOffset;

            // --- 2. Suavizado de la posición dependiente de deltaTime
            float positionSmooth = 8.0f; 
            float tPos = 1.0f - expf(-positionSmooth * m_deltaTime);
            m_position = glm::mix(m_position, desiredPos, tPos);

            // --- 3. Up vector del dron
            glm::vec3 up = glm::normalize(glm::vec3(droneRotation * glm::vec4(0, 1, 0, 0)));
            m_up = up;

            // --- 4. Definir punto a mirar (lookTarget) suavizado con slerp
            glm::vec3 forward = glm::normalize(glm::vec3(droneRotation * glm::vec4(0, 0, 1, 0)));
            glm::vec3 lookTarget = targetPos + forward * 0.5f; 

            glm::vec3 desiredOrientation = glm::normalize(lookTarget - m_position);

            // --- 5. Suavizado de orientación usando slerp implícito con mix
            float orientationSmooth = 8.0f;
            float tOri = 1.0f - expf(-orientationSmooth * m_deltaTime);
            m_orientation = glm::normalize(glm::mix(m_orientation, desiredOrientation, tOri));

            // --- 6. Actualizar view matrix
            updateView();

        }
        else
        {
            // --- 1. Definir offset de la cámara
            glm::vec3 localOffset(0.0f, 0.2f, -0.8f);
            glm::vec3 worldOffset = glm::vec3(droneRotation * glm::vec4(localOffset, 1.0f));

            glm::vec3 desiredPos = targetPos + worldOffset;

            // --- 2. Suavizado de la posición
            float positionSmooth = 8.0f;

            // Càlcul del factor de suavitzat (tPos)
            // Utilitza 1.0f (sense suavitzat) si isReplayActive és true.
            // Utilitza el suavitzat exponencial si isReplayActive és false.
            float tPos = m_replay ? 1.0f : 1.0f - expf(-positionSmooth * m_deltaTime);
            m_position = glm::mix(m_position, desiredPos, tPos);

            // --- 3. Up vector del dron
            glm::vec3 up = glm::normalize(glm::vec3(droneRotation * glm::vec4(0, 1, 0, 0)));
            m_up = up;

            // --- 4. Definir punto a mirar (lookTarget)
            glm::vec3 forward = glm::normalize(glm::vec3(droneRotation * glm::vec4(0, 0, 1, 0)));
            glm::vec3 lookTarget = targetPos + forward * 0.5f;

            glm::vec3 desiredOrientation = glm::normalize(lookTarget - m_position);

            // --- 5. Suavizado de orientación
            float orientationSmooth = 8.0f;

            // Càlcul del factor de suavitzat
            // Utilitza 1.0f (sense suavitzat) si isReplayActive és true.
            float tOri = m_replay ? 1.0f : 1.0f - expf(-orientationSmooth * m_deltaTime);
            m_orientation = glm::normalize(glm::mix(m_orientation, desiredOrientation, tOri));

            // --- 6. Actualizar view matrix
            updateView();
        }
    }
    else if (m_type == Type::FREE)
    {
        // La càmera NO es mou, només orientació cap al dron
		m_position = glm::vec3(-10.0f, 2.0f, -10.0f);
		m_orientation = glm::normalize(targetPos - m_position);

		m_up = glm::vec3(0, 1, 0);

		updateView();


    }
}

void Camera::updateFreeCamera(float deltaTime, GLFWwindow* window)
{

    float speed = m_freeCameraSpeed * deltaTime;

    // Movimiento WASD + Q/E
    glm::vec3 right = glm::normalize(glm::cross(m_orientation, glm::vec3(0, 1, 0)));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_position += speed * m_orientation;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_position -= speed * m_orientation;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_position -= speed * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_position += speed * right;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) m_position -= speed * glm::vec3(0, 1, 0);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_position += speed * glm::vec3(0, 1, 0);

    // Captura movimiento del ratón
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (m_firstMouse)
    {
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        m_firstMouse = false;
    }

    float deltaX = float(mouseX - m_lastMouseX);
    float deltaY = float(m_lastMouseY - mouseY); // invertir Y
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;

    deltaX *= m_mouseSensitivity;
    deltaY *= m_mouseSensitivity;

    m_yaw += deltaX;
    m_pitch += deltaY;

    // Limitar pitch
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    // Calcula orientación
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_orientation = glm::normalize(front);

    updateView();
}


void Camera::sendToShader(Shader& shader, const char* uniform)
{
    glm::mat4 cameraMatrix = m_projection * m_view;
    GLuint location = glGetUniformLocation(shader.getID(), uniform);
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(cameraMatrix));
}