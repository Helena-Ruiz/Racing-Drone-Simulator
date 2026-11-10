
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Model3D.h"
#include "Camera.h"
#include "Controller.h"
#include "quatern.h"
#include <glm/gtx/quaternion.hpp>
#include "Defines.h"
#include "ReplaySystem.h"
#include "AudioManager.h"
#include "PhysicsWorld.h"

class Dron {
public:
    Dron();
    void carregarModel();
    void draw(Shader& shader);
	void update(float deltaTime, Controller& controller, MODE_VOL mode, PhysicsWorld& physicsWorld);
    void reset();
	void reset(glm::vec3 position, glm::vec3 rotation);
    
	glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getForward() const;
    float getAltitude() const { return m_altitude; }
    float getBatteryLevel() const { return m_batteryLevel; }
    MODE_VOL getMode() const { return m_modeVol; }
    float getAccelerationMagnitude() const { return glm::length(m_acceleration); }
    float getThrottle() const { return m_currentThrottle; }
	float getaudioThrottle() const { return m_currentThrottle; }
    float getRoll() const { return m_roll; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    float getSpeed() const { return glm::length(m_velocity); }
    bool getReplayActive() { return m_ReplayActive; }
    glm::mat4 getRotationMatrix() const;
    bool getsopreplay() { return m_stop_replay; }
    double getreplytime() { return m_replayTime; }
    ReplaySystem& getreplay() { return m_replay; }
	GL_Quat getOrientacio() const { return m_orientacio; }
	float getRealPitch() const {
		glm::quat q(( float ) m_orientacio.w, ( float ) m_orientacio.x,
			( float ) m_orientacio.y, ( float ) m_orientacio.z);
		return glm::pitch(q);
	}
	float getRealRoll() const {
		glm::quat q(( float ) m_orientacio.w, ( float ) m_orientacio.x,
			( float ) m_orientacio.y, ( float ) m_orientacio.z);
		return glm::roll(q);
	}

	void setreplayreal(ReplaySystem rep) { m_replay = rep; }
    void setReplay(bool replay) { m_ReplayActive = replay; }
    void setMode(MODE_VOL mode) { m_modeVol = mode; }
	void setPhysicsParameters(float mass, float maxThrust, float responsiveness, float drag, float lag);
	void setModelType(int modelIndex);

	void stopReplay(bool stop) { m_stop_replay = stop; }
    void carregarPropellers();
	void onCollision(const glm::vec3& normal, float penetrationDepth); //bullet 

private:
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_acceleration;
    glm::vec3 m_angularVelocity = glm::vec3(0.0f); // unitat rad/s
    glm::vec3 m_inercia = glm::vec3(0.02f, 0.02f, 0.04f);

	float m_maxThrust;
	float m_turnResponsiveness;
	float m_dragCoef;
	float m_motorLag;
	int m_currentModelIndex = -1;

    float m_replayTime = 0.0f;
    float m_altitudeHold;
    float m_pitch;
    float m_yaw;
    float m_roll;
    int time = 0;

    float m_currentThrottle = 0.0f;

    float m_flightTime;
    float m_mass;
    float m_altitude;
    float m_batteryLevel;

    bool m_loaded = false;
    bool m_ReplayActive = false;
	float audio_Throttle = 0.0f;
    Model3D m_model;
    MODE_VOL m_modeVol;
    glm::vec3 m_propellerPos[4];


    Model3D m_propellerModel;
    bool m_propellerLoaded;
    float m_propellerAngle; 
    bool m_stop_replay = false;

    GL_Quat m_orientacio = { 1.0, 0.0, 0.0, 0.0 };

    ReplaySystem m_replay;

	bool m_isColliding = false;      //bullet
	glm::vec3 m_collisionNormal;
};