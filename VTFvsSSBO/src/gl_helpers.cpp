#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <assert.h>
#include "gl_helpers.h"
#include <glm/gtc/type_ptr.hpp>
#include "../../inih/ini.h"

ShaderMaps gl_helpers::_unifroms;
GLuint64 gl_helpers::startTime(0);
GLuint64 gl_helpers::stopTime(0);
GLuint gl_helpers::queryID[2] = {0,0};

void gl_helpers::Initialize() {
  glGenQueries(2, queryID);
}

void gl_helpers::StartTimer() {
  glQueryCounter(queryID[0], GL_TIMESTAMP);
}

double gl_helpers::GetCommandTime() {
  GLint stopTimerAvailable(0);
  glQueryCounter(queryID[1], GL_TIMESTAMP);

  while (!stopTimerAvailable) {
    glGetQueryObjectiv(queryID[1], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
  }
  glGetQueryObjectui64v(queryID[0], GL_QUERY_RESULT, &startTime);
  glGetQueryObjectui64v(queryID[1], GL_QUERY_RESULT, &stopTime);
  return double(stopTime-startTime)/1000.0;
}

GLuint gl_helpers::loadShader ( const char * source, GLenum type )
{
  GLuint shader = glCreateShader ( type );
  GLint   len   = strlen ( source );
  GLint   compileStatus;

  glShaderSource ( shader, 1, &source,  &len );
  glCompileShader ( shader );

  glGetShaderiv ( shader, GL_COMPILE_STATUS, &compileStatus );
  if ( compileStatus == 0 ) {
    int val,len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &val);
    char infoLog[val];
    glGetShaderInfoLog(shader, val, &len, infoLog);
    if (len>0) printf("Shader compilation error: %s", infoLog);
    return 0;
  }
  return shader;
}

ShaderPtr gl_helpers::createProgram (const char * vertexSource, const char * fragmentSource)
{
  GLuint	program = glCreateProgram ();
  GLuint	vs      = loadShader ( vertexSource, GL_VERTEX_SHADER );
  GLuint	fs      = loadShader ( fragmentSource, GL_FRAGMENT_SHADER );
  GLint	linked;

  glAttachShader ( program, vs );
  glAttachShader ( program, fs );
  glLinkProgram  ( program );
  glGetProgramiv ( program, GL_LINK_STATUS, &linked );

  if ( !linked ) {
    int val,len = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &val);
    char infoLog[val];
    glGetProgramInfoLog(program, val, &len, infoLog);
    if (len>0) printf("Program linking error: %s", infoLog);
    return ShaderPtr(new Shader(0));
  }

  glDeleteShader(vs);
  glDeleteShader(fs);

  return ShaderPtr(new Shader(program));
}

Texture2DPtr gl_helpers::createAlphaLuminanceTexture (int width, int height, void *data, bool aFilters ) {
  Texture2DPtr tex2d = Texture2DPtr(new Texture2D);
  tex2d->data = data;
  tex2d->width = width;
  tex2d->height = height;
  tex2d->type = GL_UNSIGNED_BYTE;
  tex2d->internalFormat = GL_RGBA;
  tex2d->format = GL_LUMINANCE_ALPHA;
  glGenTextures(1, &tex2d->Id);
  glBindTexture(GL_TEXTURE_2D, tex2d->Id);
  if (aFilters) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
  } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 0);
  };

  glTexImage2D(GL_TEXTURE_2D, 0, tex2d->internalFormat, tex2d->width, tex2d->height,
      0, tex2d->format, tex2d->type, tex2d->data);
  glBindTexture(GL_TEXTURE_2D, 0);

  tex2d->texHandle = glGetTextureHandleARB(tex2d->Id);
  glMakeTextureHandleResidentARB(tex2d->texHandle);

  return tex2d;
}

