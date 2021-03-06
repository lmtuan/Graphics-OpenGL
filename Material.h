#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <string>
#include <glm/glm.hpp>


////////////////////////////////////////////////////////////////////////////////
/// Contains paramters used for Blinn-Phong shading of a surface
struct Material {
  /// Reflection of ambient light
  glm::vec3 ka;
  /// Reflection of diffuse light
  glm::vec3 kd;
  /// Reflection of specular light
  glm::vec3 ks;
  /// Affect the size of specular spot
  float shininess;
  /// Reflection of light from other object, used for mirror like surface
  glm::vec3 kr;
  /// Emission of light
  glm::vec3 ke;
  /// Alpha transparency (0 is full transparent)
  float transparency = 1;
};

////////////////////////////////////////////////////////////////////////////////
/// Configure the material of a surface, either through texture mapping or
/// default material
struct MaterialConfig {
  bool hasTransparency = false;
  bool hasKdMap = false;
  bool hasKsMap = false;
  bool hasKeMap = false;
  bool hasNormalMap = false;
  bool hasParallaxMap = false;
  std::string kdTextureFile;
  std::string ksTextureFile;
  std::string keTextureFile;
  std::string normalTextureFile;
  std::string parallaxTextureFile;
  Material defaultMaterial;
};

#endif // MATERIAL_H_