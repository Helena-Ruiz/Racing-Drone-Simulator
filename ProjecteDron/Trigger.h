#pragma once
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include "Model3D.h"

class Trigger {
private:
	int m_id;

	Model3D* m_model = nullptr;
	btPairCachingGhostObject* m_ghost = nullptr;
	btCollisionShape* m_shape = nullptr;
	bool m_passed = false;

public:

	Trigger(int id, Model3D* model)
		: m_id(id)
	{
		m_model = model;
		btTriangleMesh* triMesh = new btTriangleMesh();
		for ( auto& mesh : model->getMeshes()) {
			for ( size_t i = 0; i < mesh.getIndices().size(); i += 3 ) {
				glm::vec3 v0 = mesh.getVertices()[ mesh.getIndices()[ i ] ].position;
				glm::vec3 v1 = mesh.getVertices()[ mesh.getIndices()[ i + 1 ] ].position;
				glm::vec3 v2 = mesh.getVertices()[ mesh.getIndices()[ i + 2 ] ].position;
				triMesh->addTriangle(
					btVector3(v0.x, v0.y, v0.z),
					btVector3(v1.x, v1.y, v1.z),
					btVector3(v2.x, v2.y, v2.z),
					true
				);
			}
		}

		m_shape = new btBvhTriangleMeshShape(triMesh, true);
		m_ghost = new btPairCachingGhostObject();
		m_ghost->setCollisionShape(m_shape);
		m_ghost->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);

		m_ghost->setWorldTransform(btTransform(
			btQuaternion(0, 0, 0, 1),
			btVector3(model->getPosition().x, model->getPosition().y, model->getPosition().z)
		));
	}

	~Trigger() {
		delete m_ghost;
		delete m_shape;
		if (m_model) delete m_model;
	}

	void reset() { m_passed = false; }

	void initPhysics();

	int getID() const { return m_id; }
	Model3D* getModel() { return m_model; }
	btPairCachingGhostObject* getGhost() { return m_ghost; }
	bool getPassed() const { return m_passed; }
	void setPassed(bool passed) { m_passed = passed; }
};
