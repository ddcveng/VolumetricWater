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

// ----------------------------------------------------------------------------

// Max buffer length
static const unsigned int MAX_BUFFER_LENGTH = 256;
// Main window handle
GLFWwindow *mainWindow = nullptr;

// Camera instance
Camera camera;

// Meshes
Mesh<Vertex_Pos_Tex>* ground = nullptr;

// other variables
const int gridSize = 16;

// 1 - distance based tesselation and displacement
// 2 - no tesselation or displacement, this is what gets sent to the gpu
// 3 - max tesselation and displacement
int mode = 1;

// Textures helper instance
Textures& textures(Textures::GetInstance());

// Texture we'll be using
GLuint rocksHeight = 0;
GLuint rocksCol = 0;

// Texture sampler to use
Sampler activeSampler = Sampler::Nearest;

// Vsync on?
bool vsync = true;

// ----------------------------------------------------------------------------

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

  if (key == GLFW_KEY_1 && action == GLFW_PRESS)
  {
      mode = 1;
  }

  if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
      mode = 2;
  }

  if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
    mode = 3;
  }
}

// Helper method for creating scene geometry
void createGeometry()
{
    //create a gridSize*gridSize grid of quad patches
    ground = Geometry::CreateQuadGrid(gridSize);
    
    // Prepare textures
    rocksHeight = Textures::LoadTexture("data/rocks_ground_02_height_4k.jpg", false);
    rocksCol = Textures::LoadTexture("data/rocks_ground_02_col_4k.jpg", false);
    
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
    delete ground;
    ground = nullptr;


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
    setupFramebuffer(0, false);

    glUseProgram(shaderProgram[ShaderProgram::Tess]);

    // using 4 vertices per patch and quads tesslation
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    // send in the required uniforms
    const glm::vec4& cameraPos(camera.GetViewToWorld()[3]);
    const glm::mat4& wtv(camera.GetWorldToView());
    const glm::mat4& proj(camera.GetProjection());
    const glm::mat4 model = glm::identity<glm::mat4>();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(wtv));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform4fv(3, 1, glm::value_ptr(cameraPos));
    glUniform1i(4, mode);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rocksCol);
    glBindSampler(0, textures.GetSampler(activeSampler));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rocksHeight);
    glBindSampler(1, textures.GetSampler(activeSampler));

    // draw mode GL_PATCHES !!!
    glBindVertexArray(ground->GetVAO());
    glDrawElements(GL_PATCHES, ground->GetIBOSize(), GL_UNSIGNED_INT, reinterpret_cast<void*>(0));

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
