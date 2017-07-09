#include "ini.h"
float angle = 0.0f;

glm::mat4 ViewMatrix;
glm::mat4 ProjMatrix;
glm::mat4 RotationMatrix;

GLuint getOpacityFromMap;
GLuint getOpacityValue;
GLuint getNormalFromMap;
GLuint getAlbedoFromMap;
GLuint getAlbedoValue;
GLuint getSpecularFromMap;
GLuint getSpecularValue;
GLuint doNothingVec4;
GLuint doNothingVec3;
GLuint doNothingFloat;

MeshPtr sphere;
ShaderPtr shader;

MeshPtr obj;
MatLibrary *ml;
Instances *inst;

struct ObjectInfo {
  glm::mat4 Pivot;
  glm::mat4 Model;
  GLuint MatId[4]; //for alignments
  GLuint norm_func;
  GLuint opac_func;
  GLuint albedo_func;
  GLuint spec_func;
};

SSBOPtr<ObjectInfo> objects;
ObjectInfo *instancePtr;

struct MatInfo {
    glm::vec4 Ka;
    glm::vec4 Kd;
    glm::vec4 Ks;
    GLuint64 map_opacity;
    GLuint64 map_bump; //Normal
    GLuint64 bump;
    GLuint64 map_d; //Alpha
    GLuint64 map_Kd;
    GLuint64 map_Ks;
    GLuint64 map_Ka;
    GLuint64 map_Ns;
};

SSBOPtr<MatInfo> materials;
MatInfo *matPtr;

struct DrawCommand {
    GLuint  count;
    GLuint  primCount;
    GLuint  firstIndex;
    GLuint  baseVertex;
    GLuint  baseInstance;
    DrawCommand(GLuint el_count, GLuint index,  GLint basev = 0){
        count = el_count; firstIndex = index;
        primCount = 1; baseVertex = basev; baseInstance = 0;
    }
};

class CommandBuffer: public std::vector<DrawCommand> {
private:
    Instances *inst;
public:
    CommandBuffer(Instances *instances) : inst(instances) {
        std::map<std::string,InstanceInfo*>::iterator it;
        for (it = inst->begin(); it != inst->end(); ++it) {
            push_back(DrawCommand(it->second->ElementsCount,
                                  it->second->baseIndex));
        }

    }
};

CommandBuffer *cmd;


