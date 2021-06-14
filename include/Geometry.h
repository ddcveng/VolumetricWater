/*
 * Source code for the NPGR019 lab practices. Copyright Martin Kahoun 2021.
 * Licensed under the zlib license, see LICENSE.txt in the root directory.
 */

#pragma once

#include "Mesh.h"
#include "Vertex.h"

// Geometry utilities class
class Geometry
{
public:
  // Creates simple quad with uniform color
  static Mesh<Vertex_Pos_Col> *CreateQuadColor();
  // Create simple quad with texture coordinates
  static Mesh<Vertex_Pos_Tex> *CreateQuadTex();
  // Create simple quad with normals, tangents and texture coordinates
  static Mesh<Vertex_Pos_Nrm_Tgt_Tex> *CreateQuadNormalTangentTex();
  // Creates simple cube with colors
  static Mesh<Vertex_Pos_Col> *CreateCubeColor();

  static Mesh<Vertex_Pos_Tex>* CreateQuadTex2D();
  static Mesh<Vertex_Pos_Tex>* CreateQuadGrid(int size);
  static Mesh<Vertex_Pos_Col> *CreatePool();
  static Mesh<Vertex_Pos_Tex>* CreatePoolTex();
  static Mesh<Vertex_Pos_Tex>* CreateCubeTexInsideOut();

  // Creates simple cube with colors and shared vertices
  static Mesh<Vertex_Pos_Col> *CreateCubeColorShared();
  // Creates simple cube with texture coordinates
  static Mesh<Vertex_Pos_Tex> *CreateCubeTex();
  // Create simple cube with normals, tangents and texture coordinates
  static Mesh<Vertex_Pos_Nrm_Tgt_Tex> *CreateCubeNormalTangentTex(bool createAdjacencyInfo = false);
  // Create tethrahedron composed from vertices
  static Mesh<Vertex_Pos_Nrm> *CreateTetrahedron();

private:
  Geometry();
  ~Geometry();
};
