#include "Model3D.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <chrono>
#include "tiny_gltf.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

// Helper key + hash per a vèrtexs únics (OBJ)
struct VertexKey {
    int v;   // vertex index
    int vt;  // texcoord index
    int vn;  // normal index
    bool operator==(VertexKey const& o) const {
        return v == o.v && vt == o.vt && vn == o.vn;
    }
};

struct VertexKeyHash {
    size_t operator()(VertexKey const& k) const noexcept {
        size_t h1 = std::hash<int>()(k.v);
        size_t h2 = std::hash<int>()(k.vt);
        size_t h3 = std::hash<int>()(k.vn);
        size_t res = h1;
        res ^= (h2 + 0x9e3779b97f4a7c15ULL + (res << 6) + (res >> 2));
        res ^= (h3 + 0x9e3779b97f4a7c15ULL + (res << 6) + (res >> 2));
        return res;
    }
};

std::string urlDecode(const std::string& SRC) {
	std::string ret;
	char ch;
	int ii;
	for ( size_t i = 0; i < SRC.length(); i++ ) {
		if ( SRC[ i ] == '%' ) {
			if ( i + 2 < SRC.length() ) {
				std::istringstream hex(SRC.substr(i + 1, 2));
				if ( hex >> std::hex >> ii ) {
					ch = static_cast< char >( ii );
					ret += ch;
					i = i + 2;
				}
				else {
					ret += SRC[ i ];
				}
			}
			else {
				ret += SRC[ i ];
			}
		}
		else {
			ret += SRC[ i ];
		}
	}
	return ret;
}



bool Model3D::loadModel(const std::string& ruta) {
    std::string ext = ruta.substr(ruta.find_last_of('.') + 1);
    if (ext == "obj" || ext == "OBJ") {
        return loadOBJ(ruta);
    }
    else if (ext == "gltf" || ext == "glb" || ext == "GLTF" || ext == "GLB") {
        return loadGLTF(ruta);
    }
    else {
        std::cerr << "Format desconegut: " << ext << std::endl;
        return false;
    }
}

