#include "KeyboardController.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstring>

//serveix per suavitzar el moviment
static float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

//rang [-1,1]
static float clampf(float v, float lo, float hi) {
	return std::max(lo, std::min(hi, v));
}

//comprova si una tecla esta premuda
static bool teclaSeleccionada(int tecla) {
	return (GetAsyncKeyState(tecla) & 0x8000) != 0;
}

//inicialitza els valors del teclat a 0
KeyboardController::KeyboardController() {
	std::memset(m_axisValues, 0, sizeof(m_axisValues));
	std::memset(m_buttonStates, 0, sizeof(m_buttonStates));
	std::memset(m_prevButtonStates, 0, sizeof(m_prevButtonStates));
}

void KeyboardController::update() {
	float target[4];
	switch (m_mode)
	{
	case MODE_VOL::ACROBATIC:
		target[0] = -1.0f;
		target[1] = 0.0f;
		target[2] = 0.0f;
		target[3] = 0.0f;
		break;

	case MODE_VOL::STABILIZED:
		target[0] = 0.0f;
		target[1] = 0.0f;
		target[2] = 0.0f;
		target[3] = 0.0f;
		break;

	default:
		target[0] = -1.0f;
		target[1] = 0.0f;
		target[2] = 0.0f;
		target[3] = 0.0f;
		break;
	}

	//moviment Throttle (amunt/avall) --> tecles W i S
	if (teclaSeleccionada('W')) {
		target[0] += 1.0f;
	}
	if (teclaSeleccionada('S')) {
		target[0] -= 1.0f;
	}

	//moviment Yaw (esquerra/dreta) --> tecles A i D
	if (teclaSeleccionada('A')) {
		target[1] += 1.0f;
	}
	if (teclaSeleccionada('D')) {
		target[1] -= 1.0f;
	}

	//moviment Pitch (endavant/enrere) --> tecles UP i DOWN
	if (teclaSeleccionada(VK_UP)) {
		target[2] += 1.0f;
	}
	if (teclaSeleccionada(VK_DOWN)) {
		target[2] -= 1.0f;
	}

	//moviment Roll (rotacio esquerra/dreta) --> tecles LEFT i RIGHT
	if (teclaSeleccionada(VK_LEFT)) {
		target[3] -= 1.0f;
	}
	if (teclaSeleccionada(VK_RIGHT)) {
		target[3] += 1.0f;
	}

	//assegura que els valors estiguin entre -1 i 1
	for (int i = 0; i < 4; ++i) {
		target[i] = clampf(target[i], -1.0f, 1.0f);
	}

	//suavitza la transicio dels valors actuals cap als objectius
	const float suavitza = 0.1f; //factor de suavitzat
	for (int i = 0; i < 4; ++i) {
		m_axisValues[i] = lerp(m_axisValues[i], target[i], suavitza);
	}

	//botons CanviCamara (tecla C) i RecollirPremi (tecla espai)

	// Guarda l’estat actual dels botons
	bool currentC = teclaSeleccionada('C');
	bool currentM = teclaSeleccionada('M');
	bool currentL = teclaSeleccionada('L');
	bool currentDebug = teclaSeleccionada(VK_F1);

	// Actualitza els estats anteriors
	m_prevButtonStates[0] = m_buttonStates[0];
	m_prevButtonStates[1] = m_buttonStates[1];
	m_prevButtonStates[2] = m_buttonStates[2];
	m_prevButtonStates[3] = m_buttonStates[3];

	// Actualitza els nous estats
	m_buttonStates[0] = currentC;
	m_buttonStates[1] = currentM;
	m_buttonStates[2] = currentL;
	m_buttonStates[3] = currentDebug;

}

float KeyboardController::getAxis(const std::string& axisName) const {

	if (axisName == "Throttle") {
		return m_axisValues[0];
	}
	else if (axisName == "Yaw") {
		return m_axisValues[1];
	}
	else if (axisName == "Pitch") {
		return m_axisValues[2];
	}
	else if (axisName == "Roll") {
		return m_axisValues[3];
	}
	return 0.0f;
}

bool KeyboardController::getButton(const std::string& buttonName) const {		

	if (buttonName == "CanviCamara")
		return (m_buttonStates[0] && !m_prevButtonStates[0]);

	if (buttonName == "SwitchMode")
		return (m_buttonStates[1] && !m_prevButtonStates[1]);

	if (buttonName == "CanviLlum")
		return (m_buttonStates[2] && !m_prevButtonStates[2]);

	if (buttonName == "debug")
		return (m_buttonStates[3] && !m_prevButtonStates[3]);

	return false;
}
