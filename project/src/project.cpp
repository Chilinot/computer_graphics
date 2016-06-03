// Private stuff
#include "utils.h"
#include "utils2.h"

// For debugging
#include <stdio.h>

// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <AntTweakBar.h>

// Misc
#include <iostream>
#include <cstdlib>
#include <algorithm>

// -- MACROS
#define GLM_FORCE_RADIANS

double lastTime;

enum CurrentSimulation {
  DEFAULT,
  TORNADO,
  FIRE,
  FOUNTAIN,
  EXPLOSION
};

// CPU representation of a particle
struct Particle{
  glm::vec3 pos, speed;
  unsigned char r,g,b,a; // Color
  float size, angle, weight;
  float life; // Remaining life of the particle. if < 0 : dead and unused.
  float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

  bool operator<(const Particle& that) const {
    // Sort in reverse order : far particles drawn first.
    return this->cameradistance > that.cameradistance;
  }
};

void colorParticleRed(Particle &p)
{
    p.r = 230;
    p.g = 110;
    p.b = 0;
}

void colorParticleYellow(Particle &p)
{
    p.r = 150;
    p.g = 110;
    p.b = 0;
}

void colorParticleGray(Particle &p)
{
    p.r = 100;
    p.g = 100;
    p.b = 100;
}

void colorParticleBlue(Particle &p)
{
    p.r = 53;
    p.g = 202;
    p.b = 239;
}

const int maxParticles = 100000;
Particle particlesContainer[maxParticles];

void sortParticles()
{
  std::sort(&particlesContainer[0], &particlesContainer[maxParticles]);
}

int lastUsedParticle = 0;
int findUnusedParticle(){

  for(int i=lastUsedParticle; i<maxParticles; i++){
    if (particlesContainer[i].life < 0){
      lastUsedParticle = i;
      return i;
    }
  }

  for(int i=0; i<lastUsedParticle; i++){
    if (particlesContainer[i].life < 0){
      lastUsedParticle = i;
      return i;
    }
  }

  return 0; // All particles are taken, override the first one
}

static GLfloat* g_particule_position_size_data = new GLfloat[maxParticles * 4];
static GLubyte* g_particule_color_data         = new GLubyte[maxParticles * 4];

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
  POSITION = 0
};

// Struct for resources
struct Context {
  int width;
  int height;
  float aspect;
  float fov;

  GLFWwindow *window;
  Trackball trackball;

  GLuint triangleProgram;
  GLuint triangleVAO;

  GLuint defaultVAO;

  GLuint particleVAO;
  GLuint particleProgram;
  GLuint billboard_vertex_buffer, particles_position_buffer, particles_color_buffer;
  GLuint texture;

  glm::vec3 camera_direction;


  // Simulation settings
  float gravity;
  glm::vec3 spawn_direction;
  float spread;
  glm::vec3 spawn_position;

  // Pre-set simulations
  CurrentSimulation current_simulation;

  bool simulate_fountain;
  bool simulate_tornado;
  bool simulate_fire;
  bool simulate_explosion;

  double last_explosion;
  float explosion_delay;

  bool wind_enabled;
  glm::vec3 wind_vector;
};

GLuint createTriangleVAO()
{
  const GLfloat vertices[] = {
    0.0f, 0.5f, 0.0f,
    -0.5f,-0.5f, 0.0f,
    0.5f,-0.5f, 0.0f,
  };

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glEnableVertexAttribArray(POSITION);
  glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  return vao;
}

