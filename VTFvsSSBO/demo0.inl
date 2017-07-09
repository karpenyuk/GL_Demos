glm::mat4 ViewMatrix;
glm::mat4 ProjMatrix;

GLuint routineC1;
GLuint routineC2;
GLuint routineC3;

MeshPtr sphere;
ShaderPtr shader;

struct InstanceInfo {
  glm::mat4 Model;
  GLuint MatId[4];
};

SSBOPtr<InstanceInfo> objects;
InstanceInfo *instancePtr;

const char vShader[] = SHADER_SOURCE(
    layout(location = 0) in vec3 in_Position;
    layout(location = 1) in vec3 in_Normal;

    struct InstanceInfo {
      mat4 Model;
      uint MatId[4];
    };

    layout(std430, binding=1) buffer Instances { InstanceInfo objects[]; };

    subroutine vec4 colorMaterial();

    subroutine ( colorMaterial ) vec4 redColor() { return vec4(1.0, 0.0, 0.0, 1.0); };
    subroutine ( colorMaterial ) vec4 greenColor() { return vec4(0.0, 1.0, 0.0, 1.0); };
    subroutine ( colorMaterial ) vec4 blueColor() {return vec4(0.0, 0.0, 1.0, 1.0); };

    out vec4 color;

    uniform mat4 Proj;
    uniform mat4 View;
    subroutine uniform colorMaterial mySelections[3];

    void main(){
        float l = dot(mat3(View)*in_Normal.xyz, vec3(1.0, 1.0, 1.0));
        uint func = objects[gl_InstanceID].MatId[0];
        color = mySelections[func]()*max(l,0.0);
        gl_Position = Proj*View*objects[gl_InstanceID].Model*vec4(in_Position.xyz,1.0);
    };
);

const char fShader[] = SHADER_SOURCE(
  in vec4 color;
  layout(location = 0) out vec4 FragColor;
  void main()
  {
    FragColor = color;
  }
);

void initialize() {

  sphere = Primitives::CreateSphere(1.0f,16,16);

  ViewMatrix = glm::lookAt( glm::vec3( 0.f, 0.f, 5.0f ),glm::vec3( 0.f, 0.f, 0.f ),glm::vec3( 0.0f, 1.0f, 0.0f ) );

  shader = gl_helpers::createProgram(vShader,fShader);

  routineC1 = glGetSubroutineIndex(shader->Id, GL_VERTEX_SHADER, "redColor");
  routineC2 = glGetSubroutineIndex(shader->Id, GL_VERTEX_SHADER, "greenColor");
  routineC3 = glGetSubroutineIndex(shader->Id, GL_VERTEX_SHADER, "blueColor");

  GLuint funcLoc = glGetSubroutineUniformLocation(shader->Id, GL_VERTEX_SHADER, "mySelections");

  objects = SSBOPtr<InstanceInfo>(new SSBO<InstanceInfo>(3, GL_SHADER_STORAGE_BUFFER));
  instancePtr = objects->getData();

  for (int i = -1; i < 2; ++i) {
    instancePtr[i+1].Model = glm::translate(glm::mat4(1.0f), glm::vec3(i*2.1f, 0.0f, 0.0f));
    instancePtr[i+1].MatId[0] = i+1;
  }

  //Setup OGL environment - background color, blending and depth test
  glClearColor ( 0.5, 0.5, 0.5, 1.0 );
  glClearDepth(1.0);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

}

void reshape(int w,int h) {
  ProjMatrix = glm::perspective(45.0f, (float)w / (float)h, 0.1f, 200.0f);
}

void update(int value) {
  //Do nothing
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


  objects->Bind(1);
  //materials->Bind(2);

  GLuint func[3] = {routineC1, routineC2, routineC3};
  glUniformSubroutinesuiv(GL_VERTEX_SHADER, 3, &func[0]);


  sphere->Bind(true);
    glDrawElementsInstanced(sphere->getFaceType(), sphere->getElementsCount(), GL_UNSIGNED_INT, 0, 3);
  sphere->Bind(false);

  glUseProgram(0);

}
