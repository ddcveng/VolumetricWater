/*
 * Source code for the NPGR019 lab practices. Copyright Martin Kahoun 2021.
 * Licensed under the zlib license, see LICENSE.txt in the root directory.
 */

#pragma once

#include <ShaderCompiler.h>

// Shader programs
namespace ShaderProgram
{
  enum
  {
    Default, Water, NumShaderPrograms
  };
}

// Shader programs handle
extern GLuint shaderProgram[ShaderProgram::NumShaderPrograms];

// Helper function for creating and compiling the shaders
bool compileShaders();

// ============================================================================

// Vertex shader types
namespace VertexShader
{
  enum
  {
    Default, Water, NumVertexShaders
  };
}

// Vertex shader sources
static const char* vsSource[] = {
// default vertex shader
R"(
#version 460 core

layout (location = 0) uniform mat4 modelToWorld;
layout (location = 1) uniform mat4 worldToView;
layout (location = 2) uniform mat4 projection;
layout (location = 3) uniform vec4 clippingPlane;

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 vTexCoord;

void main()
{
    vec4 positionWorld = modelToWorld * vec4(position, 1.0);
    gl_ClipDistance[0] = dot(clippingPlane, positionWorld);
    
    vTexCoord = texCoords;
    gl_Position = projection * worldToView * positionWorld;
}
)",
// Water vertex shader
R"(
#version 460 core

layout (location = 0) uniform mat4 modelToWorld;
layout (location = 1) uniform mat4 worldToView;
layout (location = 2) uniform mat4 projection;
layout (location = 3) uniform vec3 cameraPosWorld;
layout (location = 4) uniform float movement;
layout (location = 5) uniform vec2 nearFar;
layout (location = 6) uniform vec2 tiling;

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 vTexCoord;
out vec4 posClipSpace;
out vec3 pixelToCam;

void main()
{
    // if the pool is now a square, scale one direction appropriately to tile the texture
    vTexCoord = vec2(texCoords.x * tiling.x, texCoords.y * tiling.y);

    vec4 positionWorld = modelToWorld * vec4(position, 1.0);
    pixelToCam = cameraPosWorld - positionWorld.xyz;
    posClipSpace = projection * worldToView * positionWorld;

    gl_Position = posClipSpace;
}
)"
};

// ============================================================================

// Fragment shader types
namespace FragmentShader
{
  enum
  {
    Default, Water, NumFragmentShaders
  };
}

// Fragment shader sources
static const char* fsSource[] = {
// ----------------------------------------------------------------------------
// Default fragment shader source
// ----------------------------------------------------------------------------
R"(
#version 460 core

layout (binding = 0) uniform sampler2D diffuse;

in vec2 vTexCoord;

layout (location = 0) out vec4 color;

void main()
{
  vec3 texSample = texture(diffuse, vTexCoord).rgb;
  color = vec4(texSample, 1.0f);
}
)",
// Water fragment shader
R"(
#version 460 core

layout (location = 4) uniform float movement;
layout (location = 5) uniform vec2 nearFar;

layout (binding = 0) uniform sampler2D refraction;
layout (binding = 1) uniform sampler2D reflection;
layout (binding = 2) uniform sampler2D normalMap;
layout (binding = 3) uniform sampler2D dudvMap;
layout (binding = 4) uniform sampler2D depthBuffer;

in vec2 vTexCoord;
in vec4 posClipSpace;
in vec3 pixelToCam;

const float distortionStrenght = 0.01;
const float offsetFactor = 0.1;
const float depthScale = 0.91; // how soon will the deep water color appear, it's pretty sensitive
const float fogDensity = 1.1;

const float n1 = 1.0;  // air
const float n2 = 1.33; // water


const int fresnelPower = 4;
const float r0 = (n1 - n2) / (n1 + n2);
// in case of air -> water reflection/refraction this is very small
// and can very well be approximated as 0 with similar effect
const float R0 = r0 * r0;

layout (location = 0) out vec4 color;

// returns the linearized depth buffer value in view space
float linearize_depth(float depthVal)
{
    return 2 * nearFar.x * nearFar.y / (nearFar.y + nearFar.x - (nearFar.y - nearFar.x) * (2 * depthVal - 1));
}

void main()
{
  // convert fragment position from clip space to normalized device space
  vec2 ndsCoord = (posClipSpace.xy / posClipSpace.w) / 2.0 + 0.5;
  vec2 refractCoord = ndsCoord;
  vec2 reflectCoord = vec2(ndsCoord.x, -ndsCoord.y);

  // sample the dudvMap with respect to movement to get distorted coords
  // these are used to get the actual distortion as well as the normal at that position
  vec2 offsetCoord = texture(dudvMap, vec2(vTexCoord.x + movement, vTexCoord.y)).xy * offsetFactor;
  offsetCoord = vTexCoord + vec2(offsetCoord.x, offsetCoord.y + movement);

  // use the distorted coords to get the actual distortion
  vec2 offset = (texture(dudvMap, offsetCoord).xy * 2.0 - 1.0) * distortionStrenght;

  // offset the nds coords
  // clamp the values to avoid artifacts at the edges of the pool
  refractCoord += offset;
  refractCoord = clamp(refractCoord, 0.001, 0.999);

  reflectCoord += offset;
  reflectCoord.x = clamp(reflectCoord.x, 0.001, 0.999);
  reflectCoord.y = clamp(reflectCoord.y, -0.999, -0.001);

  vec4 refractCol = vec4(texture(refraction, refractCoord).rgb, 1.0);
  vec4 reflectCol = vec4(texture(reflection, reflectCoord).rgb, 1.0);

  // sample the depth buffer to get distance from surface of water to floor
  // and use it to create fogginess in deeper parts
  float distToFloor = texture(depthBuffer, ndsCoord).x;
  distToFloor = linearize_depth(distToFloor);

  float distToWater = gl_FragCoord.z;
  distToWater = linearize_depth(distToWater);

  float sliceDepth = distToFloor - distToWater;
  sliceDepth = clamp(sliceDepth, 0.01, 0.99);
  sliceDepth *= depthScale;
  float fogAmount = 1.0 - exp( -distToWater * fogDensity);
  
  vec4 deepBlue = vec4(0.0, 0.0, 0.247, 1.0);
  vec4 fog1 = mix(refractCol, deepBlue, fogAmount); // fog based on distance from the pool
  vec4 fog2 = mix(refractCol, deepBlue, sliceDepth); // fog based on pool depth

  refractCol = mix(fog1, fog2, 0.5);

  vec3 normalRaw = texture(normalMap, offsetCoord).rgb;
  
  // convert rgb normal data to the actual vector it represents
  // also scale it a bit in the y direction so the water is not too bumpy
  vec3 normal = vec3(normalRaw.r * 2.0 - 1.0, 2.0 * normalRaw.b, normalRaw.g * 2.0 - 1.0);

  normal = normalize(normal);
  vec3 pixelToCamN = normalize(pixelToCam);

  float cosT = dot(pixelToCamN, normal);

  // Schlick's approximation
  float R = R0 + (1 - R0) * pow(1 - cosT, fresnelPower);
  R = clamp(R, 0.05, 0.95);

  color = mix(refractCol, reflectCol, R);
}
)"
};
