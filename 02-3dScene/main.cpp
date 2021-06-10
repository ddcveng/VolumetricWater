/*
 * Source code for the NPGR019 lab practices. Copyright Martin Kahoun 2021.
 * Licensed under the zlib license, see LICENSE.txt in the root directory.
 */

#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "Camera.h"
#include "Geometry.h"
#include "Textures.h"

#include "shaders.h"

// ----------------------------------------------------------------------------
// GLM optional parameters:
// GLM_FORCE_LEFT_HANDED       - use the left handed coordinate system
// GLM_FORCE_XYZW_ONLY         - simplify vector types and use x, y, z, w only
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

// Enum class for storing default window parameters
enum class WindowParams : int
{
  Width = 800,
  Height = 600
};

// ----------------------------------------------------------------------------

// Near clip plane settings
float nearClipPlane = 0.01f;
// Far clip plane settings
float farClipPlane = 100.0f;

// Mouse movement
struct MouseStatus
{
  // Current position
  double x, y;
  // Previous position
  double prevX, prevY;

  // Updates the status - called once per frame
  void Update(double &moveX, double &moveY)
  {
    moveX = x - prevX;
    prevX = x;
    moveY = y - prevY;
    prevY = y;
  }
} mouseStatus = {0.0};

struct FrameBuffer
{
    GLuint handle;
    GLuint color;
    GLuint depth_stencil;
};

// ----------------------------------------------------------------------------

// Max buffer length
static const unsigned int MAX_BUFFER_LENGTH = 256;
// Main window handle
GLFWwindow *mainWindow = nullptr;

// Camera instance
Camera camera;

// Meshes
Mesh<Vertex_Pos_Tex>* quad = nullptr;
Mesh<Vertex_Pos_Tex>* pool = nullptr;
Mesh<Vertex_Pos_Tex>* cube = nullptr;

// Textures helper instance
Textures& textures(Textures::GetInstance());

// Texture we'll be using
GLuint waterNormal = 0;
GLuint waterDuDv = 0;
GLuint testTex = 0;
GLuint checkerTex = 0;

// Texture sampler to use
Sampler activeSampler = Sampler::Nearest;

// Framebuffers
FrameBuffer reflection = {0};
FrameBuffer refraction = {0};

// Vsync on?
bool vsync = true;

// ----------------------------------------------------------------------------
FrameBuffer createFramebuffer(int width, int height, bool depth_texture);

// Callback for handling GLFW errors
void errorCallback(int error, const char* description)
{
  printf("GLFW Error %i: %s\n", error, description);
}

// Callback for handling window resize events
void resizeCallback(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
  camera.SetProjection(45.0f, (float)width / (float)height, nearClipPlane, farClipPlane);
}

// Callback for handling mouse movement over the window - called when mouse movement is detected
void mouseMoveCallback(GLFWwindow* window, double x, double y)
{
  // Update the current position
  mouseStatus.x = x;
  mouseStatus.y = y;
}

// Keyboard callback for handling system switches
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  // Notify the window that user wants to exit the application
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // Enable/disable MSAA - note that it still uses the MSAA buffer
  if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
  {
    if (glIsEnabled(GL_MULTISAMPLE))
      glDisable(GL_MULTISAMPLE);
    else
      glEnable(GL_MULTISAMPLE);
  }

  // Enable/disable wireframe rendering
  if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
  {
    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    if (polygonMode[0] == GL_FILL)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  // Enable/disable backface culling
  if (key == GLFW_KEY_F3 && action == GLFW_PRESS)
  {
    if (glIsEnabled(GL_CULL_FACE))
      glDisable(GL_CULL_FACE);
    else
      glEnable(GL_CULL_FACE);
  }

  // Enable/disable depth test
  if (key == GLFW_KEY_F4 && action == GLFW_PRESS)
  {
    if (glIsEnabled(GL_DEPTH_TEST))
      glDisable(GL_DEPTH_TEST);
    else
      glEnable(GL_DEPTH_TEST);
  }

  // Enable/disable vsync
  if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
  {
    vsync = !vsync;
    if (vsync)
      glfwSwapInterval(1);
    else
      glfwSwapInterval(0);
  }
}

