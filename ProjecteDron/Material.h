#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "Texture.h"
#include "Defines.h"

class Material {
public:
    std::string name;

    // ============================
    // TEXTURES
    // ============================

    // Legacy (OBJ / Phong)
    std::shared_ptr<Texture> ambientMap;
    std::shared_ptr<Texture> diffuseMap;
    std::shared_ptr<Texture> specularMap;
    std::shared_ptr<Texture> shininessMap;

    // PBR Standard
    std::shared_ptr<Texture> baseColorMap;
    std::shared_ptr<Texture> metallicMap;
    std::shared_ptr<Texture> roughnessMap;
    std::shared_ptr<Texture> metallicRoughnessMap; // GLTF Packed (G=Rough, B=Metal)
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> aoMap;
    std::shared_ptr<Texture> emissiveMap;

    // PBR Advanced
    std::shared_ptr<Texture> transmissionMap;
    std::shared_ptr<Texture> clearCoatMap;
    std::shared_ptr<Texture> clearCoatRoughnessMap;

    // ============================
    // FACTORS & VALORS
    // ============================

    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    glm::vec3 emissiveFactor = glm::vec3(0.0f);

    // Per defecte PBR genèric: Plàstic/Dielectric (Metallic 0) i Mate (Roughness 1)
    // Nota: GLTF per defecte usa 1.0, però per un motor 0.0 sol ser millor visualment si no hi ha dades.
    float metallic = 0.0f;
    float roughness = 1.0f;
    float occlusionStrength = 1.0f;

    // Transparència i Render State
    AlphaMode alphaMode = AlphaMode::ALPHAMODE_OPAQUE;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;

    // Advanced extensions
    float transmission = 0.0f;
    float clearCoat = 0.0f;
    float clearCoatRoughness = 0.0f;

    Material() = default;

    // ============================
    // LOADERS (Amb gestió sRGB automàtica)
    // ============================

    // --- Colors Visuals (sRGB = true) ---
    bool loadBaseColor(const std::string& path) { return loadTexture(path, baseColorMap, true); }
    bool loadDiffuse(const std::string& path) { return loadTexture(path, diffuseMap, true); }
    bool loadEmissive(const std::string& path) { return loadTexture(path, emissiveMap, true); }
    bool loadAmbient(const std::string& path) { return loadTexture(path, ambientMap, true); }

    // --- Dades Lineals (sRGB = false) ---
    bool loadMetallicRoughness(const std::string& path) { return loadTexture(path, metallicRoughnessMap, false); }
    bool loadMetallic(const std::string& path) { return loadTexture(path, metallicMap, false); }
    bool loadRoughness(const std::string& path) { return loadTexture(path, roughnessMap, false); }
    bool loadNormal(const std::string& path) { return loadTexture(path, normalMap, false); }
    bool loadAO(const std::string& path) { return loadTexture(path, aoMap, false); }

    // Specular (Map) en workflows antics sol ser lineal (intensitat), però de vegades color.
    // En OBJ clàssic sovint es tracta com a dades.
    bool loadSpecular(const std::string& path) { return loadTexture(path, specularMap, false); }
    bool loadShininess(const std::string& path) { return loadTexture(path, shininessMap, false); }

private:
    // IMPORTANT: Afegeix 'bool isSRGB' a la implementació al .cpp
    bool loadTexture(const std::string& path, std::shared_ptr<Texture>& texPtr, bool isSRGB);
};