const char fObjShader[] = SHADER_SOURCE(
  in vec3 texCoord;
  in vec3 normal;
  in vec3 WorldPos;
  in vec3 ViewPos;
  in vec3 LightPos;


  in flat uint matId;

  layout(location = 0) out vec4 FragColor;

  vec3 Normal;

  struct MatInfo {
    vec4 Ka;
    vec4 Kd;
    vec4 Ks;

    uvec2 map_opacity;
    uvec2 map_bump; //Normal
    uvec2 bump;
    uvec2 map_d; //Alpha
    uvec2 map_Kd;
    uvec2 map_Ks;
    uvec2 map_Ka;
    uvec2 map_Ns;
  };

  layout(std430, binding = 2) buffer Materials {
    MatInfo mat[];
  };

  struct ObjectInfo {
    mat4 Pivot;
    mat4 Model;
    uint MatId[4];
    uint norm_func;
    uint opac_func;
    uint albedo_func;
    uint spec_func;
  };

  in flat ObjectInfo object;

   subroutine float floatMaterial(in uint, in float);
   subroutine vec3 vec3Material(in uint, in vec3);
   subroutine vec4 vec4Material(in uint, in vec4);

    subroutine ( floatMaterial ) float getOpacityFromMap(in uint MatId, in float alpha){
        return texture2D(sampler2D(mat[MatId].map_d), texCoord.st).r * alpha;
    }
    subroutine ( floatMaterial ) float getOpacityValue(in uint MatId, in float alpha){
        return mat[MatId].Kd.a * alpha;
    }
    subroutine ( vec3Material ) vec3 getNormalFromMap(in uint MatId, in vec3 Normal){
        vec3 tangentNormal = texture2D(sampler2D(mat[MatId].map_bump), texCoord.st).xyz * 2.0 - 1.0;

        vec3 Q1  = dFdx(WorldPos);
        vec3 Q2  = dFdy(WorldPos);
        vec2 st1 = dFdx(texCoord.st);
        vec2 st2 = dFdy(texCoord.st);

        vec3 N   = Normal;
        vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
        vec3 B  = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);

        return normalize(TBN * tangentNormal);
    }
    subroutine ( vec4Material ) vec4 getAlbedoFromMap(in uint MatId, in vec4 Diffuse){
        return texture2D(sampler2D(mat[MatId].map_Kd), texCoord.st) * Diffuse;
    }
    subroutine ( vec4Material ) vec4 getAlbedoValue(in uint MatId, in vec4 Diffuse){
        return mat[MatId].Kd * Diffuse;
    }
    subroutine ( vec4Material ) vec4 getSpecularFromMap(in uint MatId, in vec4 Specular){
        vec4 spec = texture2D(sampler2D(mat[MatId].map_Ks), texCoord.st);
        vec3 E = normalize(-ViewPos);
        vec3 R = reflect(-LightPos, Normal);
        float specular = pow(max(dot(R, E), 0.0), mat[MatId].Ks.a);
        return Specular * spec * specular;
    }
    subroutine ( vec4Material ) vec4 getSpecularValue(in uint MatId, in vec4 Specular){
        vec4 spec = mat[MatId].Ks;
        vec3 E = normalize(-ViewPos);
        vec3 R = reflect(-LightPos, Normal);
        float specular = pow(max(dot(R, E), 0.0), mat[MatId].Ks.a);
        return Specular * spec * specular;
    }
    subroutine ( vec4Material ) vec4 doNothingVec4(in uint MatId, in vec4 Value){
        return Value;
    }
    subroutine ( vec3Material ) vec3 doNothingVec3(in uint MatId, in vec3 Value){
        return Value;
    }
    subroutine ( floatMaterial ) float doNothingFloat(in uint MatId, in float Value){
        return Value;
    }

    subroutine uniform floatMaterial floatMaterials[3];
    subroutine uniform vec3Material vec3Materials[2];
    subroutine uniform vec4Material vec4Materials[5];

    vec4 Lighting() {
        Normal = vec3Materials[object.norm_func](object.MatId[0], Normal);
        vec4 color = vec4Materials[object.albedo_func](object.MatId[0], vec4(1.0));
        vec3 L = normalize(LightPos);
        float l = clamp(dot(-L,Normal), 0.0, 1.0);
        color.rgb = color.rgb*(0.2+0.8*l);
        color.a *= floatMaterials[object.opac_func](object.MatId[0], 1.0);
        vec4 spec = vec4Materials[object.spec_func](object.MatId[0], vec4(1.0));
        color.rgb += spec.rgb;
        return color;
    }

  void main()
  {
    Normal = normalize(normal);
    vec4 final_color = Lighting();
    FragColor = final_color;
  }
);

const char vObjShader[] = SHADER_SOURCE(
  layout(location = 0) in vec3 in_Position;
  layout(location = 1) in vec3 in_Normal;
  layout(location = 2) in vec3 in_TexCoord;

  out vec3 normal;

  out vec3 texCoord;
  out vec3 WorldPos;
  out vec3 ViewPos;
  out vec3 LightPos;

  out flat uint matId;

  uniform mat4 Proj;
  uniform mat4 View;

  uniform int inst_id;

  uniform vec4 lpos;

  struct ObjectInfo {
    mat4 Pivot;
    mat4 Model;
    uint MatId[4];
    uint norm_func;
    uint opac_func;
    uint albedo_func;
    uint spec_func;
  };
  layout(std430, binding = 1) buffer Objects {
      ObjectInfo obj[];
  };

  out flat ObjectInfo object;

  void main()
  {
        mat4 Model = obj[gl_DrawIDARB].Model;
        object = obj[gl_DrawIDARB];

        texCoord = in_TexCoord;

        mat4 ViewModel = View*Model;

        LightPos = -normalize(View*lpos).xyz;

        mat3 nm = mat3(inverse(ViewModel));
        normal = normalize(in_Normal*nm);

        vec4 pos = Model*vec4(in_Position.xyz,1.0);
        WorldPos = pos.xyz;
        ViewPos = vec3(View*pos);

        gl_Position = Proj*ViewModel*vec4(in_Position.xyz,1.0);

  }
);


