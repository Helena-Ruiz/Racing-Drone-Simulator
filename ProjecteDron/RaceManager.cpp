#include "RaceManager.h"
#include <iostream>
#include "AudioManager.h"

RaceManager::RaceManager()
	: m_state(RaceState::IDLE), m_currentTime(0.0f), m_nextCheckpointIndex(0)
{
}

RaceManager::~RaceManager() {
	// No esborrem els triggers aquí, els gestiona el World
	m_triggers.clear();
}

void RaceManager::init(const std::vector<Trigger*>& triggers) {
	m_triggers = triggers;
	reset();
	std::cout << "[RaceManager] Inicialitzat amb " << m_triggers.size() << " checkpoints." << std::endl;
}

void RaceManager::reset() {
	m_state = RaceState::IDLE;
	m_currentTime = 0.0f;
	m_nextCheckpointIndex = 0; // Apunta al primer trigger del vector (ID 1 real)
	m_lapsCompleted = 0;
	m_totalCheckpoints = m_triggers.size();

	// Resetegem l'estat visual dels triggers
	for ( auto* t : m_triggers ) {
		if ( t ) t->setPassed(false);
	}
}

void RaceManager::update(float deltaTime, btRigidBody* droneBody) {
	// 1. Actualitzar temps si estem corrent
	if ( m_state == RaceState::RACING ) {
		m_currentTime += deltaTime;
	}

	// Si ja hem acabat o no tenim dron, no fem res més
	if ( m_state == RaceState::FINISHED || !droneBody || m_triggers.empty() ) return;

	// 2. Comprovar col·lisió NOMÉS amb el següent checkpoint
	// Així evitem haver de comprovar els 12 triggers a cada frame
	if ( m_nextCheckpointIndex < m_triggers.size() ) {

		Trigger* targetTrigger = m_triggers[ m_nextCheckpointIndex ];

		if ( targetTrigger && checkCollision(targetTrigger, droneBody) ) {
			handleCheckpointPassed(targetTrigger);
		}
	}
}

bool RaceManager::checkCollision(Trigger* trigger, btRigidBody* droneBody) {
	if ( !trigger || !trigger->getGhost() ) return false;

	btPairCachingGhostObject* ghost = trigger->getGhost();

	// Obtenir llista d'objectes que toquen el fantasma
	btAlignedObjectArray<btCollisionObject*>& overlappingObjects = ghost->getOverlappingPairs();
	int numOverlapping = overlappingObjects.size();

	for ( int i = 0; i < numOverlapping; i++ ) {
		btCollisionObject* obj = overlappingObjects[ i ];

		// Si l'objecte que toca l'arc és el nostre Dron
		if ( obj == droneBody ) {
			AudioManager::Get().PlaySFX("Audio/pass-drone.mp3");
			return true;
		}
	}
	return false;
}

void RaceManager::handleCheckpointPassed(Trigger* trigger) {
	// Si estem en IDLE i passem el PRIMER trigger -> START
	if ( m_state == RaceState::IDLE ) {
		// Assumim que el primer trigger del vector és l'inici
		if ( m_nextCheckpointIndex == 0 ) {
			m_state = RaceState::RACING;
			std::cout << ">>> CURSA INICIADA! GO GO GO! <<<" << std::endl;
		}
		else {
			// Si passem per un altre trigger abans de començar, ignorem
			return;
		}
	}

	// Lògica comuna (marcar com passat i avançar índex)
	trigger->setPassed(true);
	m_nextCheckpointIndex++;

	std::cout << "Checkpoint " << trigger->getID() << " passat! Temps: " << m_currentTime << "s" << std::endl;

	// Comprovar condició de victòria
	if ( m_nextCheckpointIndex >= m_triggers.size() ) {
		m_state = RaceState::FINISHED;
		std::cout << ">>> FINAL DE CURSA! TEMPS FINAL: " << m_currentTime << "s <<<" << std::endl;
	}
}