Texture2DPtr gl_helpers::createColorTexture(void *color, bool aFilters) {
  Texture2DPtr tex2d = Texture2DPtr(new Texture2D);
  tex2d->data = color;
  tex2d->width = 1;
  tex2d->height = 1;
  tex2d->type = GL_UNSIGNED_BYTE;
  tex2d->internalFormat = GL_RGBA;
  tex2d->format = GL_RGBA;
  glGenTextures(1, &tex2d->Id);
  glBindTexture(GL_TEXTURE_2D, tex2d->Id);
  if (aFilters) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
  } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 0);
  };

  glTexImage2D(GL_TEXTURE_2D, 0, tex2d->internalFormat, tex2d->width, tex2d->height,
      0, tex2d->format, tex2d->type, tex2d->data);
  glBindTexture(GL_TEXTURE_2D, 0);

  tex2d->texHandle = glGetTextureHandleARB(tex2d->Id);
  glMakeTextureHandleResidentARB(tex2d->texHandle);

  return tex2d;
}

Texture2DPtr gl_helpers::loadTGA(std::string const & aFilename) {
  Texture2DPtr tex2d = Texture2DPtr(new Texture2D);
  std::ifstream FileIn(aFilename.c_str(), std::ios::in | std::ios::binary);
  tex2d->data = 0;
  tex2d->width = -1;
  tex2d->height = -1;
  if(!FileIn) {
    printf("TGA loading error: %s\n", aFilename.c_str());
    return tex2d;
  }

  unsigned char IdentificationFieldSize;
  unsigned char ColorMapType;
  unsigned char ImageType;
  unsigned short ColorMapOrigin;
  unsigned short ColorMapLength;
  unsigned char ColorMapEntrySize;
  unsigned short OriginX;
  unsigned short OriginY;
  unsigned short Width;
  unsigned short Height;
  unsigned char TexelSize;
  unsigned char Descriptor;

  FileIn.read((char*)&IdentificationFieldSize, sizeof(IdentificationFieldSize));
  FileIn.read((char*)&ColorMapType, sizeof(ColorMapType));
  FileIn.read((char*)&ImageType, sizeof(ImageType));
  FileIn.read((char*)&ColorMapOrigin, sizeof(ColorMapOrigin));
  FileIn.read((char*)&ColorMapLength, sizeof(ColorMapLength));
  FileIn.read((char*)&ColorMapEntrySize, sizeof(ColorMapEntrySize));
  FileIn.read((char*)&OriginX, sizeof(OriginX));
  FileIn.read((char*)&OriginY, sizeof(OriginY));
  FileIn.read((char*)&Width, sizeof(Width));
  FileIn.read((char*)&Height, sizeof(Height));
  FileIn.read((char*)&TexelSize, sizeof(TexelSize));
  FileIn.read((char*)&Descriptor, sizeof(Descriptor));

  tex2d->type = GL_UNSIGNED_BYTE;
  tex2d->width = Width; tex2d->height = Height;
  if(TexelSize == 24) {tex2d->internalFormat = GL_RGB8; tex2d->format = GL_BGR; }
  else if(TexelSize == 32) {tex2d->internalFormat = GL_RGBA8; tex2d->format = GL_BGRA; }
  else assert(0);

  if (FileIn.fail() || FileIn.bad()) { assert(0); return tex2d; };

  switch(ImageType) {
    default:
      assert(0); return tex2d;
    case 2:
      FileIn.seekg(18 + ColorMapLength, std::ios::beg);

      char* IdentificationField = new char[IdentificationFieldSize + 1];
      FileIn.read(IdentificationField, IdentificationFieldSize);
      IdentificationField[IdentificationFieldSize] = '\0';
      delete[] IdentificationField;

      std::size_t DataSize = Width * Height * (TexelSize >> 3);
      tex2d->data = new char [DataSize];
      FileIn.read((char*)tex2d->data, std::streamsize(DataSize));

      if(FileIn.fail() || FileIn.bad()) return tex2d;
      break;
  }

  FileIn.close();

  return tex2d;
}