GLuint createParticleVAO(Context &ctx)
{
  // The VBO containing the 4 vertices of the particles.
  // Thanks to instancing, they will be shared by all particles.
  static const GLfloat g_vertex_buffer_data[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f, 0.5f, 0.0f,
    0.5f, 0.5f, 0.0f,
  };

  glGenBuffers(1, &ctx.billboard_vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.billboard_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

  // The VBO containing the positions and sizes of the particles
  glGenBuffers(1, &ctx.particles_position_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_position_buffer);
  // Initialize with empty (NULL) buffer : it will be updated later, each frame.
  glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

  // The VBO containing the colors of the particles
  glGenBuffers(1, &ctx.particles_color_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_color_buffer);
  // Initialize with empty (NULL) buffer : it will be updated later, each frame.
  glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

  // Generate VAO to store attributes
  glGenVertexArrays(1, &ctx.particleVAO);
  glBindVertexArray(ctx.particleVAO);

  // vertices
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.billboard_vertex_buffer);
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  // centers
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_position_buffer);
  glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

  // colors
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_color_buffer);
  glVertexAttribPointer( 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

  // Re-bind default VAO to protect this from changes
  glBindVertexArray(ctx.defaultVAO);
}

void initializeTrackball(Context &ctx)
{
    double radius = double(std::min(ctx.width, ctx.height)) / 2.0;
    ctx.trackball.radius = radius;
    glm::vec2 center = glm::vec2(ctx.width, ctx.height) / 2.0f;
    ctx.trackball.center = center;
}

void init(Context &ctx)
{
  // Simulation settings
  ctx.gravity = -9.81f;
  ctx.spawn_direction = glm::vec3(0.0f, 10.0f, 0.0f);
  ctx.spread = 1.5f;
  ctx.spawn_position = glm::vec3(0.0f, 0.0f, 0.0f);

  ctx.current_simulation = FOUNTAIN;
  ctx.simulate_fountain = true;
  ctx.simulate_tornado = false;
  ctx.simulate_fire = false;
  ctx.simulate_explosion = false;

  ctx.last_explosion = glfwGetTime();
  ctx.explosion_delay = 2.0f;

  ctx.wind_enabled = false;
  ctx.wind_vector = glm::vec3(0.02f, 0.0f, 0.0f);

  // Set FOV to 90-degrees
  ctx.fov = 3.14159/2;

  // Point camera towards the particle source
  ctx.camera_direction = glm::vec3(0.0f, 0.0f, 20.0f);

  ctx.particleProgram = loadShaderProgram(shaderDir() + "particle.vert",
      shaderDir() + "particle.frag");

  lastTime = glfwGetTime();

  ctx.texture = load2DTexture((resourceDir() + "lightsabericonblue.png").c_str());

  for(int i=0; i<maxParticles; i++){
    particlesContainer[i].life = -1.0f;
    particlesContainer[i].cameradistance = -1.0f;
  }

  createParticleVAO(ctx);
  initializeTrackball(ctx);
}

