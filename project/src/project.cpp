// Assignment 1, Part 1

#include "utils.h"
#include "utils2.h"

#include <stdio.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdlib>

#include <texture.hpp>

#define GLM_FORCE_RADIANS

double lastTime;

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

const int MaxParticles = 100000;
Particle ParticlesContainer[MaxParticles];

void SortParticles(){
	std::sort(&ParticlesContainer[0], &ParticlesContainer[MaxParticles]);
}

int LastUsedParticle = 0;

// Finds a Particle in ParticlesContainer which isn't used yet.
// (i.e. life < 0);
int FindUnusedParticle(){

  for(int i=LastUsedParticle; i<MaxParticles; i++){
    if (ParticlesContainer[i].life < 0){
      LastUsedParticle = i;
      return i;
    }
  }

  for(int i=0; i<LastUsedParticle; i++){
    if (ParticlesContainer[i].life < 0){
      LastUsedParticle = i;
      return i;
    }
  }

  return 0; // All particles are taken, override the first one
}

static GLfloat* g_particule_position_size_data = new GLfloat[MaxParticles * 4];
static GLubyte* g_particule_color_data         = new GLubyte[MaxParticles * 4];


// The attribute locations we will use in the vertex shader
enum AttributeLocation {
  POSITION = 0
};

// Struct for resources
struct Context {
  int width;
  int height;
  GLFWwindow *window;
  Trackball trackball;

  GLuint triangleProgram;
  GLuint triangleVAO;

  GLuint defaultVAO;

  GLuint particleVAO;
  GLuint particleProgram;
  GLuint billboard_vertex_buffer, particles_position_buffer, particles_color_buffer;
  GLuint texture;
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
  // === -------- ===

  // The VBO containing the 4 vertices of the particles.
  // Thanks to instancing, they will be shared by all particles.
  static const GLfloat g_vertex_buffer_data[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f, 0.5f, 0.0f,
    0.5f, 0.5f, 0.0f,
  };
  //GLuint billboard_vertex_buffer;
  glGenBuffers(1, &ctx.billboard_vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.billboard_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

  // The VBO containing the positions and sizes of the particles
  //GLuint particles_position_buffer;
  glGenBuffers(1, &ctx.particles_position_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_position_buffer);
  // Initialize with empty (NULL) buffer : it will be updated later, each frame.
  glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

  // The VBO containing the colors of the particles
  //GLuint particles_color_buffer;
  glGenBuffers(1, &ctx.particles_color_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_color_buffer);
  // Initialize with empty (NULL) buffer : it will be updated later, each frame.
  glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

  // === -------- ===
  glGenVertexArrays(1, &ctx.particleVAO);
  glBindVertexArray(ctx.particleVAO);

  // 1rst attribute buffer : vertices
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.billboard_vertex_buffer);
  glVertexAttribPointer(
      0, // attribute. No particular reason for 0, but must match the layout in the shader.
      3, // size
      GL_FLOAT, // type
      GL_FALSE, // normalized?
      0, // stride
      (void*)0 // array buffer offset
      );

