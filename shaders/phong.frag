////////////////////////////////////////////////////////////////////////////////
// Phong shader - interpolates the normal for each fragment, then does the
// lighting computation
////////////////////////////////////////////////////////////////////////////////

#version 330

////////////////////////////////////////////////////////////////////////////////
// Uniforms

// Properties of the scene
#define MAX_LIGHTS 32
struct Light {
  vec3 pos; // position of light in world coordinate
  vec3 ia;  // ambient intensity
  vec3 id;  // diffuse intensity
  vec3 is;  // specular intensity
};

uniform int   numLights;          // number of lights in the scene
uniform Light lights[MAX_LIGHTS]; // lights in the scene
uniform vec3  cameraPos;          // Position of camera in the world

// Properties of the object
struct Material {
  vec3 ka;
  vec3 kd;
  vec3 ks;
  float shininess;
};
uniform Material material;            // Material of the vertex


////////////////////////////////////////////////////////////////////////////////
// Vertex input/output
in  vec3 worldPos;    // Vertex position in world space
in  vec3 worldNormal; // Vertex normal in world space
out vec4 color;       // Assigned vertex color to send to rasterizer


////////////////////////////////////////////////////////////////////////////////
/// Determine the vertex's color using Blinn-Phong illumination model
/// @param pos    Position of vertex in world space
/// @param normal Normal of vertex in world space
vec4 shadeBlinnPhong(in vec3 pos, in vec3 normal) {
  vec3 color = vec3(0.0, 0.0, 0.0);
  vec3 viewDir = normalize(cameraPos - pos); // direction toward camera

  // add illumination from each light source
  for (int i = 0; i < numLights; i++) {
    // ambient
    color += material.ka * lights[i].ia;
    // diffuse
    vec3 lightDir = normalize(lights[i].pos - pos);  // direction toward light
    color += material.kd * lights[i].id * max(0.0, dot(normal, lightDir));
    // specular
    vec3 halfVec = normalize(viewDir + lightDir);
    color += material.ks 
             * lights[i].is 
             * pow(max(0.0, dot(normal, halfVec)), material.shininess);
  }
  color = min(color, vec3(1.0, 1.0, 1.0));
  return vec4(color, 1.0);
}


void main() {
  color = shadeBlinnPhong(worldPos, normalize(worldNormal));
}