Texture2DPtr gl_helpers::createTexture(std::string aFilename, bool aFilters) {
  GLuint TextureName(0);
  glGenTextures(1, &TextureName);
  Texture2DPtr Texture = loadTGA(aFilename);
  glBindTexture(GL_TEXTURE_2D, TextureName);
  if (aFilters) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
  } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 0);
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  };
  glTexImage2D(GL_TEXTURE_2D, 0, Texture->internalFormat, Texture->width, Texture->height,
      0, Texture->format, Texture->type, Texture->data);
  glBindTexture(GL_TEXTURE_2D, 0);
  delete (uint8_t*)Texture->data;
  Texture->Id = TextureName;
  Texture->texHandle = glGetTextureHandleARB(Texture->Id);
  glMakeTextureHandleResidentARB(Texture->texHandle);
  return Texture;
}

GLuint gl_helpers::createBufferObject(GLenum bufferType, size_t buff_size, void *data, GLenum usage) {
  GLuint buffId(0);
  glGenBuffers(1,&buffId);
  glBindBuffer(bufferType, buffId);
  glBufferData(bufferType, buff_size, data, usage);
  return buffId;
}

GLuint gl_helpers::CreateBufferObject(size_t buff_size, void **data, GLenum bufferType) {
    GLuint buf;
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glGenBuffers(1, &buf);
    if (bufferType == GL_DRAW_INDIRECT_BUFFER) glBindBuffer(bufferType, buf);
    else glBindBufferBase(bufferType, 0, buf);
    glBufferStorage(bufferType, buff_size, NULL, flags | GL_DYNAMIC_STORAGE_BIT );
    *data = glMapBufferRange(bufferType, 0, buff_size, flags );
    glBindBuffer(bufferType, 0);
    return buf;
}

int gl_helpers::GetUniformLocation(GLuint shaderId, std::string &name) {
  ShaderMapIterator uniforms = _unifroms.find(shaderId);
  if (uniforms != _unifroms.end()) {
    UniformMaps::iterator it = uniforms->second.find(name);
    if (it != uniforms->second.end()) return it->second;
    else {
      int loc = glGetUniformLocation(shaderId, name.c_str());
      uniforms->second.insert(std::pair<std::string, GLuint>(name,loc));
      return loc;
    }
  } else {
      UniformMaps umap;
      _unifroms.insert(std::pair<GLuint, UniformMaps>(shaderId, umap));
      int loc = glGetUniformLocation(shaderId, name.c_str());
      umap.insert(std::pair<std::string, GLuint>(name,loc));
      return loc;
  }
  return -1;
}

void gl_helpers::setUniform(ShaderPtr shader, std::string name, const int32_t value) {
  int loc = GetUniformLocation(shader->Id,name);
  if (loc != -1) glProgramUniform1i(shader->Id,loc,value);
}

void gl_helpers::setUniform(ShaderPtr shader, std::string name, const glm::vec2 value) {
  int loc = GetUniformLocation(shader->Id,name);
  if (loc != -1) glProgramUniform2fv(shader->Id,loc,1,glm::value_ptr(value));
}

void gl_helpers::setUniform(ShaderPtr shader, std::string name, const glm::vec3 value) {
  int loc = GetUniformLocation(shader->Id,name);
  if (loc != -1) glProgramUniform3fv(shader->Id,loc,1,glm::value_ptr(value));
}

void gl_helpers::setUniform(ShaderPtr shader, std::string name, const glm::vec4 value) {
  int loc = GetUniformLocation(shader->Id,name);
  if (loc != -1) glProgramUniform4fv(shader->Id,loc,1,glm::value_ptr(value));
}

void gl_helpers::setUniform(ShaderPtr shader, std::string name, const glm::mat4 value) {
  int loc = GetUniformLocation(shader->Id,name);
  if (loc != -1) glProgramUniformMatrix4fv(shader->Id,loc,1,false,glm::value_ptr(value));
}

