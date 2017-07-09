#include "common.h"
#include <glm/gtc/type_ptr.hpp>

namespace demo {

  int viewport[4] = {0,0,0,0};

  std::string text;
  bool stop;

  #define DEMO2

  #ifdef DEMO1
      //Base Subroutine Test Demo
      const char DEMO_NAME[] = "Subroutine Test";
      #include "demo0.inl"
  #endif // DEMO1

  #ifdef DEMO2
      //Advance Subroutine Material Test Demo
      const char DEMO_NAME[] = "Adv Subroutine Material Test";
      #include "demo1.inl"
  #endif // DEMO1


} //namespace demo
