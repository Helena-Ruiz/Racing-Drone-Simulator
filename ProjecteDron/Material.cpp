#include "Material.h"
#include "Texture.h"
#include <iostream>

bool Material::loadTexture(const std::string& path, std::shared_ptr<Texture>& texPtr, bool isSRGB)
{
    if (texPtr) return true;

    try {
        texPtr = std::make_shared<Texture>(path, isSRGB);
        return true;
    }
    catch (std::exception& e) {
        std::cerr << "Error loading texture " << path << ": " << e.what() << std::endl;
        return false;
    }
}