int simulateParticles(Context &ctx, double delta, glm::vec3 cameraPosition)
{
  static int horizontal_ticker = 0;

  horizontal_ticker += 1;
  horizontal_ticker = horizontal_ticker % 360;

  int particlesCount = 0;

  for(int i = 0; i < maxParticles; i++){

    Particle& p = particlesContainer[i];

    if(p.life > 0.0f){

      // Decrease life
      p.life -= delta;
      if (p.life > 0.0f){

        if(ctx.simulate_tornado) {
          static int radius = 50;
          if(ctx.current_simulation != TORNADO) {
            ctx.spawn_direction = glm::vec3(0.0f, 0.0f, 0.0f);
            ctx.gravity = 140.0f;
            ctx.spread = 1.6f;

            ctx.current_simulation = TORNADO;
          }
          p.speed = glm::vec3(radius * cos(degreeToRadians(horizontal_ticker)), ctx.gravity, radius * sin(degreeToRadians(horizontal_ticker))) * (float) delta;
        }
        else if(ctx.simulate_fire) {
          if(ctx.current_simulation != FIRE) {
            ctx.spawn_direction = glm::vec3(0.0f, 0.5f, 0.0f);
            ctx.gravity = 1.5f;
            ctx.spread = 1.6f;

            ctx.current_simulation = FIRE;
          }

          p.speed += glm::vec3(0.0f, ctx.gravity, 0.0f) * (float) delta;

          if(p.life < 3.3f) {
            colorParticleGray(p);
          }
          else if(p.life < 4.0f) {
            colorParticleYellow(p);
          }
          else if(p.life < 5.0f) {
            colorParticleRed(p);
          }
        }
        else if(ctx.simulate_fountain) {
          if(ctx.current_simulation != FOUNTAIN) {
            ctx.gravity = -9.81f;
            ctx.spawn_direction = glm::vec3(0.0f, 10.0f, 0.0f);
            ctx.spread = 1.5f;

            ctx.current_simulation = FOUNTAIN;
          }

          colorParticleBlue(p);
          p.speed += glm::vec3(0.0f, ctx.gravity, 0.0f) * (float) delta * 0.5f;
        }
        else if(ctx.simulate_explosion) {
          if(ctx.current_simulation != EXPLOSION) {
            ctx.spread = 30.0f;
            ctx.gravity = 0.0f;

            ctx.current_simulation = EXPLOSION;
          }

          // Color particles similar to fire simulation
          if(p.life < 4.0f) {
            colorParticleGray(p);
          }
          else if(p.life < 4.5f) {
            colorParticleYellow(p);
          }
          else if(p.life < 5.0f) {
            colorParticleRed(p);
          }

          p.speed += glm::vec3(0.0f, ctx.gravity, 0.0f) * (float) delta * 0.5f;
        }
        else {
          if(ctx.current_simulation != DEFAULT) {
            ctx.gravity = -9.81f;
            ctx.spawn_direction = glm::vec3(0.0f, 10.0f, 0.0f);
            ctx.spread = 1.5f;
            ctx.spawn_position = glm::vec3(0.0f, 0.0f, 0.0f);

            ctx.current_simulation = DEFAULT;
          }

          colorParticleGray(p);
          p.speed += glm::vec3(0.0f, ctx.gravity, 0.0f) * (float) delta * 0.5f;
        }

        if(ctx.wind_enabled) {
          //if(rand() % 1) {
          //  p.speed += glm::vec3(cos(glfwGetTime()) * 0.01f, 0.0f, cos(glfwGetTime()) * 0.01f);
          //}
          //else {
          //  p.speed += glm::vec3(sin(glfwGetTime()) * 0.01f, 0.0f, sin(glfwGetTime()) * 0.01f);
          //}
          p.speed += ctx.wind_vector;
        }

        p.pos += p.speed * (float)delta;
        p.cameradistance = glm::length2( p.pos - cameraPosition );

        // Fill the GPU buffer
        g_particule_position_size_data[4*particlesCount+0] = p.pos.x;
        g_particule_position_size_data[4*particlesCount+1] = p.pos.y;
        g_particule_position_size_data[4*particlesCount+2] = p.pos.z;

        g_particule_position_size_data[4*particlesCount+3] = p.size;

        g_particule_color_data[4*particlesCount+0] = p.r;
        g_particule_color_data[4*particlesCount+1] = p.g;
        g_particule_color_data[4*particlesCount+2] = p.b;
        g_particule_color_data[4*particlesCount+3] = p.a;

      }else{
        // Particles that just died will be put at the end of the buffer in sortParticles();
        p.cameradistance = -1.0f;
      }

      particlesCount++;
    }
  }

  return particlesCount;
}

