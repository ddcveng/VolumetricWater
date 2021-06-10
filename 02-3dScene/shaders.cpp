/*
 * Source code for the NPGR019 lab practices. Copyright Martin Kahoun 2021.
 * Licensed under the zlib license, see LICENSE.txt in the root directory.
 */

#include "shaders.h"

GLuint shaderProgram[ShaderProgram::NumShaderPrograms] = {0};

bool compileShaders()
{
  GLuint vertexShader[VertexShader::NumVertexShaders] = {0};
  GLuint fragmentShader[FragmentShader::NumFragmentShaders] = {0};
  GLuint tcsShader[TesselationControlShader::NumTesselationControlShaders] = {0};
  GLuint tesShader[TesselationEvaluationShader::NumTesselationEvaluationaShaders] = {0};

  // Cleanup lambda
  auto cleanUp = [&vertexShader, &fragmentShader, &tcsShader, &tesShader]()
  {
    for (int i = 0; i < VertexShader::NumVertexShaders; ++i)
    {
      if (glIsShader(vertexShader[i]))
        glDeleteShader(vertexShader[i]);
    }

    for (int i = 0; i < FragmentShader::NumFragmentShaders; ++i)
    {
      if (glIsShader(fragmentShader[i]))
        glDeleteShader(fragmentShader[i]);
    }

    for (int i = 0; i < TesselationControlShader::NumTesselationControlShaders; ++i) {
        if (glIsShader(tcsShader[i]))
            glDeleteShader(tcsShader[i]);
    }

    for (int i = 0; i < TesselationEvaluationShader::NumTesselationEvaluationaShaders; ++i) {
        if (glIsShader(tesShader[i]))
            glDeleteShader(tesShader[i]);
    }
  };

  // Compile all vertex shaders
  for (int i = 0; i < VertexShader::NumVertexShaders; ++i)
  {
    vertexShader[i] = ShaderCompiler::CompileShader(vsSource, i, GL_VERTEX_SHADER);
    if (!vertexShader[i])
    {
      cleanUp();
      return false;
    }
  }

  // Compile all fragment shaders
  for (int i = 0; i < FragmentShader::NumFragmentShaders; ++i)
  {
    fragmentShader[i] = ShaderCompiler::CompileShader(fsSource, i, GL_FRAGMENT_SHADER);
    if (!fragmentShader[i])
    {
      cleanUp();
      return false;
    }
  }

  // Compile all tcs shaders
  for (int i = 0; i < TesselationControlShader::NumTesselationControlShaders; ++i) {
      tcsShader[i] = ShaderCompiler::CompileShader(tcsSource, i, GL_TESS_CONTROL_SHADER);
      if (!tcsShader[i]) {
          cleanUp();
          return false;
      }
  }

  // Compile all tes shaders
  for (int i = 0; i < TesselationEvaluationShader::NumTesselationEvaluationaShaders; ++i) {
      tesShader[i] = ShaderCompiler::CompileShader(tesSource, i, GL_TESS_EVALUATION_SHADER);
      if (!tesShader[i]) {
          cleanUp();
          return false;
      }
  }

  // Create all shader programs:
  shaderProgram[ShaderProgram::Tess] = glCreateProgram();
  glAttachShader(shaderProgram[ShaderProgram::Tess], vertexShader[VertexShader::Tess]);
  glAttachShader(shaderProgram[ShaderProgram::Tess], tcsShader[TesselationControlShader::Default]);
  glAttachShader(shaderProgram[ShaderProgram::Tess], tesShader[TesselationEvaluationShader::Default]);
  glAttachShader(shaderProgram[ShaderProgram::Tess], fragmentShader[FragmentShader::Default]);
  if (!ShaderCompiler::LinkProgram(shaderProgram[ShaderProgram::Tess])) {
      cleanUp();
      return false;
  }


  cleanUp();
  return true;
}
