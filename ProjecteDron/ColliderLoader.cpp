#include "ColliderLoader.h"
#include <iostream>

btRigidBody* ColliderLoader::createFromMeshData(
	const std::vector<glm::vec3>& vertices,
	const std::vector<unsigned int>& indices)
{
	if ( vertices.empty() || indices.empty() || indices.size() % 3 != 0 ) {
		std::cout << "[ColliderLoader] Avís: Dades de malla invàlides o buides" << std::endl;
		return nullptr;
	}

	// Crear triangle mesh
	btTriangleMesh* triMesh = new btTriangleMesh();
	size_t triangleCount = 0;

	for ( size_t i = 0; i < indices.size(); i += 3 ) {
		if ( indices[ i ] < vertices.size() &&
			indices[ i + 1 ] < vertices.size() &&
			indices[ i + 2 ] < vertices.size() ) {

			const glm::vec3& v0 = vertices[ indices[ i ] ];
			const glm::vec3& v1 = vertices[ indices[ i + 1 ] ];
			const glm::vec3& v2 = vertices[ indices[ i + 2 ] ];

			triMesh->addTriangle(
				btVector3(v0.x, v0.y, v0.z),
				btVector3(v1.x, v1.y, v1.z),
				btVector3(v2.x, v2.y, v2.z)
			);
			triangleCount++;
		}
	}

	if ( triangleCount == 0 ) {
		delete triMesh;
		return nullptr;
	}

	// Crear forma de col·lisió
	btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(triMesh, true);

	// Transformació
	btTransform transform;
	transform.setIdentity();
	btDefaultMotionState* motionState = new btDefaultMotionState(transform);

	// Crear cos rígid (massa 0 = estàtic)
	btRigidBody::btRigidBodyConstructionInfo info(0.0f, motionState, shape);
	btRigidBody* body = new btRigidBody(info);

	// Marcar com a objecte estàtic
	body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
	body->setActivationState(DISABLE_DEACTIVATION);

	std::cout << "[ColliderLoader] Creat collider amb "
		<< triangleCount << " triangles" << std::endl;

	return body;
}