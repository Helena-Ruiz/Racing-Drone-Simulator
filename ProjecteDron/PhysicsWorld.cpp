#include "PhysicsWorld.h"
#include <iostream>

PhysicsWorld::PhysicsWorld()
	: m_broadphase(nullptr), m_collisionConfig(nullptr),
	m_dispatcher(nullptr), m_solver(nullptr),
	m_world(nullptr), m_droneBody(nullptr),
	m_droneShape(nullptr), m_droneMotionState(nullptr)
{
}

PhysicsWorld::~PhysicsWorld()
{
	clearStaticColliders();

	if ( m_droneBody && m_world ) {
		m_world->removeRigidBody(m_droneBody);
	}

	if ( m_droneBody ) {
		delete m_droneBody->getMotionState();
		delete m_droneBody->getCollisionShape();
		delete m_droneBody;
	}

	if ( m_droneMotionState ) delete m_droneMotionState;
	if ( m_droneShape ) delete m_droneShape;

	if ( m_world ) delete m_world;
	if ( m_solver ) delete m_solver;
	if ( m_dispatcher ) delete m_dispatcher;
	if ( m_collisionConfig ) delete m_collisionConfig;
	if ( m_broadphase ) delete m_broadphase;
}

void PhysicsWorld::init()
{
	m_broadphase = new btDbvtBroadphase();
	m_broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

	m_collisionConfig = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfig);
	m_solver = new btSequentialImpulseConstraintSolver();

	m_world = new btDiscreteDynamicsWorld(
		m_dispatcher, m_broadphase, m_solver, m_collisionConfig
	);

	m_world->setGravity(btVector3(0, -9.81f, 0));

	std::cout << "[PhysicsWorld] Inicialitzat correctament" << std::endl;
}

void PhysicsWorld::addGhostObject(btPairCachingGhostObject* ghost)
{
	if ( ghost && m_world ) {
		m_world->addCollisionObject(ghost, COL_TRIGGER, COL_DRONE);
	}
}

void PhysicsWorld::createDroneBody(const glm::vec3& startPos)
{
	float radius = 0.05f;
	float height = 0.05f;

	m_droneShape = new btCapsuleShape(radius, height);
	m_droneShape->setMargin(0.02f);

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(startPos.x, startPos.y, startPos.z));

	m_droneMotionState = new btDefaultMotionState(transform);

	btRigidBody::btRigidBodyConstructionInfo info(
		0.0f,
		m_droneMotionState,
		m_droneShape
	);

	m_droneBody = new btRigidBody(info);

	m_droneBody->setCollisionFlags(
		btCollisionObject::CF_KINEMATIC_OBJECT
	);
	m_droneBody->setActivationState(DISABLE_DEACTIVATION);

	m_droneBody->setCcdMotionThreshold(radius);
	m_droneBody->setCcdSweptSphereRadius(radius * 0.5f);

	m_world->addRigidBody(
		m_droneBody,
		COL_DRONE,
		COL_STATIC | COL_TRIGGER
	);
}



void PhysicsWorld::syncDronePosition(const glm::vec3& pos)
{
	if ( !m_droneBody ) return;

	btTransform transform;
	m_droneBody->getMotionState()->getWorldTransform(transform);
	transform.setOrigin(btVector3(pos.x, pos.y, pos.z));

	m_droneBody->getMotionState()->setWorldTransform(transform);
	m_droneBody->setWorldTransform(transform);

	m_world->updateSingleAabb(m_droneBody);
}

void PhysicsWorld::syncDroneTransform(const glm::vec3& pos, const glm::quat& rotation)
{
	if ( !m_droneBody ) return;

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(pos.x, pos.y, pos.z));

	btQuaternion btRot(rotation.x, rotation.y, rotation.z, rotation.w);

	// Si el quaternio �s zero, posem identitat
	if ( rotation.w == 0.0f && rotation.x == 0.0f &&
		rotation.y == 0.0f && rotation.z == 0.0f ) {
		btRot = btQuaternion(0, 0, 0, 1);
	}

	transform.setRotation(btRot);

	m_droneBody->getMotionState()->setWorldTransform(transform);
	m_droneBody->setWorldTransform(transform);

	m_world->updateSingleAabb(m_droneBody);
}

void PhysicsWorld::addRigidBody(btRigidBody* body)
{
	if ( body && m_world ) {
		m_world->addRigidBody(body, COL_STATIC, COL_DRONE);
	}
}

void PhysicsWorld::stepSimulation(float dt)
{
	if ( m_world ) {
		m_world->stepSimulation(dt, 20, 1.0f / 240.0f);
	}
}