void initialize() {

  obj = Primitives::CreateFromFile("..\\..\\media\\sponza.ivnt");
  ml = new MatLibrary("..\\..\\media\\sponza.mat");
  inst = new Instances("..\\..\\media\\sponza.objs", ml);
  cmd = new CommandBuffer(inst);

  printf("Triangles count: %d \n",obj->getElementsCount()/3);
  printf("Objects count: %d \n",cmd->size());

  ViewMatrix = glm::lookAt( glm::vec3( 30.f, 20.f, 100.0f ),glm::vec3( 0.f, 0.f, 0.f ),glm::vec3( 0.0f, 1.0f, 0.0f ) );

  shader = gl_helpers::createProgram(vObjShader,fObjShader);

  getOpacityFromMap = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "getOpacityFromMap");
  getOpacityValue = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "getOpacityValue");
  getNormalFromMap = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "getNormalFromMap");
  getAlbedoFromMap = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "getAlbedoFromMap");
  getAlbedoValue = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "getAlbedoValue");
  getSpecularFromMap = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "getSpecularFromMap");
  getSpecularValue = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "getSpecularValue");
  doNothingVec4 = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "doNothingVec4");
  doNothingVec3 = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "doNothingVec3");
  doNothingFloat = glGetSubroutineIndex(shader->Id, GL_FRAGMENT_SHADER, "doNothingFloat");

  GLuint sfloat_Loc = glGetSubroutineUniformLocation(shader->Id, GL_FRAGMENT_SHADER, "floatMaterials");
  GLuint svec3_Loc = glGetSubroutineUniformLocation(shader->Id, GL_FRAGMENT_SHADER, "vec3Materials");
  GLuint svec4_Loc = glGetSubroutineUniformLocation(shader->Id, GL_FRAGMENT_SHADER, "vec4Materials");

  printf("sfloat_Loc: %d\n", sfloat_Loc);
  printf("svec3_Loc: %d\n", svec3_Loc);
  printf("svec4_Loc: %d\n", svec4_Loc);

  objects = SSBOPtr<ObjectInfo>(new SSBO<ObjectInfo>(inst->size(), GL_SHADER_STORAGE_BUFFER));
  instancePtr = objects->getData();

  materials = SSBOPtr<MatInfo>(new SSBO<MatInfo>(ml->size(),GL_SHADER_STORAGE_BUFFER));
  MatInfo *matPtr = materials->getData();

  //Fill SSBO structures

  {
    std::map<std::string,Material*>::iterator it;
    int i = 0;
    for (it = ml->begin(); it != ml->end(); ++it) {
      matPtr[i].Ka = it->second->Ka;
      matPtr[i].Kd = it->second->Kd;
      matPtr[i].Ks = it->second->Ks;
      matPtr[i].Ks.w = (float)it->second->Ns;
      it->second->matIdx = i; //Link Material object with MatInfo object
      printf("Material %d : %s\n", i, it->second->name.c_str());

      Texture2DPtr tex;
      if (ml->getTexture(it->second->map_opacity, tex)) matPtr[i].map_opacity = tex->texHandle;
      else matPtr[i].map_opacity = 0;
      if (ml->getTexture(it->second->map_bump, tex)) matPtr[i].map_bump = tex->texHandle;
      else matPtr[i].map_bump = 0;
      if (ml->getTexture(it->second->bump,   tex)) matPtr[i].bump =   tex->texHandle;
      else matPtr[i].bump = 0;
      if (ml->getTexture(it->second->map_d,  tex)) matPtr[i].map_d =  tex->texHandle;
      else matPtr[i].map_d = 0;
      if (ml->getTexture(it->second->map_Kd, tex)) matPtr[i].map_Kd = tex->texHandle;
      else matPtr[i].map_Kd = 0;
      if (ml->getTexture(it->second->map_Ks, tex)) matPtr[i].map_Ks = tex->texHandle;
      else matPtr[i].map_Ks = 0;
      if (ml->getTexture(it->second->map_Ka, tex)) matPtr[i].map_Ka = tex->texHandle;
      else matPtr[i].map_Ka = 0;
      if (ml->getTexture(it->second->map_Ns, tex)) matPtr[i].map_Ns = tex->texHandle;
      else matPtr[i].map_Ns = 0;
      ++i;
    }
  }

  RotationMatrix = glm::rotate(glm::mat4(1.0f), 90.0f, glm::vec3(0.0f,1.0f,0.0f));


  {
    const GLuint DO_NOTHING = 0;
    const GLuint OPACITY_FROM_MAP = 1;
    const GLuint OPACITY_FROM_VALUE = 2;

    const GLuint NORMAL_FROM_MAP = 1;
    const GLuint ALBEDO_FROM_MAP = 1;
    const GLuint ALBEDO_FROM_VALUE = 2;

    const GLuint SPECULAR_FROM_MAP = 3;
    const GLuint SPECULAR_FROM_VALUE = 4;

    std::map<std::string,InstanceInfo*>::iterator it;
    int i = 0;
    for (it = inst->begin(); it != inst->end(); ++it) {
        std::map<std::string,InstanceInfo*>::iterator pit;
        pit = inst->find(it->second->Parent);
        if (pit != inst->end()) instancePtr[i].Pivot = pit->second->ModelMatrix;
        else instancePtr[i].Pivot = glm::mat4(1.0f);
        instancePtr[i].Model = it->second->ModelMatrix * RotationMatrix *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.1, 0.1, 0.1));
        instancePtr[i].MatId[0] = it->second->Mat->matIdx;
        instancePtr[i].MatId[1] = it->second->Mat->matIdx;
        instancePtr[i].MatId[2] = it->second->Mat->matIdx;
        instancePtr[i].MatId[3] = 1000;

        if (it->second->Mat->map_bump != "") instancePtr[i].norm_func = NORMAL_FROM_MAP;//1
        else instancePtr[i].norm_func = DO_NOTHING; //0

        if (it->second->Mat->map_d != "") instancePtr[i].opac_func = OPACITY_FROM_MAP;//1
        else if (it->second->Mat->Kd.a != 1.0) instancePtr[i].opac_func = OPACITY_FROM_VALUE;//2
        else instancePtr[i].opac_func = DO_NOTHING; //0

        if (it->second->Mat->map_Kd != "") instancePtr[i].albedo_func = ALBEDO_FROM_MAP;//1
        else instancePtr[i].albedo_func = ALBEDO_FROM_VALUE;//2

        if (it->second->Mat->map_Ks != "") instancePtr[i].spec_func = SPECULAR_FROM_MAP;//3
        else instancePtr[i].spec_func = SPECULAR_FROM_VALUE;//4

        it->second->InstId = i; //Link InstanceInfo object with ObjectInfo object
        printf("Instance %d : %s, MatId: %d : %s \n", i, it->second->InstanceName.c_str(),
               it->second->Mat->matIdx, it->second->Mat->name.c_str());
        ++i;
    }
  }


  //Setup OGL environment - background color, blending and depth test
  glClearColor ( 0.2, 0.2, 0.2, 1.0 );
  glClearDepth(1.0);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

}

void reshape(int w,int h) {
  ProjMatrix = glm::perspective(45.0f, (float)w / (float)h, 0.1f, 2000.0f);
}

void update(int value) {
  RotationMatrix = glm::rotate(RotationMatrix, 0.05f, glm::vec3(1.0f,1.0f,1.0f));
}

void release(void) {
  //Do nothing
}


void display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(shader->Id);
  gl_helpers::setUniform(shader,std::string("Proj"),ProjMatrix);
  gl_helpers::setUniform(shader,std::string("View"),ViewMatrix);

  glm::vec4 lp = ViewMatrix[3]; lp.y = 100.0f; lp.x = -500.0f;
  gl_helpers::setUniform(shader,std::string("lpos"), lp);

  objects->Bind(1);
  materials->Bind(2);

  GLuint func[10] = {
      doNothingFloat, getOpacityFromMap,getOpacityValue,
      doNothingVec3, getNormalFromMap,
      doNothingVec4, getAlbedoFromMap, getAlbedoValue, getSpecularFromMap, getSpecularValue
  };
  glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 10, &func[0]);


  obj->Bind(true);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, cmd->data(), cmd->size(), 0);
  obj->Bind(false);

  glUseProgram(0);

}
