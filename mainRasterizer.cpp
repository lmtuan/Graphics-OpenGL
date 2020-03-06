////////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief Contains main function to create a window and run engine that
///        repeatedly generates a framebuffer and displays it.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes

// STL
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>

// GL
#define GL_GLEXT_PROTOTYPES
#if   defined(OSX)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <GLUT/glut.h>
#elif defined(LINUX)
#include <GL/glut.h>
#endif
#include <GL/freeglut.h>

// GLM
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>

#include "CompileShaders.h"
#include "RasterizableObject.h"
#include "ObjFileParser.h"

using glm::vec2, glm::vec3, glm::vec4, glm::mat4;

////////////////////////////////////////////////////////////////////////////////
// Global variables - avoid these

// Window
int g_width{1280};
int g_height{720};
int g_window{0};
GLuint g_program{0};

// Frame rate
const unsigned int FPS = 60;
float g_frameRate{0.f};
std::chrono::high_resolution_clock::time_point g_frameTime{
  std::chrono::high_resolution_clock::now()};
float g_delay{0.f};
float g_framesPerSecond{0.f};

// Antialiasing
bool g_antiAliasing = false;

// Scene
vec3 eye, at, up, right;
mat4 projMatrix;

std::unique_ptr<RasterizableObject> g_obj;
std::unique_ptr<RasterizableObject> g_obj2;

////////////////////////////////////////////////////////////////////////////////
// Functions

////////////////////////////////////////////////////////////////////////////////
/// @brief Initialize GL settings
void
initialize() {
  using glm::translate, glm::scale, glm::transpose, glm::inverse, glm::cross;

  glClearColor(0.f, 0.f, 0.0f, 0.f);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);
  if (g_antiAliasing) {
    glEnable(GL_MULTISAMPLE_ARB);
  } else {
    glDisable(GL_MULTISAMPLE_ARB);
  }

  g_program = compileProgram("shaders/phong.vert",
                             "shaders/phong.frag");
  // g_program = compileProgram("shaders/gouraud.vert",
                             // "shaders/gouraud.frag");
  glUseProgram(g_program);

  //////////////////////////
  // Scene

  // Set up camera
  eye = {0.f, 0.f, 0.f};
  at = {0.f, 0.f, -1.f};
  up = {0.f, 1.f, 0.f};
  right = cross(at, up);
  glUniform3f(glGetUniformLocation(g_program, "cameraPos"), eye.x, eye.y, eye.z);
  mat4 viewMatrix = glm::lookAt(eye, eye + at, up);
  projMatrix = glm::perspective(glm::radians(60.f), g_width/((float)g_height), 0.1f, 100.f);
  glUniformMatrix4fv(
      glGetUniformLocation(g_program, "viewProjectionMatrix"), 
      1, GL_FALSE, glm::value_ptr(projMatrix * viewMatrix));

  // Set up light
  glUniform1i(glGetUniformLocation(g_program, "numLights"), 2);
  glUniform3f(glGetUniformLocation(g_program, "lights[0].pos"), 2, 1, -10);
  glUniform3f(glGetUniformLocation(g_program, "lights[0].ia"), 0.1, 0.1, 0.1);
  glUniform3f(glGetUniformLocation(g_program, "lights[0].id"), 1.0, 1.0, 1.0);
  glUniform3f(glGetUniformLocation(g_program, "lights[0].is"), 1.0, 1.0, 1.0);
  glUniform3f(glGetUniformLocation(g_program, "lights[1].pos"), -3, 5, -7);
  glUniform3f(glGetUniformLocation(g_program, "lights[1].ia"), 0.1, 0.1, 0.1);
  glUniform3f(glGetUniformLocation(g_program, "lights[1].id"), 1.0, 1.0, 1.0);
  glUniform3f(glGetUniformLocation(g_program, "lights[1].is"), 1.0, 1.0, 1.0);

  // Objects
  Mesh mesh = parseObjFile("models/sphere.obj");

  MaterialUniformLocation materialLoc {
    glGetUniformLocation(g_program, "material.ka"),
    glGetUniformLocation(g_program, "material.kd"),
    glGetUniformLocation(g_program, "material.ks"),
    glGetUniformLocation(g_program, "material.shininess")
  };

  GLint vModelToWorldLoc = glGetUniformLocation(g_program, "vertexModelMatrix");
  GLint nModelToWorldLoc = glGetUniformLocation(g_program, "normalModelMatrix");

  Material mat {
    {0.5, 0.5, 0.5}, {1, 0, 0}, {0.8, 0.8, 0.8}, 100
  };
  Material mat2 {
    {0.5, 0.5, 0.5}, {0, 1, 0}, {0.5, 0.5, 0.5}, 50
  };

  mat4 vModel2World = scale(translate(mat4(1.f), {1.5f, 0.f, -15.f}), {0.1f, 0.1f, 0.1f});
  mat4 nModel2World = transpose(inverse(vModel2World));
  mat4 vModel2World2 = scale(translate(mat4(1.f), {-1.5f, 0.f, -10.f}), {0.1f, 0.1f, 0.1f});
  mat4 nModel2World2 = transpose(inverse(vModel2World));
  
  g_obj = std::make_unique<RasterizableObject>(mesh, mat, vModel2World, nModel2World);
  g_obj->setMaterialUniformLocation(materialLoc);
  g_obj->setVertexModelMatrixUniformLocation(vModelToWorldLoc);
  g_obj->setNormalModelMatrixUniformLocation(nModelToWorldLoc);

  g_obj2 = std::make_unique<RasterizableObject>(mesh, mat2, vModel2World2, nModel2World2);
  g_obj2->setMaterialUniformLocation(materialLoc);
  g_obj2->setVertexModelMatrixUniformLocation(vModelToWorldLoc);
  g_obj2->setNormalModelMatrixUniformLocation(nModelToWorldLoc);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Callback for resize of window