bool PhysicsWorld::droneHit(glm::vec3& collisionNormal, float& penetrationDepth)
{
	if ( !m_droneBody || !m_world ) return false;

	int numManifolds = m_world->getDispatcher()->getNumManifolds();
	bool hit = false;

	penetrationDepth = 0.0f;

	for ( int i = 0; i < numManifolds; i++ ) {
		btPersistentManifold* manifold = m_world->getDispatcher()->getManifoldByIndexInternal(i);

		const btCollisionObject* objA = manifold->getBody0();
		const btCollisionObject* objB = manifold->getBody1();

		// Comprovem si un dels dos �s el dron
		if ( objA == m_droneBody || objB == m_droneBody ) {

			// --- NOVA PART: IDENTIFICAR L'ALTRE OBJECTE ---
			const btCollisionObject* otherObj = ( objA == m_droneBody ) ? objB : objA;

			// Si l'altre objecte �s un TRIGGER (t� la flag NO_CONTACT_RESPONSE), L'IGNOREM.
			// Aix� fa que la funci� no reporti col�lisi� encara que es toquin.
			if ( otherObj->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE ) {
				continue; // Salta al seg�ent, aix� no �s un xoc real
			}
			// ----------------------------------------------

			int numContacts = manifold->getNumContacts();

			for ( int j = 0; j < numContacts; j++ ) {
				const btManifoldPoint& pt = manifold->getContactPoint(j);

				if ( pt.getDistance() < 0.0f ) {
					float currentDepth = -pt.getDistance();
					if ( currentDepth > penetrationDepth )
					{
						penetrationDepth = currentDepth;
						hit = true;

						btVector3 normal = pt.m_normalWorldOnB;
						if ( objA == m_droneBody ) {
							normal = -normal;
						}
						collisionNormal = glm::vec3(normal.x(), normal.y(), normal.z());
					}
				}
			}
		}
	}

	return hit;
}
void PhysicsWorld::clearStaticColliders()
{
	if ( !m_world ) return;

	// Eliminar tots els objectes excepte el drone
	for ( int i = m_world->getNumCollisionObjects() - 1; i >= 0; i-- ) {
		btCollisionObject* obj = m_world->getCollisionObjectArray()[ i ];
		if ( obj != m_droneBody ) {
			btRigidBody* body = btRigidBody::upcast(obj);
			if ( body ) {
				m_world->removeCollisionObject(obj);
				delete body->getMotionState();
				delete body->getCollisionShape();
				delete body;
			}
		}
	}

	std::cout << "[PhysicsWorld] Static colliders netejats" << std::endl;
}

bool PhysicsWorld::checkSweep(const glm::vec3& startPos, const glm::vec3& endPos, const glm::quat& rotation, glm::vec3& outNormal, float& outFraction)
{
	if ( !m_world || !m_droneShape ) return false;

	// 1. Configurar transformació INICIAL i FINAL
	btTransform start, end;
	start.setIdentity();
	end.setIdentity();

	// Passem de GLM a Bullet
	start.setOrigin(btVector3(startPos.x, startPos.y, startPos.z));
	end.setOrigin(btVector3(endPos.x, endPos.y, endPos.z));

	// Mantenim la rotació durant el moviment (important si la càpsula no és una esfera)
	btQuaternion btRot(rotation.x, rotation.y, rotation.z, rotation.w);
	start.setRotation(btRot);
	end.setRotation(btRot);

	// 2. Callback per trobar la primera col·lisió
	btCollisionWorld::ClosestConvexResultCallback callback(start.getOrigin(), end.getOrigin());

	// 3. FILTRES: Amb què volem xocar?
	// COL_STATIC = parets, terra. 
	// IMPORTANT: No posem COL_TRIGGER perquè no volem rebotar contra els checkpoints.
	callback.m_collisionFilterMask = COL_STATIC;
	callback.m_collisionFilterGroup = COL_DRONE;

	// 4. Executar el test
	// "Llança" la forma del dron (m_droneShape) des de start fins a end
	m_world->convexSweepTest(m_droneShape, start, end, callback);

	// 5. Resultats
	if ( callback.hasHit() ) {
		btVector3 normal = callback.m_hitNormalWorld;
		outNormal = glm::vec3(normal.x(), normal.y(), normal.z());

		// m_closestHitFraction va de 0.0 (inici) a 1.0 (final)
		outFraction = callback.m_closestHitFraction;
		return true;
	}

	return false;
}
