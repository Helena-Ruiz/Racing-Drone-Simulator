#pragma once

#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <tiny_gltf.h>

class Shader;

enum class CurveType {
	BEZIER,
	CATMULL_ROM
};

class Curve {
private:
	std::vector<glm::vec3> controlPoints;

	GLuint VAO;
	GLuint VBO;
	GLsizei m_drawCount;

	void setupBuffers(const std::vector<glm::vec3>& generatedPoints);

public:
	Curve();
	~Curve();

	void AddPoint(const glm::vec3& pt);

	// Accés als punts de control
	const std::vector<glm::vec3>& GetControlPoints() const;
	size_t GetPointCount() const;

	bool LoadFromGLTF(const std::string& filename);

	glm::vec3 BezierPoint(size_t segmentIndex, float t) const;

	glm::vec3 CatmullRomPoint(size_t segmentIndex, float t) const;

	std::vector<glm::vec3> GenerateCurve(CurveType type, float step = 0.01f);

	void draw(Shader& shader) const;
};