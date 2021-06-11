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

out vec4 positionWorld;
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

layout (location = 0) in vec3 position;

void main()
{
    gl_Position = vec4(position, 1.0);
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

layout (location = 0) out vec4 color;

void main()
{
    color = vec4(1.0, 0.0, 0.0, 1.0);
}
)"
};