///
/// Responsible for setting window size (viewport) and projection matrix.
void
resize(GLint _w, GLint _h) {
  g_width = _w;
  g_height = _h;

  // Viewport
  glViewport(0, 0, g_width, g_height);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Timer function to fix framerate in a GLUT application
/// @param _v Value (not used here)
///
/// Note, this is rudametary and fragile.
void
timer(int _v) {
  if(g_window != 0) {
    glutPostRedisplay();

    g_delay = std::max(0.f, 1.f/FPS - g_frameRate);
    glutTimerFunc((unsigned int)(1000.f*g_delay), timer, 0);
  }
  else
    exit(0);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Draw function for single frame
void
draw() {
  using namespace std::chrono;

  //////////////////////////////////////////////////////////////////////////////
  // Clear
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up camera
  glUniform3f(glGetUniformLocation(g_program, "cameraPos"), eye.x, eye.y, eye.z);
  mat4 viewMatrix = glm::lookAt(eye, eye + at, up);
  glUniformMatrix4fv(
      glGetUniformLocation(g_program, "viewProjectionMatrix"), 
      1, GL_FALSE, glm::value_ptr(projMatrix * viewMatrix));

  //////////////////////////////////////////////////////////////////////////////
  // Draw
  g_obj->draw();
  g_obj2->draw();

  //////////////////////////////////////////////////////////////////////////////
  // Show
  glutSwapBuffers();

  //////////////////////////////////////////////////////////////////////////////
  // Record frame time
  high_resolution_clock::time_point time = high_resolution_clock::now();
  g_frameRate = duration_cast<duration<float>>(time - g_frameTime).count();
  g_frameTime = time;
  g_framesPerSecond = 1.f/(g_delay + g_frameRate);
  // printf("FPS: %6.2f\n", g_framesPerSecond);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Callback function for keyboard presses
/// @param _key Key
/// @param _x X position of mouse
/// @param _y Y position of mouse
void
keyPressed(GLubyte _key, GLint _x, GLint _y) {
  switch(_key) {
    // Escape key : quit application
    case 27:
      std::cout << "Destroying window: " << g_window << std::endl;
      glutDestroyWindow(g_window);
      g_window = 0;
      break;
    case 'a':
      g_antiAliasing = !g_antiAliasing;
      std::cout << "Anti-aliasing: " << g_antiAliasing << std::endl;
      if (g_antiAliasing) {
        glEnable(GL_MULTISAMPLE_ARB);
        glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
      } else {
        glDisable(GL_MULTISAMPLE_ARB);
      }
      glutPostRedisplay();
      break;
    // Unhandled
    default:
      std::cout << "Unhandled key: " << (int)(_key) << std::endl;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Callback function for keyboard presses of special keys
/// @param _key Key
/// @param _x X position of mouse
/// @param _y Y position of mouse
void
specialKeyPressed(GLint _key, GLint _x, GLint _y) {
  switch(_key) {
    // Arrow keys
    case GLUT_KEY_LEFT:
    case GLUT_KEY_RIGHT:
      break;
    // Unhandled
    default:
      std::cout << "Unhandled special key: " << _key << std::endl;
      break;
  }
}

const int GLUT_NO_BUTTON = -1;
const int GLUT_WHEEL_UP = 3;
const int GLUT_WHEEL_DOWN = 4;

// only 1 mouse button can be active, other will be ignored
int g_activeDragButton = GLUT_NO_BUTTON;
int g_mouseStartX;
int g_mouseStartY;
vec3 g_eyeStart;
float g_scale = 0.02f;
float g_zScale = 0.2f;

vec3 g_atStart, g_upStart, g_rightStart;
float g_angleDelta = 0.005f;

void 
mousePressed(int button, int state, int x, int y) {
  std::cout << "Button " << button << " active " << g_activeDragButton << std::endl;
  if (button == g_activeDragButton) {
    // release the active button
    g_activeDragButton = GLUT_NO_BUTTON;
    return;
  } else if (g_activeDragButton != GLUT_NO_BUTTON) {
    // ignore other button if there's already an active button
    return;
  }
  switch(button) {
    case GLUT_LEFT_BUTTON:
      g_activeDragButton = GLUT_LEFT_BUTTON;
      g_mouseStartX = x;
      g_mouseStartY = y;
      g_eyeStart = eye;
      break;
    case GLUT_RIGHT_BUTTON:
      g_activeDragButton = GLUT_RIGHT_BUTTON;
      g_mouseStartX = x;
      g_mouseStartY = y;
      g_atStart = at;
      g_upStart = up;
      g_rightStart = right;
      break;
    case GLUT_WHEEL_UP:
      eye += at * g_zScale;
      break;
    case GLUT_WHEEL_DOWN:
      eye -= at * g_zScale;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Callback function for mouse actions
void
mouseDragged(int x, int y) {
  if(g_activeDragButton == GLUT_LEFT_BUTTON) {
    float dx = x - g_mouseStartX;
    float dy = y - g_mouseStartY;
    // change camera position
    eye = g_eyeStart + (right * -dx + up * dy) * g_scale;
  } else if(g_activeDragButton == GLUT_RIGHT_BUTTON) {
    float dx = x - g_mouseStartX;
    float dy = g_mouseStartY - y;
    vec2 delta(-dy, dx);
    float d = glm::length(delta);
    delta = glm::normalize(delta);
    
    std::cout << dx << " " << dy << " "<< d << std::endl;


    vec3 rotationAxis = g_rightStart * delta.x + g_upStart * delta.y;
    float rotationAngle = d * g_angleDelta;
    mat4 rotationMatrix = glm::rotate(mat4(1.f), rotationAngle, rotationAxis);
    at = rotationMatrix * vec4(g_atStart, 1.f);
    up = rotationMatrix * vec4(g_upStart, 1.f);
    right = glm::cross(at, up);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Main

////////////////////////////////////////////////////////////////////////////////
/// @brief main
/// @param _argc Count of command line arguments
/// @param _argv Command line arguments
/// @return Application success status
int
main(int _argc, char** _argv) {
  //////////////////////////////////////////////////////////////////////////////
  // Initialize GLUT Window
  std::cout << "Initializing GLUTWindow" << std::endl;
  // GLUT
  glutInit(&_argc, _argv);
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  glutSetOption(GLUT_MULTISAMPLE, 8);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
  glutInitWindowPosition(50, 100);
  glutInitWindowSize(g_width, g_height); // HD size
  g_window = glutCreateWindow("Spiderling: A Rudamentary Game Engine");

  // GL
  initialize();

  //////////////////////////////////////////////////////////////////////////////
  // Assign callback functions
  std::cout << "Assigning Callback functions" << std::endl;
  glutReshapeFunc(resize);
  glutDisplayFunc(draw);
  glutKeyboardFunc(keyPressed);
  glutSpecialFunc(specialKeyPressed);
  glutMouseFunc(mousePressed);
  glutMotionFunc(mouseDragged);
  glutPassiveMotionFunc(NULL);
  glutTimerFunc(1000/FPS, timer, 0);

  // Start application
  std::cout << "Starting Application" << std::endl;
  glutMainLoop();

  return 0;
}

#if   defined(OSX)
#pragma clang diagnostic pop
#endif
