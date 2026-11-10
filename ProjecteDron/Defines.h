#pragma once

enum class MapID {
	MAP_STADIUM = 0,
    MAP_CITY = 1
};

enum class DroneModel {
	OG_MODEL = 0,
	DJI_MODEL = 1,
	AGGRESSIVE_MODEL = 2
};

enum class MODE_VOL {
	ACROBATIC,
	STABILIZED,
	DESARMED
};

enum class Type {
	FPV,
	THIRD_PERSON,
	FREE,
	EDIT
};

enum class AlphaMode { 
	ALPHAMODE_OPAQUE,
	ALPHAMODE_MASK,
	ALPHAMODE_BLEND 
};

enum class ControllerType {
	CONTROLLER_PS4,
	CONTROLLER_DRONE,
	KEYBOARD
};

static int currentDebugMode = 0;


// Paràmetres físics per defecte del dron

const float GRAVETAT = -9.81f;			// Gravetat (m/s²)
const float ACRO_TORQUE = 1.0f;			// Torque per pitch/roll (responsivitat del gir)
const float ACRO_TORQUE_YAW = 0.6f;		// Torque aplicat al gir de yaw
const float ANGULAR_DAMP = 1.5f;		// Amortiment que redueix la velocitat angular
const float MAX_ANGULAR_SPEED = 8.0f;	// Límit de velocitat angular (rad/s)
const float MAX_THRUST = 30.0f;			// Empenta màxima dels motors
const float DRAG_LINEAR = 0.01f;        // Component lineal (per baixes velocitats)
const float DRAG_QUADRATIC = 0.018f;    // Component quadràtic (realista)
const float MAX_RATE = 10.0f;			// Velocitat màxima de canvi d'angles (graus/s)
const float MOTOR_LAG = 0.1f; 			// Temps de resposta dels motors (segons)
const float DRONE_MASS = 0.8f;          // Massa del dron (kg)

struct DroneConfig {
	float mass;
	float maxThrust;
	float turnResponsiveness;
	float drag;
	float motorLag;
};