void drawParticles(Context &ctx)
{
  glBindVertexArray(ctx.particleVAO);
  glUseProgram(ctx.particleProgram);

  double currentTime = glfwGetTime();
  double delta = currentTime - lastTime;
  lastTime = currentTime;

  // -- Construct matrices
  glm::mat4 view = glm::lookAt(ctx.camera_direction, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 projection = glm::perspective(ctx.fov, ctx.aspect, 0.1f, 100.0f);
  glm::mat4 viewProjection = projection * view;

  glm::vec3 cameraPosition(glm::inverse(view)[3]);

  // -- Create some new particles
  int newparticles = (int)(delta*10000.0);
  if (newparticles > (int)(0.016f*10000.0))
    newparticles = (int)(0.016f*10000.0);

  if(ctx.current_simulation != EXPLOSION || (glfwGetTime() - ctx.last_explosion) > ctx.explosion_delay) {
    for(int i=0; i<newparticles; i++){

      int particleIndex = findUnusedParticle();

      particlesContainer[particleIndex].life = 5.0f; // This particle will live 5 seconds.
      particlesContainer[particleIndex].pos = ctx.spawn_position;

      // Add some random offset to each position
      particlesContainer[particleIndex].pos += glm::vec3((rand()/(double)(RAND_MAX + 1)), (rand()/(double)(RAND_MAX + 1)), (rand()/(double)(RAND_MAX + 1)));;

      glm::vec3 randomdir = glm::vec3(
          (rand()%2000 - 1000.0f)/1000.0f,
          (rand()%2000 - 1000.0f)/1000.0f,
          (rand()%2000 - 1000.0f)/1000.0f
          );

      particlesContainer[particleIndex].speed = ctx.spawn_direction + randomdir * ctx.spread;

      // Random colors
      //particlesContainer[particleIndex].r = rand() % 256;
      //particlesContainer[particleIndex].g = rand() % 256;
      //particlesContainer[particleIndex].b = rand() % 256;

      colorParticleGray(particlesContainer[particleIndex]);

      particlesContainer[particleIndex].a = (rand() % 256) / 3;

      particlesContainer[particleIndex].size = (rand()%1000)/2000.0f + 0.1f;
    }

    if(ctx.current_simulation == EXPLOSION) {
      ctx.last_explosion = glfwGetTime();
    }
  }

  // -- Simulate all particles
  int ParticlesCount = simulateParticles(ctx, delta, cameraPosition);

  // Sort particles to ensure correct blending
  sortParticles();

  // Update buffers
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_position_buffer);
  glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
  glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_color_buffer);
  glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
  glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);

  // Set blending options
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // -- Pass uniforms

  // Bind our texture in Texture Unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx.texture);
  // Tell fragment shader to use texture unit 0
  glUniform1i(glGetUniformLocation(ctx.particleProgram, "myTextureSampler"), 0);

  glUniform3f(glGetUniformLocation(ctx.particleProgram, "CameraRight_worldspace"), view[0][0], view[1][0], view[2][0]);
  glUniform3f(glGetUniformLocation(ctx.particleProgram, "CameraUp_worldspace")   , view[0][1], view[1][1], view[2][1]);

  glUniformMatrix4fv(glGetUniformLocation(ctx.particleProgram, "VP"), 1, GL_FALSE, &viewProjection[0][0]);

  // -- Rendering time

  glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
  glVertexAttribDivisor(1, 1); // positions : one per quad (its center) -> 1
  glVertexAttribDivisor(2, 1); // color : one per quad -> 1

  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

  // Reset to defaults
  glBindVertexArray(ctx.defaultVAO);
  glUseProgram(0);
}

void display(Context &ctx)
{
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  drawParticles(ctx);
}

void reloadShaders(Context *ctx)
{
  glDeleteProgram(ctx->triangleProgram);
  ctx->triangleProgram = loadShaderProgram(shaderDir() + "triangle.vert",
      shaderDir() + "triangle.frag");
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));

  if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    reloadShaders(ctx);
  }
}

void mouseButtonPressed(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        trackballStartTracking(ctx->trackball, glm::vec2(x, y));
    }
}

void mouseButtonReleased(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        trackballStopTracking(ctx->trackball);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (TwEventMouseButtonGLFW3(window, button, action, mods)) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        mouseButtonPressed(ctx, button, x, y);
    }
    else {
        mouseButtonReleased(ctx, button, x, y);
    }
}

