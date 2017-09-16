#include "primitives.h"
#include <vector>
#include <math.h>
#include <fstream>

namespace Primitives {

  MeshPtr CreateFromFile(std::string file_name) {
    int *ind;
    char *vert;
    char *norm;
    char *texc;

    std::ifstream file(file_name.c_str(), std::ios::binary);

    int ac,is,vs,ns,ts;

    file.seekg(0, std::ios::beg);
    file.read((char*)(&ac),4);
    file.read((char*)(&is),4);
    file.read((char*)(&vs),4);
    file.read((char*)(&ns),4);
    file.read((char*)(&ts),4);

    ind = new int[is/4];
    vert = new char[vs];
    norm = new char[ns];
    texc = new char[ts];

    file.read((char*)(ind),is);
    file.read((char*)(vert),vs);
    file.read((char*)(norm),ns);
    file.read((char*)(texc),ts);


    MeshPtr obj = MeshPtr(new Mesh(GL_TRIANGLES, is/4));

    obj->vId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, vs, vert);
    obj->nId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, ns, norm);
    obj->tId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, ts, texc);
    obj->iId = gl_helpers::createBufferObject(GL_ELEMENT_ARRAY_BUFFER, is, ind);
    obj->tcc = 3;

    return obj;

  }

  MeshPtr CreatePlane(float width, float height, bool centering) {
    MeshPtr plane = MeshPtr(new Mesh(GL_TRIANGLES, 6));
    float offs = centering ? 0.0f : 0.5f;

    float vertices[12] = {
      (-0.5f+offs)*width, (-0.5f+offs)*height, 0.0f,
      ( 0.5f+offs)*width, (-0.5f+offs)*height, 0.0f,
      ( 0.5f+offs)*width, ( 0.5f+offs)*height, 0.0f,
      (-0.5f+offs)*width, ( 0.5f+offs)*height, 0.0f,
    };

    float normals[12] = {
      0.0f, 0.0f, -1.0f,
      0.0f, 0.0f, -1.0f,
      0.0f, 0.0f, -1.0f,
      0.0f, 0.0f, -1.0f,
    };

    float texcoords[8] = {
      0.0f, 1.0f,
      1.0f, 1.0f,
      1.0f, 0.0f,
      0.0f, 0.0f,
    };

    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

    plane->vId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(vertices), vertices);
    plane->nId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(normals), normals);
    plane->tId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords);
    plane->iId = gl_helpers::createBufferObject(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices);

    return plane;
  }

  MeshPtr CreateFontMesh(float width, float height) {
    MeshPtr font_mesh = MeshPtr(new Mesh(GL_TRIANGLES, 6));
    const uint32_t columns = 16;
    const uint32_t rows = 16;
    uint32_t pcount = columns * rows;

    float vertices[12] = {
       0.0f,  0.0f, 0.0f,
       width, 0.0f, 0.0f,
       width, height, 0.0f,
       0.0f,  height, 0.0f,
    };

    float normals[12] = {
      0.0f, 0.0f, -1.0f,
      0.0f, 0.0f, -1.0f,
      0.0f, 0.0f, -1.0f,
      0.0f, 0.0f, -1.0f,
    };

    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

    font_mesh->vId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(float)*12*pcount, NULL);
    font_mesh->nId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(normals)*pcount, NULL);
    font_mesh->tId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(float)*8*pcount, NULL);
    font_mesh->iId = gl_helpers::createBufferObject(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices);

    float w = 1.0f / float(columns);
    float h = 1.0f / float(rows);

    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < columns; ++j) {
        uint32_t idx = i*columns+j;

        float tx = float(j) / float(columns);
        float ty = float(i) / float(rows);

        float texcoords[8] = {
           tx,     (ty+h),
          (tx+w),  (ty+h),
          (tx+w),   ty,
           tx,      ty,
        };

        gl_helpers::uploadData(font_mesh->vId, sizeof(vertices), vertices, idx*sizeof(vertices));
        gl_helpers::uploadData(font_mesh->nId, sizeof(normals), normals, idx*sizeof(normals));
        gl_helpers::uploadData(font_mesh->tId, sizeof(texcoords), texcoords, idx*sizeof(texcoords));
      }
    }

    return font_mesh;
  }


  MeshPtr CreateCube(float cube_size) {
     assert(false);
     MeshPtr cube = MeshPtr(new Mesh(GL_TRIANGLES, 24));
     return cube;
  }

  MeshPtr CreateBox(float width, float height, float depth) {
    assert(false);
    MeshPtr box = MeshPtr(new Mesh(GL_TRIANGLES, 24));
    return box;
  }

  MeshPtr CreateSphere(float R, int VSeg, int HSeg) {
    float a,b;
    float da,db,rx,rz,rxz,ry;
    float ks,kt;
    int i,j,si,vi;

    glm::vec3 v,norm;
    glm::vec2 t;
    const float pi = 3.14159265359;

    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> TexCoords;
    std::vector<GLuint> Indices;
    Indices.clear();
    Vertices.clear();
    Normals.clear();
    TexCoords.clear();

    da = pi/VSeg;
    db = 2.0f*pi/(float(HSeg)-1.0f);
    ks = 1.0f/(float(HSeg)-1.0f);
    kt = 1.0f/(float(VSeg));

    for (i = 0; i<=VSeg; ++i) {
      a = float(i)*da-pi/2.0;
      float sa = sin(a); float ca = cos(a);
      ry = sa*R; rxz = ca*R;
      si = Indices.size();
      for (j = 0; j < HSeg; ++j) {
        b = float(j)*db; rx = rxz*cos(b); rz = rxz*sin(b); vi = i*(HSeg)+j;
        Vertices.push_back(glm::vec3(rx,ry,rz));
        TexCoords.push_back(glm::vec2(1.0-float(j)*ks, float(i)*kt));
        norm = glm::vec3( rx, ry, rz )/R;
        Normals.push_back(norm);
        if (i<VSeg) {
          Indices.push_back(vi); Indices.push_back(vi+HSeg);
        }
      }
      Indices.push_back(Indices[si]); Indices.push_back(Indices[si+1]);
    }
    Indices.resize(Indices.size()-2);
    MeshPtr sphere = MeshPtr(new Mesh(GL_TRIANGLE_STRIP, Indices.size()));

    sphere->vId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(glm::vec3)*Vertices.size(), Vertices.data());
    sphere->nId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(glm::vec3)*Normals.size(), Normals.data());
    sphere->tId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(glm::vec2)*TexCoords.size(), TexCoords.data());
    sphere->iId = gl_helpers::createBufferObject(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*Indices.size(), Indices.data());

    return sphere;
  }


