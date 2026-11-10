#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

// Categories de col·lisió (Bitmasks)
#define COL_NOTHING 0
#define COL_DRONE   (1 << 0) // 0001
#define COL_STATIC  (1 << 1) // 0010 (Parets, Terra, Estadi)
#define COL_TRIGGER (1 << 2) // 0100

/*
	Classe principal encarregada de:
	- Crear i gestionar el món de Bullet
	- Crear el cos del dron (massa 0, kinematic)
	- Actualitzar col·lisions
	- Sincronitzar la posició del dron
*/


class PhysicsWorld
{
public:
	PhysicsWorld();
	~PhysicsWorld();

	// Inicialitza Bullet
	void init();

	void addGhostObject(btPairCachingGhostObject* ghost);

	// Avança la simulació de col·lisions
	void stepSimulation(float dt);

	// Crea el cos físic del dron (kinematic)
	void createDroneBody(const glm::vec3& startPos);

	// Sincronitza només la posició (com tens al teu codi)
	void syncDronePosition(const glm::vec3& pos);

	// Alternativa: sincronitza posició i rotació
	void syncDroneTransform(const glm::vec3& pos, const glm::quat& rotation);

	// Afegeix un collider estàtic
	void addRigidBody(btRigidBody* body);

	// Comprova si el dron ha col·lisionat (mètode que ja tens)
	bool droneHit(glm::vec3& collisionNormal, float& penetrationDepth);

	// Neteja tots els objectes excepte el dron
	void clearStaticColliders();

	bool checkSweep(const glm::vec3& startPos, const glm::vec3& endPos, const glm::quat& rotation, glm::vec3& outNormal, float& outFraction);

	btRigidBody* getDroneRigidBody() { return m_droneBody; }
	btDiscreteDynamicsWorld* getBulletWorld() { return m_world; }

private:
	btBroadphaseInterface* m_broadphase;
	btDefaultCollisionConfiguration* m_collisionConfig;
	btCollisionDispatcher* m_dispatcher;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld* m_world;

	btRigidBody* m_droneBody;
	btConvexShape* m_droneShape;
	btDefaultMotionState* m_droneMotionState;
};