#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include "Shader.h"
#include "Mesh.h"  // CANVI: Incloure Mesh
#include "tiny_obj_loader.h"

class Model3D
{
public:
    Model3D();
    Model3D(std::string path);
    ~Model3D();

    bool loadModel(const std::string& ruta);

    // Posició / rotació / escala (models compatibles amb ambdues API)
    void setPosition(const glm::vec3& pos) { m_position = pos; updateModelMatrix(); }

    // Rotació en Euler (graus). Continua disponible per compatibilitat:
    void setRotation(const glm::vec3& rotDegrees) {
        m_rotation = rotDegrees;
        // sincronitza el quaternion
        m_rotationQuat = glm::quat(glm::radians(m_rotation));
        updateModelMatrix();
    }

    // Rotació directament amb quaternion (preferit)
    void setRotationQuat(const glm::quat& q) { m_rotationQuat = q; updateModelMatrix(); }
    glm::quat getRotationQuat() const { return m_rotationQuat; }

    void setScale(const glm::vec3& scl) { m_scale = scl; updateModelMatrix(); }
    void setupMesh();
    void draw(Shader& shader);
    glm::mat4 getModelMatrix() const { return m_modelMatrix; }

    // Getters
	const std::vector<Mesh>& getMeshes() const { return m_meshes; }
	std::vector<Mesh>& getMeshes() { return m_meshes; }
	glm::vec3 getPosition() const { return m_position; }
	glm::vec3 getScale() const { return m_scale; }

	void getCollisionData(std::vector<glm::vec3>& outVertices,
		std::vector<unsigned int>& outIndices) const
	{
		outVertices.clear();
		outIndices.clear();

		unsigned int vertexOffset = 0;

		for ( const auto& mesh : m_meshes ) {
			// Obtenir vèrtexs d'aquesta malla
			const auto& vertices = mesh.getVertices();

			// Transformar cada vèrtex amb la matriu del model
			for ( const auto& vertex : vertices ) {
				glm::vec4 transformed = m_modelMatrix * glm::vec4(vertex.position, 1.0f);
				outVertices.push_back(glm::vec3(transformed));
			}

			// Obtenir índexs d'aquesta malla
			const auto& indices = mesh.getIndices();
			for ( unsigned int index : indices ) {
				outIndices.push_back(index + vertexOffset);
			}

			vertexOffset += vertices.size();
		}
	}

private:
    void updateModelMatrix();
    
    bool loadOBJ(const std::string& ruta);
    bool loadGLTF(const std::string& ruta);

    std::vector<Mesh> m_meshes;

    glm::vec3 m_position;
    glm::vec3 m_rotation;      // euler en graus (per compatibilitat)
    glm::quat m_rotationQuat;  // rotació real utilitzada per la matriu
    glm::vec3 m_scale;
    glm::mat4 m_modelMatrix;
};
