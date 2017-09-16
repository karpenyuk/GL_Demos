#ifndef GL_HELPERS_H
#define GL_HELPERS_H

#include <map>
#include <vector>
#include <string>
#include <tr1/memory>

//GLEW headers
#include <GL/glew.h>
#include <GL/wglew.h>

//GLM Headers
#define GLM_FORCE_RADIANS
#define GLM_GTX_transform
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "glm/ext.hpp"
#include "glm/gtc/round.hpp"
#include "glm/gtx/string_cast.hpp"

# define GL_CHECK()\
  { GLenum errCode;\
  const GLubyte *errString;\
  if ((errCode = glGetError()) != GL_NO_ERROR) {\
    errString = gluErrorString(errCode);\
    printf("OpenGL Error: %s[%s:%d] \n", errString, __PRETTY_FUNCTION__, __LINE__);\
  } }



const GLsizei POSITION = 0;
const GLsizei NORMAL   = 1;
const GLsizei TEXCOORD = 2;

struct Shader {
  GLuint Id;
  Shader() : Id(0) {}
  Shader(GLuint id) : Id(id) {}
  ~Shader() { glDeleteProgram(Id); }
};
typedef std::tr1::shared_ptr<Shader> ShaderPtr;

struct Texture2D {
    GLuint Id;
    int width, height;
    void *data;
    std::size_t dataSize;
    std::size_t texelSize;
    GLuint internalFormat;
    GLuint format;
    GLenum type;
    GLuint64 texHandle;
    Texture2D() : Id(0) { }
    ~Texture2D() {glDeleteTextures(1,&Id);}
};
typedef std::tr1::shared_ptr<Texture2D> Texture2DPtr;


struct Material {
    std::string name;
    int matIdx;
    glm::vec4 Ka;
    glm::vec4 Kd;
    glm::vec4 Ks;
    uint8_t Ns; //Shininess
    uint8_t illum;
    std::string map_opacity;
    std::string map_bump; //Normal
    std::string bump;
    std::string map_d; //Alpha
    std::string map_Kd;
    std::string map_Ks;
    std::string map_Ka;
    std::string map_Ns;
    Material(std::string aName) {
      name = aName;
      matIdx = -1;
      Ka = glm::vec4(0.2, 0.2, 0.2, 1.0);
      Kd = glm::vec4(0.8, 0.8, 0.8, 1.0);
      Ks = glm::vec4(0.0, 0.0, 0.0, 1.0);
      Ns = 127;
      illum = 2;
      map_opacity = "";
      map_bump = "";
      bump = "";
      map_d = "";
      map_Kd = "";
      map_Ks = "";
      map_Ka = "";
      map_Ns = "";
    }
    bool isTransparency() {
      if (Kd.a<1.0f || map_d != "") return true;
      return false;
    }
};

struct InstanceInfo {
  std::string InstanceName;
  int InstId;
  Material *Mat;
  std::string Parent;
  glm::mat4 ModelMatrix;
  int ElementsCount;
  int baseVertex;
  int baseIndex;
  InstanceInfo(std::string name) {
      InstanceName = name;
      InstId = 0;
      Parent = "";
      Mat = NULL;
      ModelMatrix = glm::mat4(1.0f);
      ElementsCount = 0;
      baseVertex = 0;
      baseIndex = 0;
  }
};

struct Mesh {
  private:
    GLuint vao;
    GLenum face_type;
    GLsizei elements;
    void buildVAO();
  public:
    GLuint vId, nId, tId, iId;
    GLuint vcc, ncc, tcc;

    Mesh(GLenum aFace_type, GLsizei aElements) :
      vao(0), vId(0), nId(0), tId(0), iId(0),
      vcc(3), ncc(3), tcc(2) {
      face_type = aFace_type; elements = aElements;
    }
    ~Mesh() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vId);
        glDeleteBuffers(1, &nId);
        glDeleteBuffers(1, &tId);
        glDeleteBuffers(1, &iId);
    }

    void Bind(bool state);
    void Draw();
    GLenum getFaceType() { return face_type; }
    GLsizei getElementsCount() { return elements; }
};

typedef std::tr1::shared_ptr<Mesh> MeshPtr;


typedef std::map<std::string, GLuint> UniformMaps;
typedef std::map< GLuint, UniformMaps > ShaderMaps;
typedef std::map< GLuint, UniformMaps >::iterator ShaderMapIterator;

class gl_helpers {
  public:
    static void Initialize();
    static ShaderPtr createProgram (const char * vertexSource, const char * fragmentSource);
    static Texture2DPtr  createAlphaLuminanceTexture (int width, int height, void *data = NULL, bool aFilters = true);
    static Texture2DPtr createTexture(std::string aFilename, bool aFilters = false);
    static Texture2DPtr createColorTexture(void *color, bool aFilters = false);
    static GLuint createBufferObject(GLenum bufferType, size_t buff_size, void *data = NULL, GLenum usage = GL_STATIC_DRAW);
    static GLuint CreateBufferObject(size_t buff_size, void **data, GLenum bufferType = GL_SHADER_STORAGE_BUFFER);

    static void setUniform(ShaderPtr shader, std::string name, const int32_t value);
    static void setUniform(ShaderPtr shader, std::string name, const glm::vec2 value);
    static void setUniform(ShaderPtr shader, std::string name, const glm::vec3 value);
    static void setUniform(ShaderPtr shader, std::string name, const glm::vec4 value);
    static void setUniform(ShaderPtr shader, std::string name, const glm::mat4 value);
    static void StartTimer();
    static double GetCommandTime();
    static void uploadData(GLuint buffId, GLuint data_size, void * data_ptr, GLuint offset = 0);
    static void getData(GLuint buffId, GLuint data_size, void * data_ptr, GLuint offset = 0);
    static void CheckOpenGLError(std::string point="");
  private:
    static GLuint64 startTime, stopTime;
    static GLuint queryID[2];
    static ShaderMaps _unifroms;
    static GLuint loadShader ( const char * source, GLenum type );
    static Texture2DPtr loadTGA(std::string const & aFilename);
    static int GetUniformLocation(GLuint shaderId, std::string &name);

};

