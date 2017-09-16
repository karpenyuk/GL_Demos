#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "gl_helpers.h"
#include <GL/glut.h>
#include <GL/freeglut.h>

#include "common.h"

double FrameTime(0.0);
GLuint FrameCount(0u);
#ifdef BENCHMARK
  const int ITER_COUNTS = 100;
  const int PRIM_STEP = 100;
  std::ofstream res_file;
  int primCount = 1;
  int iterNum = 0;
  double avgTime = 0.0;
#endif // BENCHMARK


void keyboard(unsigned char key, int x, int y);
void display(void);
void initialize();
void update(int value);
void reshape(int w,int h);
void release(void);

int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitContextVersion (4, 4);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL);
  glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
  glutInitContextFlags (GLUT_FORWARD_COMPATIBLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(1200,900);
  glutCreateWindow("GLUT Test");
  glewInit();
  printf("OpenGL: %s\n", glGetString(GL_VERSION));
  glutKeyboardFunc(&keyboard);
  glutReshapeFunc(&reshape);
  glutDisplayFunc(&display);
  glutTimerFunc(16,&update,0);

  initialize();

  glutMainLoop();

  release();
  return EXIT_SUCCESS;
}

void initialize() {
  wglSwapIntervalEXT (0);
  printf("Initialization ");
  gl_helpers::Initialize();
  gl_helpers::CheckOpenGLError();
  printf("\n");
  printf("Demo name: %s\n",demo::DEMO_NAME);


  demo::initialize();
  demo::stop = false;

  #ifdef BENCHMARK
    std::string fn(demo::DEMO_NAME);
    fn.append(".txt");
    res_file.open(fn.c_str());
  #endif // BENCHMARK
}

void reshape(int w,int h)
{
  glViewport(0,0,w,h);
  demo::viewport[2] = w; demo::viewport[3] = h;
  printf("New viewport width = %d, height = %d\n", demo::viewport[2], demo::viewport[3]);
  FrameTime = 0.0;
  FrameCount = 0u;
  demo::reshape(w,h);
}


void update(int value) {
  char title[255];
  sprintf(title, "FrameTime=%4.2fms, FPS=%4.2f",FrameTime/1000.0 / double(FrameCount), double(FrameCount)/FrameTime*1000000.0);
	glutSetWindowTitle(title);
	demo::update(value);
  glutTimerFunc(62,update,0);
}

void release(void) {
  #ifdef BENCHMARK
    res_file.close();
  #endif
}

void keyboard(unsigned char key, int x, int y)
{
  switch (key)
  {
    case '\x1B':
      demo::release();
      printf("FrameTime=%4.2fms, FPS=%4.2f",FrameTime/1000.0 / double(FrameCount), double(FrameCount)/FrameTime*1000000.0);
      exit(EXIT_SUCCESS);
      break;
    case ' ':
      printf("OpenGL: %s\n", glGetString(GL_VERSION));
      printf("FrameTime=%4.2fms, FPS=%4.2f\n",FrameTime/1000.0 / double(FrameCount), double(FrameCount)/FrameTime*1000000.0);
      FrameTime = 0.0;
      FrameCount = 0u;
      break;
  }
  demo::keyboard(key, x,y);
}

void display()
{
  #ifdef BENCHMARK
    //
  #endif

  gl_helpers::StartTimer();

  demo::display();

  FrameTime+=gl_helpers::GetCommandTime();
  ++FrameCount;

  gl_helpers::CheckOpenGLError();
  #ifdef BENCHMARK
    if (!demo::stop)
      avgTime += FrameTime;
    FrameTime = 0.0;
    glFinish();
  #endif
  glutSwapBuffers ();
  glutPostRedisplay();
}
