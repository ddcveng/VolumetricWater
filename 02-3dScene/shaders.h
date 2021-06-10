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
    Tess, NumShaderPrograms
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
    Tess, NumVertexShaders
  };
}

// Vertex shader sources
static const char* vsSource[] = {
// Tesselation vertex shader
R"(
    #version 460 core

    layout (location = 0) uniform mat4 modelToWorld;

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 texCoords;

    out vec4 positionWorld;
    out vec2 vTexCoords;

    void main()
    {
        positionWorld = modelToWorld * vec4(position, 1.0);
        vTexCoords = texCoords;

        //will set gl_Position later in the TES...
    }
)"
};

// ============================================================================

// Fragment shader types
namespace FragmentShader
{
  enum
  {
    Default, NumFragmentShaders
  };
}

// Fragment shader sources
static const char* fsSource[] = {
// ----------------------------------------------------------------------------
// Default fragment shader source
// ----------------------------------------------------------------------------
R"(
#version 460 core

// Texture sampler
layout (binding = 0) uniform sampler2D diffuse;

// Fragment shader inputs
in vec2 vTexCoord;

// Fragment shader outputs
layout (location = 0) out vec4 color;

void main()
{
  vec3 texSample = texture(diffuse, vTexCoord).rgb;
  color = vec4(texSample, 1.0f);
}
)"
};

namespace TesselationControlShader {
enum {
    Default, NumTesselationControlShaders
};
}

static const char* tcsSource[] = {
R"(
#version 460 core

layout (vertices = 4) out;

layout (location = 3) uniform vec4 cameraPosWorld;
layout (location = 4) uniform int mode;

in vec4 positionWorld[];
in vec2 vTexCoords[];

out vec4 worldPos_ES_in[];
out vec2 texCoords_ES_in[];

//queried for GL_MAX_TESS_GEN_LEVEL
const int maxTessLvl = 64;

// in meters
const float maxDistance = 7.0;
const float minDistance = 0.5;

float distToTessLvl(float distance)
{
    float d = clamp(distance, minDistance, maxDistance);
    d /= maxDistance; // d in range [0, 1]

    float tess = max(1.0, maxTessLvl - d * maxTessLvl);

    //adaptive tesselation
    if (mode == 1) {
        if (tess < 4.0) {
            tess = 2.0;
        } else if (tess < 8.0) {
            tess = 4.0;
        } else if (tess < 16.0) {
            tess = 8.0;
        } else if (tess < 32.0) {
            tess = 16.0;
        } else if (tess < 50) {
            tess = 32.0;
        } else {
            tess = 64.0;
        }
    }
    //no tesselation 
    else if (mode == 2) {
        tess = 1;
    }
    //max tesselation
    else if (mode == 3) {
        tess = maxTessLvl;
    }

    return tess;
}

void main()
{
    texCoords_ES_in[gl_InvocationID] = vTexCoords[gl_InvocationID];
    worldPos_ES_in[gl_InvocationID] = positionWorld[gl_InvocationID];

    //we only need to setup the tesselation levels once per patch
    if (gl_InvocationID == 0)
    {
        vec4 botleft = positionWorld[0];
        vec4 botright = positionWorld[1];
        vec4 topright = positionWorld[2];
        vec4 topleft = positionWorld[3];
        
        //distance from camera to middle of the side of the quad
        float distLeft  = distance(mix(botleft,  topleft,  0.5), cameraPosWorld);
        float distBot   = distance(mix(botleft,  botright, 0.5), cameraPosWorld);
        float distRight = distance(mix(botright, topright, 0.5), cameraPosWorld);
        float distTop   = distance(mix(topleft,  topright, 0.5), cameraPosWorld);


        //outer levels based on distance
        float OL0 = distToTessLvl(distLeft);
        float OL1 = distToTessLvl(distBot);
        float OL2 = distToTessLvl(distRight);
        float OL3 = distToTessLvl(distTop);
    
        //inner levels
        float IL0 = min(OL0, OL2);
        float IL1 = min(OL1, OL3);

        gl_TessLevelOuter[0] = OL0;
        gl_TessLevelOuter[1] = OL1;
        gl_TessLevelOuter[2] = OL2;
        gl_TessLevelOuter[3] = OL3;

        gl_TessLevelInner[0] = IL0;
        gl_TessLevelInner[1] = IL1;
    }
}
)"

};

namespace TesselationEvaluationShader {
enum {
    Default, NumTesselationEvaluationaShaders
};
}

static const char* tesSource[] = {
R"(
#version 460 core

// we are working with quad patches
// changing the spacing doesn't do much in our case
layout (quads, equal_spacing, ccw) in;

layout (binding = 1) uniform sampler2D heightMap;

layout (location = 1) uniform mat4 worldToView;
layout (location = 2) uniform mat4 projection;
layout (location = 4) uniform int mode;

// inputs from the TCS
// 4 vertex patches -> 4 element arrays
in vec4 worldPos_ES_in[];
in vec2 texCoords_ES_in[];

out vec2 vTexCoord;

const float totalHeight = 4.0f;

// use normalized coords from the primitive generator 
// to construct the position of the newly created vertex
vec4 interpolate(vec4 v0, vec4 v1, vec4 v2, vec4 v3)
{
    vec4 a = mix(v0, v1, gl_TessCoord.x);
    vec4 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

// same thing just for vec2
vec2 interpolateUV(vec2 v0, vec2 v1, vec2 v2, vec2 v3)
{
    vec2 a = mix(v0, v1, gl_TessCoord.x);
    vec2 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

void main()
{
    vTexCoord = interpolateUV(texCoords_ES_in[0], texCoords_ES_in[1], texCoords_ES_in[2], texCoords_ES_in[3]);
    vec4 interpolatedWorldPos = interpolate(worldPos_ES_in[0], worldPos_ES_in[1], worldPos_ES_in[2], worldPos_ES_in[3]);
    
    if (mode != 2) {
        float displacement = (texture(heightMap, vTexCoord).r - 1.0) * totalHeight;
        interpolatedWorldPos.y += displacement;
    }
    
    gl_Position = projection * worldToView * interpolatedWorldPos;
}

)"

};

