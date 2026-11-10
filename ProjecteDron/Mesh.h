#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "Shader.h"
#include "Material.h"

class Mesh
{
public:
    // Estructura del vèrtex
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
		glm::vec4 tangent;
    };

    Mesh();
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const Material& material);
    ~Mesh();

    // Configuració i renderització
    void setupBuffers();
    void draw(Shader& shader);
    void cleanup();

    // Getters i setters
    void setMaterial(const Material& material) { m_material = material; }
    Material& getMaterial() { return m_material; }
    const Material& getMaterial() const { return m_material; }

    bool isInitialized() const { return m_VAO != 0; }
    size_t getVertexCount() const { return m_vertices.size(); }
    size_t getIndexCount() const { return m_indices.size(); }

	const std::vector<Vertex>& getVertices() const { return m_vertices; }        //bullet
	const std::vector<unsigned int>& getIndices() const { return m_indices; }
	std::vector<glm::vec3> getPositions() const
	{
		std::vector<glm::vec3> positions;
		positions.reserve(m_vertices.size());
		for ( const auto& vertex : m_vertices ) 
		{
			positions.push_back(vertex.position);
		}
		return positions;
	}

private:
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    Material m_material;

    GLuint m_VAO, m_VBO, m_EBO;
    bool m_buffersInitialized;
};