void gl_helpers::uploadData(GLuint buffId, GLuint data_size, void * data_ptr, GLuint offset) {
  glBindBuffer(GL_ARRAY_BUFFER, buffId);
  glBufferSubData(GL_ARRAY_BUFFER, offset, data_size, data_ptr);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void gl_helpers::getData(GLuint buffId, GLuint data_size, void * data_ptr, GLuint offset) {
  glBindBuffer(GL_ARRAY_BUFFER, buffId);
  glGetBufferSubData(GL_ARRAY_BUFFER, offset, data_size, data_ptr);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void Mesh::buildVAO() {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vId);
  glEnableVertexAttribArray(POSITION);
  glVertexAttribPointer(POSITION, vcc, GL_FLOAT, false, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, nId);
  glEnableVertexAttribArray(NORMAL);
  glVertexAttribPointer(NORMAL, ncc, GL_FLOAT, false, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, tId);
  glEnableVertexAttribArray(TEXCOORD);
  glVertexAttribPointer(TEXCOORD, tcc, GL_FLOAT, false, 0, 0);
  glBindVertexArray(0);
}

void Mesh::Bind(bool state) {
  if (!vao) buildVAO();
  if (state) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iId);
  } else {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
}

void Mesh::Draw() {
  glDrawElements(face_type, elements, GL_UNSIGNED_INT, 0 );
}

inline int next_p2 ( int a )
{
	int rval=1;
	while(rval<a) rval<<=1;
	return rval;
}

void gl_helpers::CheckOpenGLError(std::string point) {
  GLenum errCode;
  const GLubyte *errString;

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    errString = gluErrorString(errCode);
    printf("OpenGL Error: %s: %s\n", point.c_str(), errString);
  }
}

glm::vec4 vec_from_string (std::string s){

    std::istringstream ss(s);

    std::string result;
    glm::vec4 v(0.0);

    if( std::getline( ss, result , '{') )
    {
        std::getline( ss, result , '}');
        std::istringstream iss( result );

        std::string token;

        if (std::getline( iss, token, ';' ))
          v.x = atof(token.c_str());
        if (std::getline( iss, token, ';' ))
          v.y = atof(token.c_str());
        if (std::getline( iss, token, ';' ))
          v.z = atof(token.c_str());
        if (std::getline( iss, token, ';' ))
          v.w = atof(token.c_str());

    }
    return v;
}