bool Model3D::loadGLTF(const std::string& ruta)
{
	using clock = std::chrono::high_resolution_clock;
	auto start_total = clock::now();

	std::cout << "=== CARREGANT MODEL AMB .GLTF/.GLB (USANT DADES DEL FITXER): " << ruta << " ===" << std::endl;

	auto urlDecode = [ ] (const std::string& SRC) -> std::string {
		std::string ret;
		char ch;
		int ii;
		for ( size_t i = 0; i < SRC.length(); i++ ) {
			if ( SRC[ i ] == '%' ) {
				if ( i + 2 < SRC.length() ) {
					std::istringstream hex(SRC.substr(i + 1, 2));
					if ( hex >> std::hex >> ii ) {
						ch = static_cast< char >( ii );
						ret += ch;
						i = i + 2;
					}
					else {
						ret += SRC[ i ];
					}
				}
				else {
					ret += SRC[ i ];
				}
			}
			else {
				ret += SRC[ i ];
			}
		}
		return ret;
		};

	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF loader;
	std::string err, warn;
	bool ret = false;

	if ( ruta.substr(ruta.size() - 4) == ".glb" )
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, ruta);
	else
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, ruta);

	if ( !warn.empty() ) std::cout << "WARN: " << warn << std::endl;
	if ( !err.empty() )  std::cerr << "ERR: " << err << std::endl;
	if ( !ret ) return false;

	m_meshes.clear();
	std::string base_dir = ruta.substr(0, ruta.find_last_of("/\\"));

	// 1. CARREGAR MATERIALS PBR
	std::vector<Material> materialsLoaded;
	materialsLoaded.reserve(gltfModel.materials.size());

	std::map<int, std::shared_ptr<Texture>> textureCache;

	auto getTexture = [ & ] (int texIndex, bool isSRGB) -> std::shared_ptr<Texture> {
		if ( texIndex < 0 || texIndex >= gltfModel.textures.size() ) return nullptr;
		if ( textureCache.count(texIndex) ) return textureCache[ texIndex ];

		int imgIndex = gltfModel.textures[ texIndex ].source;
		if ( imgIndex < 0 || imgIndex >= gltfModel.images.size() ) return nullptr;

		const auto& img = gltfModel.images[ imgIndex ];
		std::string fullPath;

		if ( !img.uri.empty() ) {
			fullPath = base_dir + "/" + urlDecode(img.uri);
		}
		else {
			std::cout << "Avís: Textura embeguda no suportada." << std::endl;
			return nullptr;
		}

		auto newTex = std::make_shared<Texture>(fullPath, isSRGB);
		textureCache[ texIndex ] = newTex;
		return newTex;
		};

	for ( const auto& mat : gltfModel.materials ) {
		Material loadedMat;
		loadedMat.name = mat.name;

		loadedMat.baseColorFactor = glm::vec4(1.0f);
		loadedMat.metallic = 0.0f;
		loadedMat.roughness = 1.0f;
		
		if ( !mat.alphaMode.empty() ) {
			if ( mat.alphaMode == "OPAQUE" ) loadedMat.alphaMode = AlphaMode::ALPHAMODE_OPAQUE;
			else if ( mat.alphaMode == "MASK" ) loadedMat.alphaMode = AlphaMode::ALPHAMODE_MASK;
			else if ( mat.alphaMode == "BLEND" ) loadedMat.alphaMode = AlphaMode::ALPHAMODE_BLEND;
			else loadedMat.alphaMode = AlphaMode::ALPHAMODE_OPAQUE;
		}
		else {
			loadedMat.alphaMode = AlphaMode::ALPHAMODE_OPAQUE;
		}

		if ( mat.values.count("baseColorFactor") ) {
			auto v = mat.values.at("baseColorFactor").ColorFactor();
			loadedMat.baseColorFactor = glm::vec4(v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ]);
		}
		if ( mat.values.count("metallicFactor") )
			loadedMat.metallic = ( float ) mat.values.at("metallicFactor").Factor();
		if ( mat.values.count("roughnessFactor") )
			loadedMat.roughness = ( float ) mat.values.at("roughnessFactor").Factor();
		if ( mat.additionalValues.count("emissiveFactor") ) {
			auto v = mat.additionalValues.at("emissiveFactor").ColorFactor();
			loadedMat.emissiveFactor = glm::vec3(v[ 0 ], v[ 1 ], v[ 2 ]);
		}

		if ( mat.values.count("baseColorTexture") )
			loadedMat.baseColorMap = getTexture(mat.values.at("baseColorTexture").TextureIndex(), true);

		if ( mat.additionalValues.count("emissiveTexture") )
			loadedMat.emissiveMap = getTexture(mat.additionalValues.at("emissiveTexture").TextureIndex(), true);

		if ( mat.values.count("metallicRoughnessTexture") )
			loadedMat.metallicRoughnessMap = getTexture(mat.values.at("metallicRoughnessTexture").TextureIndex(), false);

		if ( mat.additionalValues.count("normalTexture") )
			loadedMat.normalMap = getTexture(mat.additionalValues.at("normalTexture").TextureIndex(), false);

		if ( mat.additionalValues.count("occlusionTexture") )
			loadedMat.aoMap = getTexture(mat.additionalValues.at("occlusionTexture").TextureIndex(), false);

		materialsLoaded.push_back(std::move(loadedMat));
	}

	// 2. FUNCIÓ DE CÀRREGA DE GEOMETRIA
	auto loadPrimitiveData = [ & ](const tinygltf::Primitive& prim,
		std::vector<Mesh::Vertex>& vertices,
		std::vector<unsigned int>& indices)
		{
			const float* positions = nullptr;
			const float* normals = nullptr;
			const float* texcoords = nullptr;
			const float* tangents = nullptr;

			size_t count = 0;

			// Positions
			{
				const auto& accessor = gltfModel.accessors[ prim.attributes.at("POSITION") ];
				const auto& bufferView = gltfModel.bufferViews[ accessor.bufferView ];
				const auto& buffer = gltfModel.buffers[ bufferView.buffer ];
				positions = reinterpret_cast< const float* >( &buffer.data[ bufferView.byteOffset + accessor.byteOffset ] );
				count = accessor.count;
			}

			// Normals 
			if ( prim.attributes.count("NORMAL") ) {
				const auto& accessor = gltfModel.accessors[ prim.attributes.at("NORMAL") ];
				const auto& bufferView = gltfModel.bufferViews[ accessor.bufferView ];
				const auto& buffer = gltfModel.buffers[ bufferView.buffer ];
				normals = reinterpret_cast< const float* >( &buffer.data[ bufferView.byteOffset + accessor.byteOffset ] );
			}

			// TexCoords 
			if ( prim.attributes.count("TEXCOORD_0") ) {
				const auto& accessor = gltfModel.accessors[ prim.attributes.at("TEXCOORD_0") ];
				const auto& bufferView = gltfModel.bufferViews[ accessor.bufferView ];
				const auto& buffer = gltfModel.buffers[ bufferView.buffer ];
				texcoords = reinterpret_cast< const float* >( &buffer.data[ bufferView.byteOffset + accessor.byteOffset ] );
			}

			// Tangents
			if ( prim.attributes.count("TANGENT") ) {
				const auto& accessor = gltfModel.accessors[ prim.attributes.at("TANGENT") ];
				const auto& bufferView = gltfModel.bufferViews[ accessor.bufferView ];
				const auto& buffer = gltfModel.buffers[ bufferView.buffer ];
				tangents = reinterpret_cast< const float* >( &buffer.data[ bufferView.byteOffset + accessor.byteOffset ] );
			}

			// Inicialització de vèrtexs
			vertices.resize(count);
			for ( size_t i = 0; i < count; ++i ) {
				vertices[ i ].position = glm::vec3(positions[ i * 3 + 0 ], positions[ i * 3 + 1 ], positions[ i * 3 + 2 ]);

				// Normal
				if ( normals ) {
					vertices[ i ].normal = glm::vec3(normals[ i * 3 + 0 ], normals[ i * 3 + 1 ], normals[ i * 3 + 2 ]);
				}
				else {
					vertices[ i ].normal = glm::vec3(0.0f, 1.0f, 0.0f);
				}

				// Tangent
				if ( tangents ) {
					vertices[ i ].tangent = glm::vec4(
						tangents[ i * 4 + 0 ],
						tangents[ i * 4 + 1 ],
						tangents[ i * 4 + 2 ],
						tangents[ i * 4 + 3 ] // w component
					);
				}
				else {
					vertices[ i ].tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				}

				// TexCoord
				if ( texcoords ) {
					vertices[ i ].texCoord = glm::vec2(texcoords[ i * 2 + 0 ], texcoords[ i * 2 + 1 ]);
				}
				else {
					vertices[ i ].texCoord = glm::vec2(0.0f);
				}
			}

			// Índexs
			if ( prim.indices >= 0 ) {
				const auto& accessor = gltfModel.accessors[ prim.indices ];
				const auto& bufferView = gltfModel.bufferViews[ accessor.bufferView ];
				const auto& buffer = gltfModel.buffers[ bufferView.buffer ];
				indices.resize(accessor.count);

				if ( accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ) {
					const unsigned short* buf = reinterpret_cast< const unsigned short* >( &buffer.data[ bufferView.byteOffset + accessor.byteOffset ] );
					for ( size_t i = 0; i < accessor.count; ++i ) indices[ i ] = buf[ i ];
				}
				else if ( accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ) {
					const unsigned int* buf = reinterpret_cast< const unsigned int* >(&buffer.data[ bufferView.byteOffset + accessor.byteOffset ]);
					for ( size_t i = 0; i < accessor.count; ++i ) indices[ i ] = buf[ i ];
				}
				else if ( accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ) {
					const unsigned char* buf = reinterpret_cast< const unsigned char* >(&buffer.data[ bufferView.byteOffset + accessor.byteOffset ]);
					for ( size_t i = 0; i < accessor.count; ++i ) indices[ i ] = buf[ i ];
				}
			}

			std::cout << "  - Normals del fitxer: " << ( normals ? "SI" : "NO" )
				<< ", Tangents del fitxer: " << ( tangents ? "SI" : "NO" )
				<< ", TexCoords: " << ( texcoords ? "SI" : "NO" ) << std::endl;
		};

	// 3. PROCESSAR NODES I TRANSFORMACIONS
	auto getNodeTransform = [ ] (const tinygltf::Node& node) -> glm::mat4 {
		glm::mat4 mat(1.0f);
		if ( !node.matrix.empty() ) {
			std::vector<float> floatMatrix;
			for ( double d : node.matrix ) floatMatrix.push_back(( float ) d);
			mat = glm::make_mat4(floatMatrix.data());
		}
		else {
			if ( !node.translation.empty() )
				mat = glm::translate(mat, glm::vec3(( float ) node.translation[ 0 ], ( float ) node.translation[ 1 ], ( float ) node.translation[ 2 ]));
			if ( !node.rotation.empty() )
				mat *= glm::mat4_cast(glm::quat(( float ) node.rotation[ 3 ], ( float ) node.rotation[ 0 ], ( float ) node.rotation[ 1 ], ( float ) node.rotation[ 2 ]));
			if ( !node.scale.empty() )
				mat = glm::scale(mat, glm::vec3(( float ) node.scale[ 0 ], ( float ) node.scale[ 1 ], ( float ) node.scale[ 2 ]));
		}
		return mat;
		};

	std::function<void(int, glm::mat4)> processNode = [ & ] (int nodeIndex, glm::mat4 parentTransform) {
		const auto& node = gltfModel.nodes[ nodeIndex ];
		glm::mat4 nodeTransform = parentTransform * getNodeTransform(node);

		if ( node.mesh >= 0 ) {
			const auto& mesh = gltfModel.meshes[ node.mesh ];
			std::cout << "Processant mesh: " << mesh.name << " amb " << mesh.primitives.size() << " primitives" << std::endl;

			for ( const auto& prim : mesh.primitives ) {
				std::vector<Mesh::Vertex> vertices;
				std::vector<unsigned int> indices;

				loadPrimitiveData(prim, vertices, indices);

				glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(nodeTransform)));

				// Transformar vèrtexs a espai local del model
				for ( auto& vertex : vertices ) {
					// Posició
					glm::vec4 pos = nodeTransform * glm::vec4(vertex.position, 1.0f);
					vertex.position = glm::vec3(pos) / pos.w;

					// Normal 
					vertex.normal = glm::normalize(normalMatrix * vertex.normal);

					// Tangent
					glm::vec3 t = glm::normalize(normalMatrix * glm::vec3(vertex.tangent));
					vertex.tangent = glm::vec4(t, vertex.tangent.w);
				}

				Material mat = ( prim.material >= 0 && ( size_t ) prim.material < materialsLoaded.size() )
					? materialsLoaded[ prim.material ] : Material();

				m_meshes.emplace_back(vertices, indices, mat);
				std::cout << "  Primitive: " << vertices.size() << " vertices, "
					<< indices.size() << " indices, material: " << mat.name << std::endl;
			}
		}

		for ( int child : node.children )
			processNode(child, nodeTransform);
		};

	const tinygltf::Scene& scene = gltfModel.scenes[ gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0 ];
	for ( int sceneNode : scene.nodes )
		processNode(sceneNode, glm::mat4(1.0f));

	auto end_total = clock::now();
	std::chrono::duration<double> t_total = end_total - start_total;
	std::cout << "GLTF carregat (usant dades del fitxer) en " << t_total.count()
		<< " s. Malles: " << m_meshes.size() << std::endl;

	return true;
}
bool Model3D::loadOBJ(const std::string& ruta)
{
    using clock = std::chrono::high_resolution_clock;
    auto start_total = clock::now();

    std::cout << "=== CARREGANT MODEL AMB .OBJ: " << ruta << " ===" << std::endl;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    reader_config.mtl_search_path = ruta.substr(0, ruta.find_last_of("/\\")) + "/";

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(ruta, reader_config)) {
        if (!reader.Error().empty()) std::cerr << "ERROR TinyObj: " << reader.Error();
        return false;
    }

    // Warnings i errors de TinyObj
    if (!reader.Warning().empty()) std::cout << "WARN: " << reader.Warning() << std::endl;
    if (!reader.Error().empty())   std::cerr << "ERR: " << reader.Error() << std::endl;

    attrib = reader.GetAttrib();
    shapes = reader.GetShapes();
    materials = reader.GetMaterials();

    m_meshes.clear();

    // 1. CARREGAR MATERIALS (OBJ -> PBR)
    std::vector<Material> materialsLoaded;
    materialsLoaded.reserve(materials.size());
    std::string mtl_dir = ruta.substr(0, ruta.find_last_of("/\\"));

    std::map<std::string, std::shared_ptr<Texture>> textureCache;

    auto loadTex = [&](const std::string& filename, bool isSRGB) -> std::shared_ptr<Texture> {
        if (filename.empty()) return nullptr;
        if (textureCache.count(filename)) return textureCache[filename];

        std::string fullPath = mtl_dir + "/" + filename;
        auto tex = std::make_shared<Texture>(fullPath, isSRGB);
        textureCache[filename] = tex;
        return tex;
        };

    for (const auto& mat : materials) {
        Material loadedMat;
        loadedMat.name = mat.name;

        // Textures 
        loadedMat.baseColorMap = loadTex(mat.diffuse_texname, true);

        // Bump/Normal
        if (!mat.bump_texname.empty()) loadedMat.normalMap = loadTex(mat.bump_texname, false);
        else if (!mat.normal_texname.empty()) loadedMat.normalMap = loadTex(mat.normal_texname, false);

        // Specular map 
        loadedMat.metallicMap = loadTex(mat.specular_texname, false);

        // Shininess 
        loadedMat.roughnessMap = loadTex(mat.specular_highlight_texname, false);

        // Emissive
        loadedMat.emissiveMap = loadTex(mat.emissive_texname, true);

        // Factors
        loadedMat.baseColorFactor = glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f);
        loadedMat.emissiveFactor = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);

        float avgSpec = (mat.specular[0] + mat.specular[1] + mat.specular[2]) / 3.0f;
        loadedMat.metallic = (avgSpec > 0.5f) ? 1.0f : 0.0f;
        loadedMat.roughness = glm::clamp(1.0f - (mat.shininess / 100.0f), 0.05f, 1.0f);

        materialsLoaded.push_back(std::move(loadedMat));
    }

    // 2. PROCESSAR GEOMETRIA
    bool hasNormals = !attrib.normals.empty();
    bool hasTexcoords = !attrib.texcoords.empty();

    for (const auto& shape : shapes) {
        std::unordered_map<int, std::vector<unsigned int>> materialFaces;

        // Agrupem cares per material
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int matId = shape.mesh.material_ids[f];
            materialFaces[matId].push_back(static_cast<unsigned int>(f));
        }

        for (const auto& kv : materialFaces) {
            int matId = kv.first;
            const auto& faces = kv.second;

            std::vector<Mesh::Vertex> vertices;
            std::vector<unsigned int> indices;
            std::unordered_map<VertexKey, unsigned int, VertexKeyHash> uniqueVertices;

            for (unsigned int f : faces) {
                size_t fv = 3; 
                size_t offset = f * 3;

                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[offset + v];
                    VertexKey key{ idx.vertex_index, idx.texcoord_index, idx.normal_index };

                    if (uniqueVertices.count(key)) {
                        indices.push_back(uniqueVertices[key]);
                    }
                    else {
                        Mesh::Vertex vert{};

                        // Posició
                        vert.position = {
                            attrib.vertices[3 * key.v + 0],
                            attrib.vertices[3 * key.v + 1],
                            attrib.vertices[3 * key.v + 2]
                        };

                        // Normal
                        if (key.vn >= 0 && hasNormals) {
                            vert.normal = {
                                attrib.normals[3 * key.vn + 0],
                                attrib.normals[3 * key.vn + 1],
                                attrib.normals[3 * key.vn + 2]
                            };
                        }

                        // UV
                        if (key.vt >= 0 && hasTexcoords) {
                            vert.texCoord = {
                                attrib.texcoords[2 * key.vt + 0],
                                1.0f - attrib.texcoords[2 * key.vt + 1] 
                            };
                        }

                        vert.tangent = glm::vec4(0.0f);

                        unsigned int newIndex = static_cast<unsigned int>(vertices.size());
                        vertices.push_back(vert);
                        indices.push_back(newIndex);
                        uniqueVertices[key] = newIndex;
                    }
                }
            }

            // 3. CÀLCUL DE TANGENTS
            if (hasTexcoords && !vertices.empty()) {

                std::vector<glm::vec3> tempBitangents(vertices.size(), glm::vec3(0.0f));

                for (size_t i = 0; i < indices.size(); i += 3) {
                    unsigned int i0 = indices[i];
                    unsigned int i1 = indices[i + 1];
                    unsigned int i2 = indices[i + 2];

                    Mesh::Vertex& v0 = vertices[i0];
                    Mesh::Vertex& v1 = vertices[i1];
                    Mesh::Vertex& v2 = vertices[i2];

                    glm::vec3 edge1 = v1.position - v0.position;
                    glm::vec3 edge2 = v2.position - v0.position;
                    glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
                    glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

                    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                    if (!std::isfinite(f)) f = 0.0f;

                    glm::vec3 tangent;
                    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                    glm::vec3 bitangent;
                    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
                    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
                    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

                    // Acumulem Tangent
                    v0.tangent += glm::vec4(tangent, 0.0f);
                    v1.tangent += glm::vec4(tangent, 0.0f);
                    v2.tangent += glm::vec4(tangent, 0.0f);

                    // Acumulem Bitangent
                    tempBitangents[i0] += bitangent;
                    tempBitangents[i1] += bitangent;
                    tempBitangents[i2] += bitangent;
                }

                for (size_t i = 0; i < vertices.size(); ++i) {
                    Mesh::Vertex& v = vertices[i];
                    const glm::vec3& accumulatedBitangent = tempBitangents[i];

                    // Recuperem la normal i la tangent acumulada
                    glm::vec3 n = v.normal;
                    glm::vec3 t = glm::vec3(v.tangent); // Extraiem xyz del vec4

                    // Protecció contra normals zero
                    if (glm::length(n) < 0.001f) {
                        n = glm::vec3(0, 1, 0);
                        v.normal = n; // Actualitzem la normal si era invàlida
                    }
                    else {
                        n = glm::normalize(n);
                        v.normal = n; // Normalitzem per seguretat
                    }

                    glm::vec3 orthoT = t - n * glm::dot(n, t);

                    if (glm::length(orthoT) > 0.0001f) {
                        orthoT = glm::normalize(orthoT);
                    }
                    else {
                        orthoT = glm::vec3(1, 0, 0); // Fallback
                    }

                 
                    glm::vec3 crossNT = glm::cross(n, orthoT);
                    float w = (glm::dot(crossNT, accumulatedBitangent) < 0.0f) ? -1.0f : 1.0f;

                    v.tangent = glm::vec4(orthoT, w);
                }
            }

            Material mat = (matId >= 0 && matId < materialsLoaded.size()) ? materialsLoaded[matId] : Material();
            m_meshes.emplace_back(vertices, indices, mat);
        }
    }

    auto end_total = clock::now();
    std::chrono::duration<double> t_total = end_total - start_total;
    std::cout << "OBJ carregat en " << t_total.count() << " s. Malles: " << m_meshes.size() << std::endl;

    return true;
}

