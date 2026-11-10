#include "Trigger.h"

void Trigger::initPhysics()
{
	// 1. Validem que tenim model
	if ( !m_model ) {
		std::cerr << "Error: Intentant crear física per Trigger sense model!" << std::endl;
		return;
	}

	// 2. Creem l'objecte Ghost
	m_ghost = new btPairCachingGhostObject();

	// 3. Obtenim els vèrtexs del Model3D
	std::vector<glm::vec3> vertices;
	std::vector<unsigned int> indices;

	// Recuperem els vèrtexs "bruts" del model (sense transformar encara)
	m_model->getCollisionData(vertices, indices);

	// 4. Creem un CONVEX HULL SHAPE
	btConvexHullShape* shape = new btConvexHullShape();

	glm::vec3 scale = m_model->getScale();

	for ( const auto& v : vertices ) {
		btVector3 bPoint(v.x * scale.x, v.y * scale.y, v.z * scale.z);
		shape->addPoint(bPoint);
	}

	// Optimització important per Convex Hull
	shape->optimizeConvexHull();

	m_ghost->setCollisionShape(shape);

	// 5. Posició i Rotació
	// Ara hem de posar el Ghost al mateix lloc que el model visual
	btTransform t;
	t.setIdentity();

	glm::vec3 pos = m_model->getPosition();
	glm::quat rot = m_model->getRotationQuat();

	t.setOrigin(btVector3(pos.x, pos.y, pos.z));
	t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

	m_ghost->setWorldTransform(t);

	// 6. Flags (Igual que abans: NO CONTACT RESPONSE perquè sigui un fantasma)
	m_ghost->setCollisionFlags(m_ghost->getCollisionFlags() |
		btCollisionObject::CF_NO_CONTACT_RESPONSE |
		btCollisionObject::CF_STATIC_OBJECT);

	m_ghost->setUserPointer(this);
}