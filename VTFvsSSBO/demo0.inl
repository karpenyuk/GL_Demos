glm::mat4 ViewMatrix;
glm::mat4 ProjMatrix;

MeshPtr mesh;
ShaderPtr shader;
Texture2DPtr hm;

TBOPtr<float> tex_buff;
float *texPtr;
SSBOPtr<float> ssbo_buff;
float *ssboPtr;

GLuint ssboProc;
GLuint tboProc;
GLuint zeroProc;
GLuint noiseProc;
GLuint normalProc;

GLuint stboId;
GLuint noiseId;


const char vShader[] = SHADER_SOURCE(

    layout(location = 0) in vec3 in_Position;
    layout(location = 1) in vec3 in_Normal;
    layout(location = 2) in vec3 in_TexCoord;

    uniform mat4 Proj;
    uniform mat4 View;

    uniform samplerBuffer tbo_tex;

    layout(std430, binding = 3) readonly buffer hm {
        float heights[];
    };

    float hash( vec2 p ) {
      float h = dot(p,vec2(127.1,311.7));
      return fract(sin(h)*43758.5453123);
    }
    float noise( in vec2 p ) {
      vec2 i = floor( p );
      vec2 f = fract( p );
      vec2 u = f*f*(3.0-2.0*f);
      return -1.0+2.0*mix( mix( hash( i + vec2(0.0,0.0) ),
                         hash( i + vec2(1.0,0.0) ), u.x),
                    mix( hash( i + vec2(0.0,1.0) ),
                         hash( i + vec2(1.0,1.0) ), u.x), u.y);
    }

    subroutine vec2 NoiseFunc(in vec2 p);

    subroutine (NoiseFunc) vec2 randomTC(in vec2 p) {
      float s = (1.0+noise(p))/2.0;
      float t = (1.0+noise(vec2(p.x,s)))/2.0;
      return vec2(s,t);
    }
    subroutine (NoiseFunc) vec2 normalTC(in vec2 p) {
      return p;
    }

    subroutine float SSBO_TBO(in int offset);

    subroutine (SSBO_TBO) float ssboValue(in int offset) {
      return heights[offset];
    }

    subroutine (SSBO_TBO) float tboValue(in int offset) {
      return texelFetch(tbo_tex, offset).r;
    }

    subroutine (SSBO_TBO) float zeroValue(in int offset) {
      return 0.0;
    }

    subroutine uniform SSBO_TBO ssbo_tboSelector;
    subroutine uniform NoiseFunc noiseSelector;

    out vec2 tc;
    void main(){
        tc = noiseSelector(in_TexCoord.st);
        vec4 p = vec4(in_Position.xyz,1.0);
        int offs = int(tc.x*1280.0+(tc.y*1280.0*1280.0));

        p.y = ssbo_tboSelector(offs)*3.0-1.5;
        gl_Position = Proj*View*p;
    }

);

const char fShader[] = SHADER_SOURCE(
  in vec2 tc;
  uniform sampler2D map;

  layout(location = 0) out vec4 FragColor;
  void main()
  {
    FragColor = vec4(texture2D(map, tc).rrr,1.0);
  }
);

void keyboard(unsigned char key, int x, int y) {
  printf(" Key %d pressed\n", key);
  switch (key)
  {
    case 's':
     stboId = ssboProc;
     break;
    case 't':
     stboId = tboProc;
     break;
    case 'z':
     stboId = zeroProc;
     break;
    case 'n':
     noiseId = noiseProc;
     break;
    case 'm':
     noiseId = normalProc;
     break;
  }
};

void initialize() {

  mesh = Primitives::CreatePlane(20.0f,20.0f,1280,1280,false);
  hm = gl_helpers::createTexture("..\\..\\media\\hm.tga",true);

  printf("HM DataSize = %d, Width=%d, Height=%d\n", hm->dataSize, hm->width, hm->height );

  uint32_t elCount = hm->width*hm->height;
  printf("Elements Count = %d\n", elCount);

  float *tbo_data = new float[elCount];
  uint8_t *ptr = (uint8_t *)hm->data;

  ssbo_buff = SSBOPtr<float>(new SSBO<float>(elCount, GL_SHADER_STORAGE_BUFFER));
  ssboPtr = ssbo_buff->getData();

  printf("Fill TBO data from Texture Data %p... ", hm->data);

  for (int i = 0; i < elCount; ++i) {
    tbo_data[i] = ptr[i*3]/255.0;
    ssboPtr[i] = tbo_data[i];
  }
  printf("Done\n");

  tex_buff = TBOPtr<float>(new TBO<float>(elCount, 4, GL_R32F, tbo_data));


  ViewMatrix = glm::lookAt( glm::vec3( 0.f, 0.f, -20.0f ),glm::vec3( 0.f, 0.f, 0.f ),glm::vec3( 0.0f, 1.0f, 0.0f ) );

  shader = gl_helpers::createProgram(vShader,fShader);

  //Setup OGL environment - background color, blending and depth test
  glClearColor ( 0.5, 0.5, 0.5, 1.0 );
  glClearDepth(1.0);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  //glPolygonMode(GL_FRONT, GL_LINE);


ssboProc = glGetSubroutineIndex(shader->Id, GL_VERTEX_SHADER, "ssboValue");
tboProc = glGetSubroutineIndex(shader->Id, GL_VERTEX_SHADER, "tboValue");
zeroProc = glGetSubroutineIndex(shader->Id, GL_VERTEX_SHADER, "zeroValue");
stboId = ssboProc;

normalProc = glGetSubroutineIndex(shader->Id, GL_VERTEX_SHADER, "normalTC");
noiseProc = glGetSubroutineIndex(shader->Id, GL_VERTEX_SHADER, "randomTC");
noiseId = normalProc;
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
  ViewMatrix = glm::lookAt( glm::vec3( 0.f, 10.f, -20.0f ),glm::vec3( 0.f, 0.f, 0.f ),glm::vec3( 0.0f, 1.0f, 0.0f ) );
  gl_helpers::setUniform(shader,std::string("Proj"),ProjMatrix);
  gl_helpers::setUniform(shader,std::string("View"),ViewMatrix);
  gl_helpers::setUniform(shader,std::string("map"),0);
  gl_helpers::setUniform(shader,std::string("tbo_tex"),1);

  GLuint subs[2];
  subs[1] = noiseId;
  subs[0] = stboId;

  glUniformSubroutinesuiv(GL_VERTEX_SHADER, 2, subs);

  ssbo_buff->Bind(3);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, hm->Id);

  tex_buff->Bind(1);

  mesh->Bind(true);
    mesh->Draw();
  mesh->Bind(false);

  glUseProgram(0);

}