Model3D::Model3D()
    : m_position(0.0f), m_rotation(0.0f), m_rotationQuat(1.0f, 0.0f, 0.0f, 0.0f),
    m_scale(1.0f), m_modelMatrix(1.0f)
{
}

Model3D::Model3D(std::string path)
{
    m_position = glm::vec3(0.0f);
    m_rotation = glm::vec3(0.0f);
    m_rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_scale = glm::vec3(1.0f);
    m_modelMatrix = glm::mat4(1.0f);
	loadModel(path);
	setupMesh();
}

Model3D::~Model3D()
{
}

void Model3D::setupMesh()
{
    for (auto& mesh : m_meshes) {
        mesh.setupBuffers();
    }
}

void Model3D::draw(Shader& shader)
{
	shader.use();
	updateModelMatrix();
	shader.setMat4("model", m_modelMatrix);

	shader.setVec3("lightDir", glm::vec3(-0.5f, -1.0f, -0.5f));
	shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.9f));

	for (auto& mesh : m_meshes) {
		mesh.draw(shader);
	}
}

void Model3D::updateModelMatrix()
{
    m_modelMatrix = glm::mat4(1.0f);
    m_modelMatrix = glm::translate(m_modelMatrix, m_position);
    m_modelMatrix *= glm::toMat4(m_rotationQuat);
    m_modelMatrix = glm::scale(m_modelMatrix, m_scale);
}