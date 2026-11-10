#include "Mesh.h"
#include <iostream>
#include "Defines.h"

Mesh::Mesh()
    : m_VAO(0), m_VBO(0), m_EBO(0), m_buffersInitialized(false)
{
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const Material& material)
    : m_vertices(vertices), m_indices(indices), m_material(material),
    m_VAO(0), m_VBO(0), m_EBO(0), m_buffersInitialized(false)
{
}

Mesh::~Mesh()
{
    cleanup();
}

void Mesh::setupBuffers()
{
    if (m_buffersInitialized) {
        cleanup();
    }

    // Generar buffers
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    // VBO - Dades dels vèrtexs
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex),
        m_vertices.data(), GL_STATIC_DRAW);

    // EBO - Índexs
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int),
        m_indices.data(), GL_STATIC_DRAW);

    // Atributs dels vèrtexs
    // Posició (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // 4. Tangent (NOU)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    // Coordenades de textura (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m_buffersInitialized = true;

    std::cout << "Mesh buffers inicialitzats - VAO: " << m_VAO
        << ", Vèrtexs: " << m_vertices.size()
        << ", Índexs: " << m_indices.size() << std::endl;
}

void Mesh::draw(Shader& shader)
{
	if ( !m_buffersInitialized || m_VAO == 0 ) return;

	shader.use();

	// 1. Uniforms per defecte
	shader.setVec4("defaultAlbedo", m_material.baseColorFactor);
	shader.setFloat("defaultRoughness", m_material.roughness);
	shader.setFloat("defaultMetallic", m_material.metallic);
	shader.setVec3("defaultEmissive", m_material.emissiveFactor);
	shader.setFloat("emissiveIntensity", 1.0f); // intensitat per defecte

	// 2. Flags per defecte
	shader.setBool("useAlbedoMap", false);
	shader.setBool("useNormalMap", false);
	shader.setBool("useMetallicRoughnessMap", false);
	shader.setBool("useAOMap", false);
	shader.setBool("useEmissiveMap", false);

	// 3. Vincular textures
	GLuint textureUnit = 0;

	if ( m_material.baseColorMap ) {
		m_material.baseColorMap->bind(textureUnit);
		shader.setInt("albedoMap", textureUnit);
		shader.setBool("useAlbedoMap", true);
		textureUnit++;
	}

	if ( m_material.normalMap ) {
		m_material.normalMap->bind(textureUnit);
		shader.setInt("normalMap", textureUnit);
		shader.setBool("useNormalMap", true);
		textureUnit++;
	}

	if ( m_material.metallicRoughnessMap ) {
		m_material.metallicRoughnessMap->bind(textureUnit);
		shader.setInt("metallicRoughnessMap", textureUnit);
		shader.setBool("useMetallicRoughnessMap", true);
		textureUnit++;
	}

	if ( m_material.aoMap ) {
		m_material.aoMap->bind(textureUnit);
		shader.setInt("aoMap", textureUnit);
		shader.setBool("useAOMap", true);
		textureUnit++;
	}

	if ( m_material.emissiveMap ) {
		m_material.emissiveMap->bind(textureUnit);
		shader.setInt("emissiveMap", textureUnit);
		shader.setBool("useEmissiveMap", true);
		textureUnit++;
	}

	// 4. Alpha / blending
	if ( m_material.alphaMode == AlphaMode::ALPHAMODE_BLEND ) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
	}
	else if ( m_material.alphaMode == AlphaMode::ALPHAMODE_MASK ) {
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		shader.setFloat("alphaCutoff", m_material.alphaCutoff);
	}
	else { // ALPHA_OPAQUE
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}

	// 6. Dibuixar
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, static_cast< GLsizei >( m_indices.size() ), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// 7. Restaurar estat OpenGL
	if ( m_material.alphaMode == AlphaMode::ALPHAMODE_BLEND )
		glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

}

void Mesh::cleanup()
{
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_EBO) {
        glDeleteBuffers(1, &m_EBO);
        m_EBO = 0;
    }
    m_buffersInitialized = false;
}