void moveTrackball(Context *ctx, int x, int y)
{
    if (ctx->trackball.tracking) {
        trackballMove(ctx->trackball, glm::vec2(x, y));
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
    if (TwEventCursorPosGLFW3(window, x, y)) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    moveTrackball(ctx, x, y);
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
  TwWindowSize(width, height);

  Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
  ctx->width = width;
  ctx->height = height;
  ctx->aspect = float(width) / float(height);
  ctx->trackball.radius = double(std::min(width, height)) / 2.0;
  ctx->trackball.center = glm::vec2(width, height) / 2.0f;
  glViewport(0, 0, width, height);
}

int main(int argc, char** argv)
{
  Context ctx;

  // Create a GLFW window
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  ctx.width = 500;
  ctx.height = 500;
  ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Computer Graphics Project", nullptr, nullptr);
  glfwMakeContextCurrent(ctx.window);

  glfwSetWindowUserPointer(ctx.window, &ctx);
  glfwSetFramebufferSizeCallback(ctx.window, resizeCallback);
  glfwSetKeyCallback(ctx.window, keyCallback);
  glfwSetMouseButtonCallback(ctx.window, mouseButtonCallback);
  glfwSetCursorPosCallback(ctx.window, cursorPosCallback);
  glfwSetFramebufferSizeCallback(ctx.window, resizeCallback);

  // Load OpenGL functions
  glewExperimental = true;
  GLenum status = glewInit();
  if (status != GLEW_OK) {
    std::cerr << "Error: " << glewGetErrorString(status) << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Initialize rendering
  glGenVertexArrays(1, &ctx.defaultVAO);
  glBindVertexArray(ctx.defaultVAO);
  init(ctx);

  // TWEAKBAR
  TwInit(TW_OPENGL_CORE, nullptr);
  TwWindowSize(ctx.width, ctx.height);
  TwBar *tweakbar = TwNewBar("Settings");

  TwAddVarRW(tweakbar, "Eye Direction", TW_TYPE_DIR3F, &ctx.camera_direction, "");

  // Simulation settings
  TwAddSeparator(tweakbar, NULL, "");
  TwAddVarRW(tweakbar, "Gravity", TW_TYPE_FLOAT, &ctx.gravity, "step=0.1");
  TwAddVarRW(tweakbar, "Spawn Direction", TW_TYPE_DIR3F, &ctx.spawn_direction, "");
  TwAddVarRW(tweakbar, "Spread", TW_TYPE_FLOAT, &ctx.spread, "step=0.1");
  TwAddVarRW(tweakbar, "Spawn Position", TW_TYPE_DIR3F, &ctx.spawn_position, "");

  // Pre-set simulations
  TwAddSeparator(tweakbar, NULL, "");
  TwAddVarRW(tweakbar, "Tornado",  TW_TYPE_BOOLCPP, &ctx.simulate_tornado, "");
  TwAddVarRW(tweakbar, "Fire",  TW_TYPE_BOOLCPP, &ctx.simulate_fire, "");
  TwAddVarRW(tweakbar, "Fountain",  TW_TYPE_BOOLCPP, &ctx.simulate_fountain, "");
  TwAddVarRW(tweakbar, "Explosion",  TW_TYPE_BOOLCPP, &ctx.simulate_explosion, "");

  TwAddSeparator(tweakbar, NULL, "");
  TwAddVarRW(tweakbar, "Explosion delay", TW_TYPE_FLOAT, &ctx.explosion_delay, "step=0.1 min=0.0");
  TwAddVarRW(tweakbar, "Enable wind",  TW_TYPE_BOOLCPP, &ctx.wind_enabled, "");
  TwAddVarRW(tweakbar, "Wind direction", TW_TYPE_DIR3F, &ctx.wind_vector, "");

  // Start rendering loop
  while (!glfwWindowShouldClose(ctx.window)) {
    glfwPollEvents();
    display(ctx);
    TwDraw();
    glfwSwapBuffers(ctx.window);
  }

  // Shutdown
  TwTerminate();
  glfwDestroyWindow(ctx.window);
  glfwTerminate();
  std::exit(EXIT_SUCCESS);
}