// Helper method for creating scene geometry
void createGeometry()
{
    //create a gridSize*gridSize grid of quad patches
    quad = Geometry::CreateQuadTex();
    pool = Geometry::CreatePoolTex();
    cube = Geometry::CreateCubeTex();
    
    // Prepare textures
    waterNormal = Textures::LoadTexture("data/waterNormal.png", false);
    waterDuDv = Textures::LoadTexture("data/waterDUDV.png", false);
    checkerTex = Textures::CreateCheckerBoardTexture(256, 16);
    testTex = Textures::LoadTexture("data/512-UV-test.png", false);
    
    textures.CreateSamplers();
}

// Helper method for OpenGL initialization
bool initOpenGL()
{
  // Set the GLFW error callback
  glfwSetErrorCallback(errorCallback);

  // Initialize the GLFW library
  if (!glfwInit()) return false;

  // Request OpenGL 3.3 core profile upon window creation
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create the window
  mainWindow = glfwCreateWindow((int)WindowParams::Width, (int)WindowParams::Height, "", nullptr, nullptr);
  if (mainWindow == nullptr)
  {
    printf("Failed to create the GLFW window!");
    return false;
  }

  // Make the created window with OpenGL context current for this thread
  glfwMakeContextCurrent(mainWindow);

  // Check that GLAD .dll loader and symbol imported is ready
  if (!gladLoadGL()) {
    printf("GLAD failed!\n");
    return false;
  }

  // Enable vsync
  if (vsync)
    glfwSwapInterval(1);
  else
    glfwSwapInterval(0);

  // Enable backface culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

  // Register a window resize callback
  glfwSetFramebufferSizeCallback(mainWindow, resizeCallback);

  // Register keyboard callback
  glfwSetKeyCallback(mainWindow, keyCallback);

  // Register mouse movement callback
  glfwSetCursorPosCallback(mainWindow, mouseMoveCallback);

  // Set the OpenGL viewport and camera projection
  resizeCallback(mainWindow, (int)WindowParams::Width, (int)WindowParams::Height);

  // Set the initial camera position and orientation
  camera.SetTransformation(glm::vec3(0.0f, 2.5f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

  reflection = createFramebuffer((int)WindowParams::Width, (int)WindowParams::Height, false);
  // need to sample depth buffer for fogginess
  refraction = createFramebuffer((int)WindowParams::Width, (int)WindowParams::Height, true);

  return true;
}

// Helper method for graceful shutdown
void shutDown()
{
    // Release shader programs
    for (int i = 0; i < ShaderProgram::NumShaderPrograms; ++i) {
        glDeleteProgram(shaderProgram[i]);
    }

    // Delete the pool mesh
    delete quad;
    quad = nullptr;
    delete pool;
    pool = nullptr;

    // Release the window
    glfwDestroyWindow(mainWindow);

    // Close the GLFW library
    glfwTerminate();
}

// Helper method for handling input events
void processInput(float dt)
{
  // Camera movement - keyboard events
  int direction = (int)MovementDirections::None;
  if (glfwGetKey(mainWindow, GLFW_KEY_W) == GLFW_PRESS)
    direction |= (int)MovementDirections::Forward;

  if (glfwGetKey(mainWindow, GLFW_KEY_S) == GLFW_PRESS)
    direction |= (int)MovementDirections::Backward;

  if (glfwGetKey(mainWindow, GLFW_KEY_A) == GLFW_PRESS)
    direction |= (int)MovementDirections::Left;

  if (glfwGetKey(mainWindow, GLFW_KEY_D) == GLFW_PRESS)
    direction |= (int)MovementDirections::Right;

  if (glfwGetKey(mainWindow, GLFW_KEY_R) == GLFW_PRESS)
    direction |= (int)MovementDirections::Up;

  if (glfwGetKey(mainWindow, GLFW_KEY_F) == GLFW_PRESS)
    direction |= (int)MovementDirections::Down;

  // Update the mouse status
  double dx, dy;
  mouseStatus.Update(dx, dy);

  // Camera orientation - mouse movement
  glm::vec2 mouseMove(0.0f, 0.0f);
  if (glfwGetMouseButton(mainWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
  {
    mouseMove.x = (float)(dx);
    mouseMove.y = (float)(dy);
  }

  // Update the camera movement
  camera.Move((MovementDirections)direction, mouseMove, dt);

  // Reset camera position/orientation
  if (glfwGetKey(mainWindow, GLFW_KEY_ENTER) == GLFW_PRESS)
    camera.SetTransformation(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

// should be fine but !!!
FrameBuffer createFramebuffer(int width, int height, bool depth_texture)
{
    // Bind the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint handle = 0;
    glGenFramebuffers(1, &handle);

    glBindFramebuffer(GL_FRAMEBUFFER, handle);

    // --------------------------------------------------------------------------
    // Render target texture:
    // --------------------------------------------------------------------------

    
    GLuint renderTarget = 0;

    glGenTextures(1, &renderTarget);

    glBindTexture(GL_TEXTURE_2D, renderTarget);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTarget, 0);

    GLuint depthStencil = 0;

    if (depth_texture)
    {
        glGenTextures(1, &depthStencil);
        glBindTexture(GL_TEXTURE_2D, depthStencil);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthStencil, 0);
    } 
    else {
        glGenRenderbuffers(1, &depthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);
    }

    // Set the list of draw buffers.
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    // Check for completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        printf("Failed to create framebuffer: 0x%04X\n", status);
    }

    // Bind back the window system provided framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return { handle, renderTarget, depthStencil };
}

void setupFramebuffer(GLuint fbo, bool useStencil = false)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glEnable(GL_CLIP_DISTANCE0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (useStencil) 
    {
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glDisable(GL_STENCIL_TEST);
    }
}

void renderScene(float dt)
{
    // first render everything under the water for refractions
    setupFramebuffer(refraction.handle, false);

    // draw ...

    // now render everything above the water for reflections
    setupFramebuffer(reflection.handle, false);
    // we need to move the camera down by 2 time the distance from the water to the camera
    // and invert the pitch

    // draw ...

    // now draw everything in the scene
    // plus water with reflection and refraction
    setupFramebuffer(0, true);

    // use stencil buffer to mask out the ground around where the pool should be

    // draw ...

    glBindVertexArray(0);
    glUseProgram(0);
}

// Helper method for implementing the application main loop
void mainLoop()
{
  static double prevTime = 0.0;
  while (!glfwWindowShouldClose(mainWindow))
  {
    // Calculate delta time
    double time = glfwGetTime();
    float dt = (float)(time - prevTime);
    prevTime = time;

    // Print it to the title bar
    static char title[MAX_BUFFER_LENGTH];
    snprintf(title, MAX_BUFFER_LENGTH, "dt = %.2fms, FPS = %.1f", dt * 1000.0f, 1.0f / dt);
    glfwSetWindowTitle(mainWindow, title);

    // Poll the events like keyboard, mouse, etc.
    glfwPollEvents();

    // Process keyboard input
    processInput(dt);

    // Render the scene
    renderScene(dt);

    // Swap actual buffers on the GPU
    glfwSwapBuffers(mainWindow);
  }
}

int main()
{
  // Initialize the OpenGL context and create a window
  if (!initOpenGL())
  {
    printf("Failed to initialize OpenGL!\n");
    shutDown();
    return -1;
  }

  // Compile shaders needed to run
  if (!compileShaders())
  {
    printf("Failed to compile shaders!\n");
    shutDown();
    return -1;
  }

  // Create the scene geometry
  createGeometry();

  // Enter the application main loop
  mainLoop();

  // Release used resources and exit
  shutDown();
  return 0;
}
