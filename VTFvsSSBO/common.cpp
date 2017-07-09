#include "common.h"
#include <glm/gtc/type_ptr.hpp>

namespace demo {

  int viewport[4] = {0,0,0,0};

  std::string text;
  bool stop;

  #define DEMO1

  #ifdef DEMO1
      //Base Subroutine Test Demo
      const char DEMO_NAME[] = "Vertex Texture Fetch";
      #include "demo0.inl"
  #endif // DEMO1

} //namespace demo