template <typename T > class SSBO {
  private:
    uint32_t arrayLength;
    T *data;
    GLuint buffId;
    GLuint buffType;
    GLuint loc;
  public:
    SSBO(uint32_t elementsCount, GLuint bufferType = GL_SHADER_STORAGE_BUFFER) {
        arrayLength = elementsCount; buffType = bufferType; loc = 0;
        buffId = gl_helpers::CreateBufferObject(sizeof(T) * elementsCount, (void**)&data, bufferType);
        printf("SSBO initialized: Id=%d, Ptr=%p\n", buffId, data);
    }
    ~SSBO() {
      glBindBuffer(buffType, buffId);
      glUnmapBuffer(buffType);
      glDeleteBuffers(1, &buffId);
    }

    T* getData() { return data; }
    GLuint getBuffId() { return buffId; }
    uint32_t getCount() { return arrayLength; }
    uint32_t getBuffSize() { return arrayLength * sizeof(T); }
    void Bind(GLuint location = 0) { glBindBufferBase(buffType, location, buffId); loc = location; }
    void BindAs(GLuint buff_type, GLuint location = 0) { glBindBufferBase(buff_type, location, buffId); loc = location; }
    void BindAsAttrib(GLuint location = 0) { glBindBuffer(GL_ARRAY_BUFFER, buffId);
       glVertexAttribPointer(location, sizeof(T) / sizeof(float), GL_FLOAT, false, 0, 0);
       glEnableVertexAttribArray(location);
       loc = location;
    }
};

template<typename T>
class SSBOPtr : public std::tr1::shared_ptr<SSBO<T> > {
public:
  typedef          std::tr1::shared_ptr<SSBO<T> >   base_class;
  typedef typename base_class::element_type         element_type;

  SSBOPtr() : base_class() {}
  SSBOPtr(element_type* rawPtr) : base_class(rawPtr) {}
};

template <typename T > class TBO {
  private:
    uint32_t arrayLength;
    T *data;
    GLuint buffId;
    GLuint buffType;
    GLuint texId;
    GLint TextureBufferOffsetAlignment;
    uint32_t elements_count;
    uint32_t element_size;
    uint32_t data_size;


  public:
    TBO(uint32_t elementsCount, uint32_t elementSize, GLuint textureFormat, T *source_data) {
        data = source_data;
        elements_count = elementsCount;
        element_size = elementSize;
        data_size = elementsCount * elementSize;
        printf("elements_count = %d, element_size = %d, data_size = %d\n", elements_count, element_size, data_size);
        glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &TextureBufferOffsetAlignment);
        int MultipledSize = glm::ceilMultiple(int(data_size), int(TextureBufferOffsetAlignment));
        printf("TextureBufferOffsetAlignment = %d, MultipledSize = %d\n", TextureBufferOffsetAlignment, MultipledSize);


		glGenBuffers(1, &buffId);
		buffType = GL_TEXTURE_BUFFER;

		glBindBuffer(buffType, buffId);
		glBufferData(buffType, TextureBufferOffsetAlignment + MultipledSize, 0, GL_STATIC_DRAW);
		glBufferSubData(buffType, TextureBufferOffsetAlignment, data_size, data);

		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		printf("TBO initialized: Id=%d, Ptr=%p\n", buffId, data);

		glGenTextures(1, &texId);
		glBindTexture(GL_TEXTURE_BUFFER, texId);
		glTexBufferRange(GL_TEXTURE_BUFFER, textureFormat, buffId, TextureBufferOffsetAlignment, data_size);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

    }
    ~TBO() {
      glBindBuffer(buffType, buffId);
      glUnmapBuffer(buffType);
      glDeleteBuffers(1, &buffId);
      glBindTexture(GL_TEXTURE_BUFFER, 0);
      glDeleteTextures(1, &texId);
    }

    T* getData() { return data; }
    GLuint getBuffId() { return buffId; }
    GLuint getTexId() { return texId; }
    uint32_t getCount() { return elements_count; }
    uint32_t getBuffSize() { return data_size; }

    void Bind(GLenum texureUnit) {
        glActiveTexture(GL_TEXTURE0+texureUnit);
		glBindTexture(GL_TEXTURE_BUFFER,texId);
    }
    void UnBind(GLenum texureUnit ) {
        glActiveTexture(GL_TEXTURE0+texureUnit);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
    }
};

template<typename T>
class TBOPtr : public std::tr1::shared_ptr<TBO<T> > {
public:
  typedef          std::tr1::shared_ptr<TBO<T> >   base_class;
  typedef typename base_class::element_type        element_type;

  TBOPtr() : base_class() {}
  TBOPtr(element_type* rawPtr) : base_class(rawPtr) {}
};

class MatLibrary : public std::map<std::string, Material*> {
private:
  std::string path;
  void SplitFilename (const std::string& str) {
        size_t found=str.find_last_of("/\\");
        path = str.substr(0,found);
        path +="\\";
  }
public:
  MatLibrary(std::string file_name);
  std::map<std::string, Texture2DPtr> textures;
  bool getTexture(std::string file_name, Texture2DPtr &tex);
  std::string getPath() { return path; }
};

class Instances : public std::map<std::string, InstanceInfo*> {
public:
  MatLibrary *mat_lib;
  Instances(std::string file_name, MatLibrary *ml);
};

#endif // GL_HELPERS_H
