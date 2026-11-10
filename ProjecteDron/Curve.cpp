#include "Curve.h"
#include "Shader.h" 
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Curve::Curve()
	: VAO(0), VBO(0), m_drawCount(0)
{
}

Curve::~Curve() {
	if ( VAO ) glDeleteVertexArrays(1, &VAO);
	if ( VBO ) glDeleteBuffers(1, &VBO);
}

void Curve::AddPoint(const glm::vec3& pt) {
	controlPoints.push_back(pt);
}

const std::vector<glm::vec3>& Curve::GetControlPoints() const {
	return controlPoints;
}

size_t Curve::GetPointCount() const {
	return controlPoints.size();
}

bool Curve::LoadFromGLTF(const std::string& filename) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	std::cout << "[Curve] Loading GLTF file: " << filename << std::endl;
	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);

	if ( !warn.empty() ) std::cout << "[Curve] GLTF Warning: " << warn << "\n";
	if ( !err.empty() ) std::cerr << "[Curve] GLTF Error: " << err << "\n";
	if ( !ret ) {
		std::cerr << "[Curve] Failed to load GLTF file!" << std::endl;
		return false;
	}

	if ( model.meshes.empty() ) return false;

	const auto& mesh = model.meshes[ 0 ];
	if ( mesh.primitives.empty() ) return false;

	const auto& prim = mesh.primitives[ 0 ];
	auto it = prim.attributes.find("POSITION");
	if ( it == prim.attributes.end() ) return false;

	const tinygltf::Accessor& accessor = model.accessors[ it->second ];
	const tinygltf::BufferView& view = model.bufferViews[ accessor.bufferView ];
	const tinygltf::Buffer& buffer = model.buffers[ view.buffer ];

	controlPoints.clear();
	const float* posData = reinterpret_cast< const float* >( &buffer.data[ view.byteOffset + accessor.byteOffset ] );

	size_t stride = accessor.ByteStride(view) ? accessor.ByteStride(view) / sizeof(float) : 3;

	for ( size_t i = 0; i < accessor.count; i++ ) {
		float x = posData[ i * stride + 0 ];
		float y = posData[ i * stride + 1 ];
		float z = posData[ i * stride + 2 ];
		controlPoints.emplace_back(x, y, z);
	}

	std::cout << "[Curve] Total control points loaded: " << controlPoints.size() << std::endl;
	return true;
}

glm::vec3 Curve::BezierPoint(size_t segmentIndex, float t) const {
	if ( segmentIndex + 3 >= controlPoints.size() ) return glm::vec3(0.0f);

	const glm::vec3& p0 = controlPoints[ segmentIndex + 0 ];
	const glm::vec3& p1 = controlPoints[ segmentIndex + 1 ];
	const glm::vec3& p2 = controlPoints[ segmentIndex + 2 ];
	const glm::vec3& p3 = controlPoints[ segmentIndex + 3 ];

	float u = 1.0f - t;
	return ( u * u * u ) * p0 + ( 3 * u * u * t ) * p1 + ( 3 * u * t * t ) * p2 + ( t * t * t ) * p3;
}

glm::vec3 Curve::CatmullRomPoint(size_t segmentIndex, float t) const {
	glm::vec3 p0 = ( segmentIndex == 0 ) ? controlPoints[ 0 ] : controlPoints[ segmentIndex - 1 ];
	glm::vec3 p1 = controlPoints[ segmentIndex ];
	glm::vec3 p2 = ( segmentIndex + 1 < controlPoints.size() ) ? controlPoints[ segmentIndex + 1 ] : controlPoints.back();
	glm::vec3 p3 = ( segmentIndex + 2 < controlPoints.size() ) ? controlPoints[ segmentIndex + 2 ] : controlPoints.back();

	float t2 = t * t;
	float t3 = t2 * t;

	return 0.5f * (
		( 2.0f * p1 ) +
		( -p0 + p2 ) * t +
		( 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 ) * t2 +
		( -p0 + 3.0f * p1 - 3.0f * p2 + p3 ) * t3
		);
}

std::vector<glm::vec3> Curve::GenerateCurve(CurveType type, float step) {
	std::vector<glm::vec3> curvePoints;
	if ( controlPoints.size() < 4 && type == CurveType::BEZIER ) return curvePoints;
	if ( controlPoints.size() < 2 && type == CurveType::CATMULL_ROM ) return curvePoints;

	if ( type == CurveType::BEZIER ) {
		for ( size_t i = 0; i + 3 < controlPoints.size(); i += 3 ) {
			for ( float t = 0; t <= 1.0f; t += step ) {
				curvePoints.push_back(BezierPoint(i, t));
			}
		}
	}
	else if ( type == CurveType::CATMULL_ROM ) {
		for ( size_t i = 0; i < controlPoints.size() - 1; i++ ) {
			for ( float t = 0; t <= 1.0f; t += step ) {
				curvePoints.push_back(CatmullRomPoint(i, t));
			}
		}
	}

	setupBuffers(curvePoints);

	return curvePoints;
}

void Curve::setupBuffers(const std::vector<glm::vec3>& generatedPoints) {
	m_drawCount = ( GLsizei ) generatedPoints.size();
	if ( m_drawCount == 0 ) return;

	if ( VAO ) glDeleteVertexArrays(1, &VAO);
	if ( VBO ) glDeleteBuffers(1, &VBO);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, generatedPoints.size() * sizeof(glm::vec3), generatedPoints.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), ( void* ) 0);

	glBindVertexArray(0);
}

void Curve::draw(Shader& shader) const {
	if ( VAO == 0 || m_drawCount == 0 ) return;

	shader.use();

	glLineWidth(3.0f);

	glBindVertexArray(VAO);
	glDrawArrays(GL_LINE_STRIP, 0, m_drawCount);
	glBindVertexArray(0);

	glLineWidth(1.0f);
}