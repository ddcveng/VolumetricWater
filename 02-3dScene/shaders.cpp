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

  // Cleanup lambda
  auto cleanUp = [&vertexShader, &fragmentShader]()
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

  // Create all shader programs:
  shaderProgram[ShaderProgram::Water] = glCreateProgram();
  glAttachShader(shaderProgram[ShaderProgram::Water], vertexShader[VertexShader::Water]);
  glAttachShader(shaderProgram[ShaderProgram::Water], fragmentShader[FragmentShader::Water]);
  if (!ShaderCompiler::LinkProgram(shaderProgram[ShaderProgram::Water])) {
      cleanUp();
      return false;
  }

  shaderProgram[ShaderProgram::Default] = glCreateProgram();
  glAttachShader(shaderProgram[ShaderProgram::Default], vertexShader[VertexShader::Default]);
  glAttachShader(shaderProgram[ShaderProgram::Default], fragmentShader[FragmentShader::Default]);
  if (!ShaderCompiler::LinkProgram(shaderProgram[ShaderProgram::Default])) {
      cleanUp();
      return false;
  }


  cleanUp();
  return true;
}