static int mat_handler(void* user, const char* section, const char* name, const char* value){
    MatLibrary *mat_lib = (MatLibrary*)user;
    std::map<std::string,Material*>::iterator it;
    it = mat_lib->find(section);
    Material *mat;
    if (it == mat_lib->end()) {
       mat = new Material(section);
       mat_lib->insert( std::pair<std::string, Material*>(section,mat));
    } else {
      mat = it->second;
    }

    Texture2DPtr tex;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH(section, "Ka")) {
       mat->Ka = vec_from_string(value);
    } else if (MATCH(section, "Kd")) {
       mat->Kd = vec_from_string(value);
    } else if (MATCH(section, "Ks")) {
       mat->Ks = vec_from_string(value);
    } else if (MATCH(section, "Ns")) {
       mat->Ns = atoi(value);
    } else if (MATCH(section, "illum")) {
       mat->illum = atoi(value);
    } else if (MATCH(section, "map_opacity")) {
       mat->map_opacity = strdup(value);
       if ( strcmp(value, "") ) {
         if (!mat_lib->getTexture(value,tex)) {
          tex = gl_helpers::createTexture(mat_lib->getPath()+value, true);
          mat_lib->textures.insert( std::pair<std::string,Texture2DPtr>(value, tex));
         }
       }
    } else if (MATCH(section, "map_bump")) {
       mat->map_bump = strdup(value);
       if ( strcmp(value, "") ) {
         if (!mat_lib->getTexture(value,tex)) {
          tex = gl_helpers::createTexture(mat_lib->getPath()+value, true);
          mat_lib->textures.insert( std::pair<std::string,Texture2DPtr>(value, tex));
         }
       }
    } else if (MATCH(section, "bump")) {
       mat->bump = strdup(value);
       if ( strcmp(value, "") ) {
         if (!mat_lib->getTexture(value,tex)) {
          tex = gl_helpers::createTexture(mat_lib->getPath()+value, true);
          mat_lib->textures.insert( std::pair<std::string,Texture2DPtr>(value, tex));
         }
       }
    } else if (MATCH(section, "map_d")) {
       mat->map_d = strdup(value);
       if ( strcmp(value, "") ) {
         if (!mat_lib->getTexture(value,tex)) {
          tex = gl_helpers::createTexture(mat_lib->getPath()+value, true);
          mat_lib->textures.insert( std::pair<std::string,Texture2DPtr>(value, tex));
         }
       }
    } else if (MATCH(section, "map_Kd")) {
       mat->map_Kd = strdup(value);
       if ( strcmp(value, "") ) {
         if (!mat_lib->getTexture(value,tex)) {
          tex = gl_helpers::createTexture(mat_lib->getPath()+value, true);
          mat_lib->textures.insert( std::pair<std::string,Texture2DPtr>(value, tex));
         }
       }
    } else if (MATCH(section, "map_Ks")) {
       mat->map_Ks = strdup(value);
       if ( strcmp(value, "") ) {
         if (!mat_lib->getTexture(value,tex)) {
          tex = gl_helpers::createTexture(mat_lib->getPath()+value, true);
          mat_lib->textures.insert( std::pair<std::string,Texture2DPtr>(value, tex));
         }
       }
    } else if (MATCH(section, "map_Ka")) {
       mat->map_Ka = strdup(value);
       if ( strcmp(value, "") ) {
         if (!mat_lib->getTexture(value,tex)) {
          tex = gl_helpers::createTexture(mat_lib->getPath()+value, true);
          mat_lib->textures.insert( std::pair<std::string,Texture2DPtr>(value, tex));
         }
       }
    } else if (MATCH(section, "map_Ns")) {
       mat->map_Ns = strdup(value);
       if ( strcmp(value, "") ) {
         if (!mat_lib->getTexture(value,tex)) {
          tex = gl_helpers::createTexture(mat_lib->getPath()+value, true);
          mat_lib->textures.insert( std::pair<std::string,Texture2DPtr>(value, tex));
         }
       }
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

MatLibrary::MatLibrary(std::string file_name) {
  SplitFilename(file_name);
  ini_parse(file_name.c_str(), mat_handler, this);

}

bool MatLibrary::getTexture(std::string file_name, Texture2DPtr &tex) {
  std::map<std::string,Texture2DPtr>::iterator it;
  it = textures.find(file_name);
  if (it != textures.end()) {
    tex = it->second;
    return true;
  } else return false;
}

static int inst_handler(void* user, const char* section, const char* name, const char* value){
    Instances *inst = (Instances*)user;
    InstanceInfo *obj;
    std::map<std::string,InstanceInfo*>::iterator it;
    it = inst->find(section);
    if (it == inst->end()) {
        obj = new InstanceInfo(section);
        inst->insert( std::pair<std::string,InstanceInfo*>(section,obj));
    } else obj = it->second;


    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    obj->ModelMatrix = glm::mat4(1.0f);
    if (MATCH(section, "MaterialName")) {
       obj->Mat = inst->mat_lib->find(value)->second;
    } else if (MATCH(section, "ElementsCount")) {
       obj->ElementsCount = atoi(value);
    } else if (MATCH(section, "baseVertex")) {
       obj->baseVertex = atoi(value);
    } else if (MATCH(section, "baseIndex")) {
       obj->baseIndex = atoi(value);
    } else if (MATCH(section, "ModelMatrix")) {
       //not implemented yet
    } else {
       return 0;  /* unknown section/name, error */
    }
    return 1;
}

Instances::Instances(std::string file_name, MatLibrary *ml): mat_lib(ml) {
  ini_parse(file_name.c_str(), inst_handler, this);
}
