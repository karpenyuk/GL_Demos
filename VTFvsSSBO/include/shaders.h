#ifndef SHADERS_H
#define SHADERS_H

const char vShader[] = " \
#version 440\n\
out vec2 TexCoord;\
out vec4 pos;\
out vec3 norm;\
layout(location = 0) in vec3 in_Position;\
layout(location = 1) in vec3 in_Normal;\
layout(location = 2) in vec2 in_TexCoord;\
uniform mat4 NormalMatrix;\
uniform mat4 ModelView;\
uniform mat4 Proj;\
void main ()\
{\
  TexCoord = in_TexCoord.xy;\
  norm = normalize(mat3(NormalMatrix)*in_Normal);\
  pos = ModelView*vec4(in_Position,1.0);\
  gl_Position = Proj*pos;\
}\
";

const char fShader[] = "\
#version 440\n\
uniform vec4 LightPos;\
uniform sampler2D DiffuseMap;\
in vec2 TexCoord;\
in vec3 norm;\
in vec4 pos;\
layout(location = 0) out vec4 FragColor;\
layout(early_fragment_tests) in;\
void main() \
{\
  vec3 l = normalize ( LightPos.xyz - pos.xyz );\
  float lambertTerm = max(dot(normalize(norm),l),0.0);\
  vec4 TexColor = texture2D(DiffuseMap,TexCoord.xy);\
  FragColor = vec4(lambertTerm*TexColor.rgb,1.0);\
  FragColor = TexColor*vec4(0.0, 1.0, 0.0,1.0);\
}\
";


const char vInstanceShader[] = " \
#version 440\n\
out vec2 TexCoord;\
layout(location = 0) in vec3 in_Position;\
layout(location = 1) in vec3 in_Normal;\
layout(location = 2) in vec2 in_TexCoord;\
layout(std140, binding=3) buffer Instances { vec4 offsets[]) };\
uniform mat4 NormalMatrix;\
uniform mat4 ModelView;\
uniform mat4 Proj;\
uniform vec2 char_scale;\
void main ()\
{\
  TexCoord = char_scale*in_TexCoord.xy + offsets[gl_InstanceID].zw;\
  vec4 p = vec3(in_Position, 1.0);\
  p.xy += offsets[gl_InstanceID].xy;\
  gl_Position = Proj*(ModelView*p);\
}\
";

const char fInstanceShader[] = "\
#version 440\n\
uniform sampler2D DiffuseMap;\
in vec2 TexCoord;\
layout(location = 0) out vec4 FragColor;\
layout(early_fragment_tests) in;\
void main() \
{\
  vec4 TexColor = texture2D(DiffuseMap,TexCoord.xy);\
  FragColor = TexColor*vec4(0.0, 1.0, 0.0,1.0);\
}\
";

#endif //SHADERS_H

