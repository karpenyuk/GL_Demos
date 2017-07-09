const char vShader[] = SHADER_SOURCE(

    layout(location = 0) in vec3 in_Position;
    layout(location = 1) in vec3 in_Normal;
    layout(location = 2) in vec3 in_TexCoord;

    uniform vec4 SceneColor; //Ecm + Acm * Acs(0.2, 0.2, 0.2, 1.0)

    struct MaterailInfo {
      uint matId;
      uint func;
    };

    struct InstanceInfo {
      mat4 Model;
      MaterailInfo Mat[2];
    };





    layout(std430, binding=1) buffer Instances { InstanceInfo objects[]; };

    struct PhongLight {
      vec4 pos;
      vec4 dir;
      vec4 ambient;
      vec4 diffuse;
      vec4 specular;
    };

    layout(std430, binding=2) buffer Lights {
        PhongLight lights[];
    };

    struct PhongMaterial {
       MaterailInfo emission;
       MaterailInfo ambient;
       MaterailInfo diffuse;
       MaterailInfo specular;
       MaterailInfo shininess;
    };

    layout(std430, binding=3) buffer Materials {
       PhongMaterial pMaterials[];
    };

    layout(std430, binding=4) buffer Textures {
       uvec2 texId[];
    };

    lod = 0;
    uint matId = objects[id].Mat[lod].matId;
    uint lightFunc = objects[id].Mat[lod].func;

    subroutine uniform lightModel getLight[3];
    subroutine uniform colorFunc getColor[3];

    vec4 color = Lights[lightFunc](matId, N, V)
    { //PhongLight
      uint func = pMaterials[matId].ambient.func;
      PhongMaterial mat = pMaterials[matId];
      vec4 amb = getColor[func](mat.ambient.matId, TexCoord);
    }

    subroutine (colorFunc) vec4 colorFromId(in uint id, in vec2 tc) {
      return unpackUnorm4x8(id);
    }

    subroutine (colorFunc) vec4 colorFromTexture(in uint id, in vec2 tc) {
      return texture(sampler2D(texId[id]), tc);
    }

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


    subroutine (colorFunc) vec4 colorFromNoise1(in uint id, in vec2 tc) {
      return vec4(noise(tc));
    }


    subroutine vec4 colorMaterial();

    subroutine ( colorMaterial ) vec4 redColor() { return vec4(1.0, 0.0, 0.0, 1.0); };
    subroutine ( colorMaterial ) vec4 greenColor() { return vec4(0.0, 1.0, 0.0, 1.0); };
    subroutine ( colorMaterial ) vec4 blueColor() {return vec4(0.0, 0.0, 1.0, 1.0); };


    uint matId;
    uint lightId;
    //in_TexCoord


    subroutine vec4 lightModel ( in vec3 N, in vec3 V, in vec2 tc );

    subroutine (lightModel) vec4 phongLight( in vec3 N, in vec3 V, in vec2 tc )
    {
        vec3 L = normalize(lights[lightId].pos-V);
        float lambertTerm = dot(N,L);
        vec4 final_color = vec4(0.0);
        if (lambertTerm > 0.0) {
          final_color =
            lights[lightId].diffuse * pmaterials[matId].diffuse * lambertTerm;
          vec3 E = normalize(-V);
          vec3 R = reflect(-L, N);
          float specular = pow(max(dot(R, E), 0.0), pmaterials[matId].shininess);
          final_color += lights[lightId].specular * pmaterials[matId].specular * specular;
        }
        return final_color;
    }

    subroutine (lightModel) vec4 lambertLight( in vec3 N, in vec3 V, in vec2 tc )
    {
        vec3 L = normalize(lights[lightId].pos-V);
        float v = -normalize(V);
        float lambert = clamp(dot(v,L),0.0, 1.0);
        return  lights[lightId].diffuse * materials[matId].diffuse * lambert;
    }



    out vec4 color;

    uniform mat4 Proj;
    uniform mat4 View;
    subroutine uniform colorMaterial mySelections[3];

    void main(){
        //vec4 final_color = SceneColor*materials[matId] + lights[lightId]*materials[matId].ambient;
        //float l = dot(in_Normal.xyz*mat3(View*objects[gl_InstanceID].Model), vec3(1.0, 1.0, 1.0));
        vec3 v = normalize(in_Position.xyz);
        vec3 lp = normalize(vec3(1.0, 1.0, 1.0));
        float l = dot(-v, -lp);
        l = clamp(l, 0.0, 1.0);
        uint func = objects[gl_InstanceID].MatId[0];
        color = mySelections[func]();
        color.rgb = normalize(in_Normal.xyz);
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
