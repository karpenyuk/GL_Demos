#ifndef COMMON_H
#define COMMON_H

#define MCUBE
//#define BENCHMARK

#include <stdlib.h>
#include <stdio.h>
#include "gl_helpers.h"
#include "primitives.h"

#define STRINGIF_(EXPR) #EXPR
#define STRINGIFY(EXPR) STRINGIF_(EXPR)

#define SHADER_SOURCE(SOURCE) "#version 440\n #extension GL_ARB_shader_draw_parameters : enable\n\
    #extension GL_ARB_bindless_texture : require\n\
    #extension GL_ARB_shading_language_packing : require\n\
    " STRINGIFY(SOURCE)

namespace demo {

  //Test with 100x100 instances
  extern const uint32_t ROWS;
  extern const uint32_t COLS;
  extern const uint32_t INSTANCES_COUNT;
  extern const char DEMO_NAME[];

  extern uint32_t INSTANCES_NUM;

  extern int viewport[4];

  extern std::string text;
  extern bool stop;

  void display(void);
  void initialize();
  void update(int value);
  void reshape(int w,int h);
  void release(void);
  double PerfTest(uint32_t inst_count);

} //namespace demo

#endif // COMMON_H
