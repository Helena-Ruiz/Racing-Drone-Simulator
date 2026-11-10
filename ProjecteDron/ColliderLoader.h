#pragma once
#include <btBulletDynamicsCommon.h>
#include <vector>
#include <glm/glm.hpp>

/*
	Crea colliders a partir de les dades dels teus Models3D
*/

class ColliderLoader
{
public:
	// Crea un collider estàtic a partir dels vèrtexs i índexs
	static btRigidBody* createFromMeshData(
		const std::vector<glm::vec3>& vertices,
		const std::vector<unsigned int>& indices
	);
};