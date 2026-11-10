#include "GamepadController.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "Defines.h"

static float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

GamepadController::GamepadController(int gamepadID)
    : m_gamepadID(gamepadID)
{
	m_type = ControllerType::CONTROLLER_DRONE;
    std::memset(m_axisValues, 0, sizeof(m_axisValues));
    std::memset(m_buttonStates, 0, sizeof(m_buttonStates));
	std::memset(m_prevButtonStates, 0, sizeof(m_prevButtonStates));
}

void GamepadController::update()
{
	if ( !glfwJoystickPresent(m_gamepadID) ) {
		std::cerr << "[GamepadController] No gamepad connected (ID " << m_gamepadID << ")" << std::endl;
		return;
	}

	// 1. Guarda l'estat actual com a previ (l'estat del frame passat)
	for ( int i = 0; i < 4; ++i ) {
		m_prevButtonStates[ i ] = m_buttonStates[ i ];
	}

	int axesCount, buttonsCount;
	const float* axes = glfwGetJoystickAxes(m_gamepadID, &axesCount);
	const unsigned char* buttons = glfwGetJoystickButtons(m_gamepadID, &buttonsCount);

	// Per debug: mostra canvis
	static float prevAxes[ 8 ] = { 0 };
	static bool prevButtons[ 4 ] = { false };

	// 2. Actualitza Eixos (Nous valors)
	for ( int i = 0; i < 8; ++i )
		m_axisValues[ i ] = ( axesCount > i ) ? axes[ i ] : 0.0f;

	// 3. Actualitza Botons (Nous valors)
	for ( int i = 0; i < 4; ++i )
		m_buttonStates[ i ] = ( buttonsCount > i ) ? buttons[ i ] == GLFW_PRESS : false;

	// Mostra només si ha canviat (El teu codi de debug)
	for ( int i = 0; i < 8; ++i ) {
		if ( fabs(m_axisValues[ i ] - prevAxes[ i ]) > 0.05f ) {
			std::cout << "Axis " << i << " = " << m_axisValues[ i ] << std::endl;
			prevAxes[ i ] = m_axisValues[ i ];
		}
	}
	for ( int i = 0; i < 4; ++i ) {
		if ( m_buttonStates[ i ] != prevButtons[ i ] ) {
			std::cout << "Button " << i << " = " << m_buttonStates[ i ] << std::endl;
			prevButtons[ i ] = m_buttonStates[ i ];
		}
	}
}

float GamepadController::getAxis(const std::string& axisName) const
{
    switch (m_type) 
	{
	case ControllerType::CONTROLLER_PS4:
        if (axisName == "Roll")         return  m_axisValues[2];
        if (axisName == "Pitch")        return -m_axisValues[5];
        if (axisName == "Throttle")     return -m_axisValues[1];
        if (axisName == "Yaw")          return -m_axisValues[0];
        if (axisName == "")             return  m_axisValues[4];
        if (axisName == "")				return  m_axisValues[3];
        if (axisName == "")             return  m_axisValues[6];
        if (axisName == "")				return  m_axisValues[4];
        return 0.0f;

    case ControllerType::CONTROLLER_DRONE:
        if (axisName == "Roll")         return m_axisValues[0];
        if (axisName == "Pitch")        return m_axisValues[1];
        if (axisName == "Throttle")     return m_axisValues[2];
        if (axisName == "Yaw")          return -m_axisValues[3];
        if (axisName == "SwitchMode")	return m_axisValues[4];
        if (axisName == "")             return m_axisValues[5];
        if (axisName == "")				return m_axisValues[6];
        if (axisName == "CameraSwitch") return m_axisValues[7];
        return 0.0f;
	
	default:
		if (axisName == "") return 0.0f;
		return 0.0f;
    }
}

bool GamepadController::getButton(const std::string& buttonName) const
{
    switch ( m_type ) {
    case ControllerType::CONTROLLER_PS4:
        if (buttonName == "CameraSwitch") return m_buttonStates[1];
        if (buttonName == "SwitchMode") return m_buttonStates[2];
        return false;
		break;
	case ControllerType::CONTROLLER_DRONE:
		if (buttonName == "debug")
			return (m_buttonStates[0] && !m_prevButtonStates[0]);
		break;
	default:
		if (buttonName == "") return false;
		return false;
    }
}

bool GamepadController::isConnected() const
{
    return glfwJoystickPresent(m_gamepadID);
}
