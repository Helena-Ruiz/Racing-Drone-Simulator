#pragma once

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h> // Important per triggers
#include "Trigger.h"

enum class RaceState {
	IDLE,       // Abans de creuar el primer arc
	RACING,     // Cronòmetre en marxa
	FINISHED    // Cursa acabada
};

class RaceManager {
public:
	RaceManager();
	~RaceManager();

	// Inicialitza amb els triggers carregats al World
	void init(const std::vector<Trigger*>& triggers);

	// Es crida a cada frame (Scene::update)
	void update(float deltaTime, btRigidBody* droneBody);

	// Getters per al HUD
	float getCurrentTime() const { return m_currentTime; }
	int getNextCheckpointID() const { return m_nextCheckpointIndex; } // Retorna índex (0..N)
	int getTotalCheckpoints() const { return ( int ) m_triggers.size(); }
	RaceState getState() const { return m_state; }
	int getTotalLaps() const { return m_totalLaps; }
	int getLapsCompleted() const { return m_lapsCompleted; }

	void setTotalLaps(int laps) { m_totalLaps = laps; }

	// Reinicia la cursa
	void reset();

private:
	std::vector<Trigger*> m_triggers;
	RaceState m_state;

	float m_currentTime;
	int m_nextCheckpointIndex; // Índex dins del vector (0 = Trigger 1)
	int m_totalCheckpoints;
	int m_lapsCompleted;
	int m_totalLaps;

	// Funció interna per comprovar solapament Bullet
	bool checkCollision(Trigger* trigger, btRigidBody* droneBody);

	// Què passa quan creuem un arc vàlid
	void handleCheckpointPassed(Trigger* trigger);
};