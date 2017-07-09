#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "gl_helpers.h"

namespace Primitives {

  MeshPtr CreatePlane(float width, float height, bool centering = true);
  MeshPtr CreateCube(float cube_size);
  MeshPtr CreateBox(float width, float height, float depth);
  MeshPtr CreateSphere(float R, int VSeg, int HSeg);
  MeshPtr CreateFontMesh(float width = 1.0f, float height = 1.0f);
  MeshPtr CreateFromFile(std::string file_name);

} //namespace Primitives

#endif // PRIMITIVES_H