MeshPtr CreatePlane(float width, float height, int TilesX, int TilesY, bool AsLine) {
  std::vector<glm::vec3> Vertices;
  std::vector<glm::vec3> Normals;
  std::vector<glm::vec2> TexCoords;
  std::vector<GLuint> Indices;
  Indices.clear();
  Vertices.clear();
  Normals.clear();
  TexCoords.clear();

  int offs;
  glm::vec3 v;

  float kx = (width)/float(TilesX);
  float ky = (height)/float(TilesY);
  float H = 0;

  int LW = TilesX + 1;
  for (int i = 0; i <= TilesY; ++i) {
    int oy = i;
    for (int j = 0; j <= TilesX; ++j) {
      Vertices.push_back(glm::vec3(j*kx-width/2.0f, float(H), i*ky-height/2.0f));
      float x = j*kx / (width);
      float y = 1.0f - i*ky / height;
      TexCoords.push_back(glm::vec2(x,y));
      if (i < TilesY) {
        int ox = j; offs = oy * LW + ox;
        Indices.push_back(offs);
        Indices.push_back(offs + LW);
      }
    }
    int j = Indices[Indices.size() - 1];
    offs = (oy + 1) * LW;
    Indices.push_back(j); Indices.push_back(offs);
  }

  if (AsLine) {
   Indices[Indices.size()-1] = 0;
   for (int i = 0; i < LW - 1; ++i) Indices.push_back(i);
  } else Indices.resize(Indices.size()-4);

  Normals.resize(Vertices.size());
  for (int i = 0; i<Normals.size(); ++i) Normals[i] = glm::vec3(0.0f, 1.0f, 0.0f);

  GLenum ft = GL_TRIANGLE_STRIP;
  if (AsLine) ft = GL_LINE_STRIP;

    MeshPtr mesh = MeshPtr(new Mesh(ft, Indices.size()));

    mesh->vId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(glm::vec3)*Vertices.size(), Vertices.data());
    mesh->nId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(glm::vec3)*Normals.size(), Normals.data());
    mesh->tId = gl_helpers::createBufferObject(GL_ARRAY_BUFFER, sizeof(glm::vec2)*TexCoords.size(), TexCoords.data());
    mesh->iId = gl_helpers::createBufferObject(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*Indices.size(), Indices.data());

    return mesh;

}


} //namespace Primitives