  // 2nd attribute buffer : positions of particles' centers
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_position_buffer);
  glVertexAttribPointer(
      1, // attribute. No particular reason for 1, but must match the layout in the shader.
      4, // size : x + y + z + size => 4
      GL_FLOAT, // type
      GL_FALSE, // normalized?
      0, // stride
      (void*)0 // array buffer offset
      );

  // 3rd attribute buffer : particles' colors
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_color_buffer);
  glVertexAttribPointer(
      2, // attribute. No particular reason for 1, but must match the layout in the shader.
      4, // size : r + g + b + a => 4
      GL_UNSIGNED_BYTE, // type
      GL_TRUE, // normalized? *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
      0, // stride
      (void*)0 // array buffer offset
      );

  // === -------- ===

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
  //ctx.triangleProgram = loadShaderProgram(shaderDir() + "triangle.vert",
  //    shaderDir() + "triangle.frag");

  //ctx.triangleVAO = createTriangleVAO();

  ctx.particleProgram = loadShaderProgram(shaderDir() + "particle.vert",
      shaderDir() + "particle.frag");

  GLuint programID = ctx.particleProgram;

  //createParticleVAO(ctx);

  initializeTrackball(ctx);

  //for(int i=0; i<MaxParticles; i++){
  //  ParticlesContainer[i].life = -1.0f;
  //  ParticlesContainer[i].cameradistance = -1.0f;
  //}

  //lastTime = glfwGetTime();

  //ctx.texture = loadDDS((resourceDir() + "particle.DDS").c_str());

  // Vertex shader
  GLuint CameraRight_worldspace_ID  = glGetUniformLocation(programID, "CameraRight_worldspace");
  GLuint CameraUp_worldspace_ID  = glGetUniformLocation(programID, "CameraUp_worldspace");
  GLuint ViewProjMatrixID = glGetUniformLocation(programID, "VP");

  // fragment shader
  GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");


  static GLfloat* g_particule_position_size_data = new GLfloat[MaxParticles * 4];
  static GLubyte* g_particule_color_data         = new GLubyte[MaxParticles * 4];

  for(int i=0; i<MaxParticles; i++){
    ParticlesContainer[i].life = -1.0f;
    ParticlesContainer[i].cameradistance = -1.0f;
  }



  GLuint Texture = loadDDS((resourceDir() + "particle.DDS").c_str());

  // The VBO containing the 4 vertices of the particles.
  // Thanks to instancing, they will be shared by all particles.
  static const GLfloat g_vertex_buffer_data[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f,
    0.5f,  0.5f, 0.0f,
  };
  GLuint billboard_vertex_buffer;
  glGenBuffers(1, &billboard_vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

  // The VBO containing the positions and sizes of the particles
  GLuint particles_position_buffer;
  glGenBuffers(1, &particles_position_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
  // Initialize with empty (NULL) buffer : it will be updated later, each frame.
  glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

  // The VBO containing the colors of the particles
  GLuint particles_color_buffer;
  glGenBuffers(1, &particles_color_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
  // Initialize with empty (NULL) buffer : it will be updated later, each frame.
  glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);



  lastTime = glfwGetTime();



  do
  {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    double currentTime = glfwGetTime();
    double delta = currentTime - lastTime;
    lastTime = currentTime;


    glm::mat4 model = trackballGetRotationMatrix(ctx.trackball);
    glm::mat4 ViewMatrix = glm::lookAt(glm::vec3(0.0, 1.0, 4.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 ProjectionMatrix = glm::perspective(3.14159/2, 1.0f , 0.1f, 8.0f);
    glm::mat4 mv = ViewMatrix * model;
    glm::mat4 mvp = ProjectionMatrix * mv;


    //computeMatricesFromInputs();
    //glm::mat4 ProjectionMatrix = getProjectionMatrix();
    //glm::mat4 ViewMatrix = getViewMatrix();

    // We will need the camera's position in order to sort the particles
    // w.r.t the camera's distance.
    // There should be a getCameraPosition() function in common/controls.cpp,
    // but this works too.
    glm::vec3 CameraPosition(glm::inverse(ViewMatrix)[3]);

    glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;


    // Generate 10 new particule each millisecond,
    // but limit this to 16 ms (60 fps), or if you have 1 long frame (1sec),
    // newparticles will be huge and the next frame even longer.
    int newparticles = (int)(delta*10000.0);
    if (newparticles > (int)(0.016f*10000.0))
      newparticles = (int)(0.016f*10000.0);

    for(int i=0; i<newparticles; i++){
      int particleIndex = FindUnusedParticle();
      ParticlesContainer[particleIndex].life = 5.0f; // This particle will live 5 seconds.
      ParticlesContainer[particleIndex].pos = glm::vec3(0,0,-20.0f);

      float spread = 1.5f;
      glm::vec3 maindir = glm::vec3(0.0f, 10.0f, 0.0f);
      // Very bad way to generate a random direction;
      // See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
      // combined with some user-controlled parameters (main direction, spread, etc)
      glm::vec3 randomdir = glm::vec3(
          (rand()%2000 - 1000.0f)/1000.0f,
          (rand()%2000 - 1000.0f)/1000.0f,
          (rand()%2000 - 1000.0f)/1000.0f
          );

      ParticlesContainer[particleIndex].speed = maindir + randomdir*spread;


      // Very bad way to generate a random color
      ParticlesContainer[particleIndex].r = rand() % 256;
      ParticlesContainer[particleIndex].g = rand() % 256;
      ParticlesContainer[particleIndex].b = rand() % 256;
      ParticlesContainer[particleIndex].a = (rand() % 256) / 3;

      ParticlesContainer[particleIndex].size = (rand()%1000)/2000.0f + 0.1f;

    }



    // Simulate all particles
    int ParticlesCount = 0;
    for(int i=0; i<MaxParticles; i++){

      Particle& p = ParticlesContainer[i]; // shortcut

      if(p.life > 0.0f){

        // Decrease life
        p.life -= delta;
        if (p.life > 0.0f){

          // Simulate simple physics : gravity only, no collisions
          p.speed += glm::vec3(0.0f,-9.81f, 0.0f) * (float)delta * 0.5f;
          p.pos += p.speed * (float)delta;
          p.cameradistance = glm::length2( p.pos - CameraPosition );
          //ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

          // Fill the GPU buffer
          g_particule_position_size_data[4*ParticlesCount+0] = p.pos.x;
          g_particule_position_size_data[4*ParticlesCount+1] = p.pos.y;
          g_particule_position_size_data[4*ParticlesCount+2] = p.pos.z;

          g_particule_position_size_data[4*ParticlesCount+3] = p.size;

          g_particule_color_data[4*ParticlesCount+0] = p.r;
          g_particule_color_data[4*ParticlesCount+1] = p.g;
          g_particule_color_data[4*ParticlesCount+2] = p.b;
          g_particule_color_data[4*ParticlesCount+3] = p.a;

        }else{
          // Particles that just died will be put at the end of the buffer in SortParticles();
          p.cameradistance = -1.0f;
        }

        ParticlesCount++;

      }
    }

    SortParticles();


    //printf("%d ",ParticlesCount);


    // Update the buffers that OpenGL uses for rendering.
    // There are much more sophisticated means to stream data from the CPU to the GPU,
    // but this is outside the scope of this tutorial.
    // http://www.opengl.org/wiki/Buffer_Object_Streaming


    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
    glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
    glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use our shader
    glUseProgram(programID);

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    // Set our "myTextureSampler" sampler to user Texture Unit 0
    glUniform1i(TextureID, 0);

    // Same as the billboards tutorial
    glUniform3f(CameraRight_worldspace_ID, ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
    glUniform3f(CameraUp_worldspace_ID   , ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);

    glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &ViewProjectionMatrix[0][0]);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glVertexAttribPointer(
        0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
        );

    // 2nd attribute buffer : positions of particles' centers
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glVertexAttribPointer(
        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        4,                                // size : x + y + z + size => 4
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
        );

    // 3rd attribute buffer : particles' colors
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glVertexAttribPointer(
        2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        4,                                // size : r + g + b + a => 4
        GL_UNSIGNED_BYTE,                 // type
        GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
        0,                                // stride
        (void*)0                          // array buffer offset
        );

    // These functions are specific to glDrawArrays*Instanced*.
    // The first parameter is the attribute buffer we're talking about.
    // The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
    // http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
    glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
    glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
    glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1

    // Draw the particules !
    // This draws many times a small triangle_strip (which looks like a quad).
    // This is equivalent to :
    // for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4),
    // but faster.
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    // Swap buffers
    glfwSwapBuffers(ctx.window);
    glfwPollEvents();

  } // Check if the ESC key was pressed or the window was closed
  while( glfwGetKey(ctx.window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
      glfwWindowShouldClose(ctx.window) == 0 );


}

void drawParticles(Context &ctx)
{
  glBindVertexArray(ctx.particleVAO);
  glUseProgram(ctx.particleProgram);

  // === -------- ===

  double currentTime = glfwGetTime();
  double delta = currentTime - lastTime;
  lastTime = currentTime;

  // === -------- ===

  glm::mat4 model = trackballGetRotationMatrix(ctx.trackball);
  glm::mat4 view = glm::lookAt(glm::vec3(0.0, 1.0, 4.0), glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 projection = glm::perspective(1.0f, 1.0f , 0.1f, 8.0f);
  glm::mat4 viewProjection = projection * view;
  //glm::mat4 mv = view * model;
  //glm::mat4 mvp = projection * mv;

  glm::vec3 CameraPosition(glm::inverse(view)[3]);

  // === -------- ===

  // Generate 10 new particule each millisecond,
  // but limit this to 16 ms (60 fps), or if you have 1 long frame (1sec),
  // newparticles will be huge and the next frame even longer.
  int newparticles = (int)(delta*10000.0);
  if (newparticles > (int)(0.016f*10000.0))
    newparticles = (int)(0.016f*10000.0);

  for(int i=0; i<newparticles; i++){
    int particleIndex = FindUnusedParticle();
    ParticlesContainer[particleIndex].life = 5.0f; // This particle will live 5 seconds.
    ParticlesContainer[particleIndex].pos = glm::vec3(0,0,-20.0f);

    float spread = 1.5f;
    glm::vec3 maindir = glm::vec3(0.0f, 10.0f, 0.0f);
    // Very bad way to generate a random direction;
    // See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
    // combined with some user-controlled parameters (main direction, spread, etc)
    glm::vec3 randomdir = glm::vec3(
        (rand()%2000 - 1000.0f)/1000.0f,
        (rand()%2000 - 1000.0f)/1000.0f,
        (rand()%2000 - 1000.0f)/1000.0f
        );

    ParticlesContainer[particleIndex].speed = maindir + randomdir*spread;


    // Very bad way to generate a random color
    ParticlesContainer[particleIndex].r = rand() % 256;
    ParticlesContainer[particleIndex].g = rand() % 256;
    ParticlesContainer[particleIndex].b = rand() % 256;
    ParticlesContainer[particleIndex].a = (rand() % 256) / 3;

    ParticlesContainer[particleIndex].size = (rand()%1000)/2000.0f + 0.1f;

  }

  // === -------- ===

  // Simulate all particles
  int ParticlesCount = 0;
  for(int i=0; i<MaxParticles; i++){

    Particle& p = ParticlesContainer[i]; // shortcut

    if(p.life > 0.0f){

      // Decrease life
      p.life -= delta;
      if (p.life > 0.0f){

        // Simulate simple physics : gravity only, no collisions
        p.speed += glm::vec3(0.0f,-9.81f, 0.0f) * (float)delta * 0.5f;
        p.pos += p.speed * (float)delta;
        p.cameradistance = glm::length2( p.pos - CameraPosition );
        //ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

        // Fill the GPU buffer
        g_particule_position_size_data[4*ParticlesCount+0] = p.pos.x;
        g_particule_position_size_data[4*ParticlesCount+1] = p.pos.y;
        g_particule_position_size_data[4*ParticlesCount+2] = p.pos.z;

        g_particule_position_size_data[4*ParticlesCount+3] = p.size;

        g_particule_color_data[4*ParticlesCount+0] = p.r;
        g_particule_color_data[4*ParticlesCount+1] = p.g;
        g_particule_color_data[4*ParticlesCount+2] = p.b;
        g_particule_color_data[4*ParticlesCount+3] = p.a;

      }else{
        // Particles that just died will be put at the end of the buffer in SortParticles();
        p.cameradistance = -1.0f;
      }

      ParticlesCount++;

    }
  }

  // === -------- ===

  // Update the buffers that OpenGL uses for rendering.
  // There are much more sophisticated means to stream data from the CPU to the GPU,
  // but this is outside the scope of this tutorial.
  // http://www.opengl.org/wiki/Buffer_Object_Streaming

  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_position_buffer);
  glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
  glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

  glBindBuffer(GL_ARRAY_BUFFER, ctx.particles_color_buffer);
  glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
  glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);

  // === -------- ===

  // Pass uniforms
  //glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"), 1, GL_FALSE, &mv[0][0]);
  //glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, &mvp[0][0]);
  //glUniform1f(glGetUniformLocation(program, "u_time"), delta);

  GLuint CameraRight_worldspace_ID  = glGetUniformLocation(ctx.particleProgram, "CameraRight_worldspace");
  GLuint CameraUp_worldspace_ID  = glGetUniformLocation(ctx.particleProgram, "CameraUp_worldspace");
  GLuint ViewProjMatrixID = glGetUniformLocation(ctx.particleProgram, "VP");

  // fragment shader
  GLuint TextureID  = glGetUniformLocation(ctx.particleProgram, "myTextureSampler");

  // Bind our texture in Texture Unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx.texture);
  // Set our "myTextureSampler" sampler to user Texture Unit 0
  glUniform1i(TextureID, 0);

  // Same as the billboards tutorial
  glUniform3f(CameraRight_worldspace_ID, view[0][0], view[1][0], view[2][0]);
  glUniform3f(CameraUp_worldspace_ID   , view[0][1], view[1][1], view[2][1]);

  glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &viewProjection[0][0]);

  // === -------- ===

  // These functions are specific to glDrawArrays*Instanced*.
  // The first parameter is the attribute buffer we're talking about.
  // The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
  // http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
  glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
  glVertexAttribDivisor(1, 1); // positions : one per quad (its center) -> 1
  glVertexAttribDivisor(2, 1); // color : one per quad -> 1

  // Draw the particules !
  // This draws many times a small triangle_strip (which looks like a quad).
  // This is equivalent to :
  // for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4),
  // but faster.
  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);


  glBindVertexArray(ctx.defaultVAO);
  glUseProgram(0);
}

void drawTriangle(GLuint program, GLuint vao)
{
  glUseProgram(program);

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glUseProgram(0);
}

void display(Context &ctx)
{
  // Black background
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //drawTriangle(ctx.triangleProgram, ctx.triangleVAO);

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

void resizeCallback(GLFWwindow* window, int width, int height)
{
  Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
  ctx->width = width;
  ctx->height = height;
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

  // Load OpenGL functions
  glewExperimental = true;
  GLenum status = glewInit();
  if (status != GLEW_OK) {
    std::cerr << "Error: " << glewGetErrorString(status) << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

  // Initialize rendering
  glGenVertexArrays(1, &ctx.defaultVAO);
  glBindVertexArray(ctx.defaultVAO);

  puts("HELLO");
  init(ctx);
  puts("WHAT?");

  // Start rendering loop
  while (!glfwWindowShouldClose(ctx.window)) {
    glfwPollEvents();
    display(ctx);
    glfwSwapBuffers(ctx.window);
  }

  // Shutdown
  glfwDestroyWindow(ctx.window);
  glfwTerminate();
  std::exit(EXIT_SUCCESS);
}
