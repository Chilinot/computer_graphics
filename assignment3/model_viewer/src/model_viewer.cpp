// Assignment 3, Part 1 and 2
//
// Modify this file according to the lab instructions.
//

#include "utils.h"
#include "utils2.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef WITH_TWEAKBAR
#include <AntTweakBar.h>
#endif // WITH_TWEAKBAR

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdlib>
#include <algorithm>

GLfloat skyboxVertices[] = {
  // Positions
  -1.0f,  1.0f, -1.0f,
  -1.0f, -1.0f, -1.0f,
  1.0f, -1.0f, -1.0f,
  1.0f, -1.0f, -1.0f,
  1.0f,  1.0f, -1.0f,
  -1.0f,  1.0f, -1.0f,

  -1.0f, -1.0f,  1.0f,
  -1.0f, -1.0f, -1.0f,
  -1.0f,  1.0f, -1.0f,
  -1.0f,  1.0f, -1.0f,
  -1.0f,  1.0f,  1.0f,
  -1.0f, -1.0f,  1.0f,

  1.0f, -1.0f, -1.0f,
  1.0f, -1.0f,  1.0f,
  1.0f,  1.0f,  1.0f,
  1.0f,  1.0f,  1.0f,
  1.0f,  1.0f, -1.0f,
  1.0f, -1.0f, -1.0f,

  -1.0f, -1.0f,  1.0f,
  -1.0f,  1.0f,  1.0f,
  1.0f,  1.0f,  1.0f,
  1.0f,  1.0f,  1.0f,
  1.0f, -1.0f,  1.0f,
  -1.0f, -1.0f,  1.0f,

  -1.0f,  1.0f, -1.0f,
  1.0f,  1.0f, -1.0f,
  1.0f,  1.0f,  1.0f,
  1.0f,  1.0f,  1.0f,
  -1.0f,  1.0f,  1.0f,
  -1.0f,  1.0f, -1.0f,

  -1.0f, -1.0f, -1.0f,
  -1.0f, -1.0f,  1.0f,
  1.0f, -1.0f, -1.0f,
  1.0f, -1.0f, -1.0f,
  -1.0f, -1.0f,  1.0f,
  1.0f, -1.0f,  1.0f
};

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
    POSITION = 0,
    NORMAL = 1
};

// Struct for representing an indexed triangle mesh
struct Mesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
};

// Struct for representing a vertex array object (VAO) created from a
// mesh. Used for rendering.
struct MeshVAO {
    GLuint vao;
    GLuint vertexVBO;
    GLuint normalVBO;
    GLuint indexVBO;
    int numVertices;
    int numIndices;
};

// Struct for resources and state
struct Context {
    int width;
    int height;
    float aspect;
    GLFWwindow *window;
    GLuint program;
    Trackball trackball;
    Mesh mesh;
    MeshVAO meshVAO;
    GLuint defaultVAO;
    float elapsed_time;

    // My settings
    bool ambient_toggle;
    bool diffuse_toggle;
    bool specular_toggle;
    bool gamma_toggle;
    bool invert_toggle;
    bool normal_toggle;
    float zoom_factor;
    bool ortho_projection;

    float background_color[3];
    float background_intensity;

    glm::vec3 eye_position;

    // Uniforms
    glm::vec3 light_position;
    glm::vec3 light_color;
    glm::vec3 ambient_color;
    glm::vec3 diffuse_color;
    glm::vec3 specular_color;
    float specular_power;

    // Cubemaps
    GLuint cubemap_0;
    GLuint cubemap_1;
    GLuint cubemap_2;
    GLuint cubemap_3;
    GLuint cubemap_4;
    GLuint cubemap_5;
    GLuint cubemap_6;
    GLuint cubemap_7;

    GLuint* cubemap;
    int cubemap_choice;

    // Skybox
    GLuint skyboxProgram;
    GLuint skyboxCubemap;
    GLuint skyboxVBO;
    GLuint skyboxVAO;
};

// Returns the value of an environment variable
std::string getEnvVar(const std::string &name)
{
    char const* value = std::getenv(name.c_str());
    if (value == nullptr) {
        return std::string();
    }
    else {
        return std::string(value);
    }
}

// Returns the absolute path to the shader directory
std::string shaderDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/src/shaders/";
}

// Returns the absolute path to the 3D model directory
std::string modelDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/3d_models/";
}

// Returns the absolute path to the cubemap texture directory
std::string cubemapDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/cubemaps/";
}

void loadMesh(const std::string &filename, Mesh *mesh)
{
    OBJMesh obj_mesh;
    objMeshLoad(obj_mesh, filename);
    mesh->vertices = obj_mesh.vertices;
    mesh->normals = obj_mesh.normals;
    mesh->indices = obj_mesh.indices;
}

void createMeshVAO(Context &ctx, const Mesh &mesh, MeshVAO *meshVAO)
{
    // Generates and populates a VBO for the vertices
    glGenBuffers(1, &(meshVAO->vertexVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    auto verticesNBytes = mesh.vertices.size() * sizeof(mesh.vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, verticesNBytes, mesh.vertices.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the vertex normals
    glGenBuffers(1, &(meshVAO->normalVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    auto normalsNBytes = mesh.normals.size() * sizeof(mesh.normals[0]);
    glBufferData(GL_ARRAY_BUFFER, normalsNBytes, mesh.normals.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the element indices
    glGenBuffers(1, &(meshVAO->indexVBO));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    auto indicesNBytes = mesh.indices.size() * sizeof(mesh.indices[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesNBytes, mesh.indices.data(), GL_STATIC_DRAW);

    // Creates a vertex array object (VAO) for drawing the mesh
    glGenVertexArrays(1, &(meshVAO->vao));
    glBindVertexArray(meshVAO->vao);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    glEnableVertexAttribArray(NORMAL);
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    glBindVertexArray(ctx.defaultVAO); // unbinds the VAO

    // Additional information required by draw calls
    meshVAO->numVertices = mesh.vertices.size();
    meshVAO->numIndices = mesh.indices.size();
}

void createSkyboxVAO(Context &ctx)
{
  // Generate 1 buffer and store the ID at ctx.skyboxVBO
  glGenBuffers(1, &ctx.skyboxVBO);

  // Generate 1 VAO
  glGenVertexArrays(1, &ctx.skyboxVAO);

  // Bind the VAO
  glBindVertexArray(ctx.skyboxVAO);

  // Bind the new VBO to the GL_ARRAY_BUFFER target
  glBindBuffer(GL_ARRAY_BUFFER, ctx.skyboxVBO);

  // Store the skyboxVertices array in the buffer bound to the GL_ARRAY_BUFFER target.
  // Since the vertices of the skybox will never change we define the data as GL_STATIC_DRAW.
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

  // Specify the layout of the data in the VBO and store it in layout position 2 (first argument)
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLfloat*) 0);

  // Enable the layout
  glEnableVertexAttribArray(2);

  // Unbind the VAO so that we don't accidentaly change anything in it
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
    // General settings
    ctx.ambient_toggle   = true;
    ctx.diffuse_toggle   = true;
    ctx.specular_toggle  = true;
    ctx.gamma_toggle     = true;
    ctx.invert_toggle    = false;
    ctx.normal_toggle    = false;
    ctx.zoom_factor      = 1.0f;
    ctx.ortho_projection = false;

    ctx.background_color[0] = 0.3f;
    ctx.background_color[1] = 0.3f;
    ctx.background_color[2] = 0.3f;
    ctx.background_intensity = 1.0f;

    ctx.eye_position = glm::vec3(0.0f, 0.0f, -1.0f);

    // Uniforms
    ctx.light_position = glm::vec3(0.0f, 3.0f, 0.0f);
    ctx.light_color    = glm::vec3(1.0f);

    ctx.ambient_color  = glm::vec3(0.3f, 0.0f, 0.0f);
    ctx.diffuse_color  = glm::vec3(0.3f, 0.0f, 0.0f);
    ctx.specular_color = glm::vec3(0.04f);

    ctx.specular_power = 100.0f;

    // Shader programs
    ctx.program = loadShaderProgram(shaderDir() + "mesh.vert",
                                    shaderDir() + "mesh.frag");
    ctx.skyboxProgram = loadShaderProgram(shaderDir() + "skybox.vert",
                                          shaderDir() + "skybox.frag");

    // Mesh
    loadMesh((modelDir() + "bunny.obj"), &ctx.mesh);
    createMeshVAO(ctx, ctx.mesh, &ctx.meshVAO);

    // Skybox
    ctx.skyboxCubemap = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/2048");
    createSkyboxVAO(ctx);

    // Load cubemap texture(s)
    ctx.cubemap_0 = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/0.125");
    ctx.cubemap_1 = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/0.5");
    ctx.cubemap_2 = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/2");
    ctx.cubemap_3 = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/8");
    ctx.cubemap_4 = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/32");
    ctx.cubemap_5 = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/128");
    ctx.cubemap_6 = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/512");
    ctx.cubemap_7 = loadCubemap(cubemapDir() + "/RomeChurch/prefiltered/2048");

    ctx.cubemap_choice = 1;

    initializeTrackball(ctx);
}

// MODIFY THIS FUNCTION
void drawMesh(Context &ctx, GLuint program, const MeshVAO &meshVAO)
{
    // Define uniforms
    glm::mat4 model = trackballGetRotationMatrix(ctx.trackball);
    model           = glm::scale(model, glm::vec3(0.5f));

    glm::mat4 view  = glm::lookAt(ctx.eye_position,  glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection;
    if(ctx.ortho_projection) {
      projection = glm::ortho(-ctx.aspect, ctx.aspect, -1.0f, 1.0f, -1.0f, 10.0f);
    }
    else {
      projection = glm::perspective((3.14159f/2)*ctx.zoom_factor, (float)ctx.width/(float)ctx.height, 0.1f, 10.0f);
    }

    glm::mat4 mv  = view * model;
    glm::mat4 mvp = projection * mv;

    // --- Skybox

    glDepthMask(GL_FALSE);

    glUseProgram(ctx.skyboxProgram);

    glUniformMatrix4fv(glGetUniformLocation(ctx.skyboxProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(ctx.skyboxProgram, "view"), 1, GL_FALSE, &view[0][0]);

    glBindVertexArray(ctx.skyboxVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.skyboxCubemap);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(ctx.defaultVAO);

    glDepthMask(GL_TRUE);

    // --- Regular Model
    glUseProgram(program);

    // Bind textures
    switch(ctx.cubemap_choice) {
      case 1: ctx.cubemap = &ctx.cubemap_0; break;
      case 2: ctx.cubemap = &ctx.cubemap_1; break;
      case 3: ctx.cubemap = &ctx.cubemap_2; break;
      case 4: ctx.cubemap = &ctx.cubemap_3; break;
      case 5: ctx.cubemap = &ctx.cubemap_4; break;
      case 6: ctx.cubemap = &ctx.cubemap_5; break;
      case 7: ctx.cubemap = &ctx.cubemap_6; break;
      case 8: ctx.cubemap = &ctx.cubemap_7; break;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *(ctx.cubemap));

    // Pass uniforms
    glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_mv"), 1, GL_FALSE, &mv[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_mvp"), 1, GL_FALSE, &mvp[0][0]);
    glUniform1f(glGetUniformLocation(ctx.program, "u_time"), ctx.elapsed_time);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_light_position"), 1, &ctx.light_position[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_light_color"),    1, &ctx.light_color[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_ambient_color"),  1, &ctx.ambient_color[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_diffuse_color"),  1, &ctx.diffuse_color[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_specular_color"), 1, &ctx.specular_color[0]);
    glUniform1f(glGetUniformLocation(ctx.program, "u_specular_power"),      ctx.specular_power);

    // Cubemap
    glUniform1i(glGetUniformLocation(ctx.program, "u_cubemap"), GL_TEXTURE0);

    // Toggles
    glUniform1i(glGetUniformLocation(ctx.program, "u_ambient_toggle"),  ctx.ambient_toggle);
    glUniform1i(glGetUniformLocation(ctx.program, "u_diffuse_toggle"),  ctx.diffuse_toggle);
    glUniform1i(glGetUniformLocation(ctx.program, "u_specular_toggle"), ctx.specular_toggle);
    glUniform1i(glGetUniformLocation(ctx.program, "u_gamma_toggle"),    ctx.gamma_toggle);
    glUniform1i(glGetUniformLocation(ctx.program, "u_invert_toggle"),   ctx.invert_toggle);
    glUniform1i(glGetUniformLocation(ctx.program, "u_normal_toggle"),   ctx.normal_toggle);

    // Draw!
    glBindVertexArray(meshVAO.vao);
    glDrawElements(GL_TRIANGLES, meshVAO.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(ctx.defaultVAO);
}

void display(Context &ctx)
{
    glClearColor(ctx.background_color[0], ctx.background_color[1], ctx.background_color[2], ctx.background_intensity);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST); // ensures that polygons overlap correctly
    drawMesh(ctx, ctx.program, ctx.meshVAO);
}

void reloadShaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = loadShaderProgram(shaderDir() + "mesh.vert",
                                     shaderDir() + "mesh.frag");
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

void moveTrackball(Context *ctx, int x, int y)
{
    if (ctx->trackball.tracking) {
        trackballMove(ctx->trackball, glm::vec2(x, y));
    }
}

void errorCallback(int error, const char* description)
{
    std::cerr << description << std::endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
#ifdef WITH_TWEAKBAR
    if (TwEventKeyGLFW3(window, key, scancode, action, mods)) return;
#endif // WITH_TWEAKBAR

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS) {
        switch(key) {
            // Reload shaders
            case GLFW_KEY_S: reloadShaders(ctx); break;
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
#ifdef WITH_TWEAKBAR
    if (TwEventMouseButtonGLFW3(window, button, action, mods)) return;
#endif // WITH_TWEAKBAR

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

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
#ifdef WITH_TWEAKBAR
    if (TwEventCursorPosGLFW3(window, x, y)) return;
#endif // WITH_TWEAKBAR

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    moveTrackball(ctx, x, y);
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
#ifdef WITH_TWEAKBAR
    TwWindowSize(width, height);
#endif // WITH_TWEAKBAR

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    ctx->aspect = float(width) / float(height);
    ctx->trackball.radius = double(std::min(width, height)) / 2.0;
    ctx->trackball.center = glm::vec2(width, height) / 2.0f;
    glViewport(0, 0, width, height);
}

int main(void)
{
    Context ctx;

    // Create a GLFW window
    glfwSetErrorCallback(errorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    ctx.width = 500;
    ctx.height = 500;
    ctx.aspect = float(ctx.width) / float(ctx.height);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Model viewer", nullptr, nullptr);
    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);
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

    // Initialize AntTweakBar (if enabled)
#ifdef WITH_TWEAKBAR
    TwInit(TW_OPENGL_CORE, nullptr);
    TwWindowSize(ctx.width, ctx.height);
    TwBar *tweakbar = TwNewBar("Settings");

    // Light
    TwAddVarRW(tweakbar, "Light Position", TW_TYPE_DIR3F, &ctx.light_position, "");
    TwAddVarRW(tweakbar, "Light Color", TW_TYPE_COLOR3F, &ctx.light_color, "");

    // Colors
    TwAddSeparator(tweakbar, NULL, "");
    TwAddVarRW(tweakbar, "Ambient Color", TW_TYPE_COLOR3F, &ctx.ambient_color, "");
    TwAddVarRW(tweakbar, "Diffuse Color", TW_TYPE_COLOR3F, &ctx.diffuse_color, "");
    TwAddVarRW(tweakbar, "Specular Color", TW_TYPE_COLOR3F, &ctx.specular_color, "");
    TwAddVarRW(tweakbar, "Background Color", TW_TYPE_COLOR3F, &ctx.background_color, "");

    // Toggle
    TwAddSeparator(tweakbar, NULL, "");
    TwAddVarRW(tweakbar, "Ambient",  TW_TYPE_BOOLCPP, &ctx.ambient_toggle, "");
    TwAddVarRW(tweakbar, "Diffuse",  TW_TYPE_BOOLCPP, &ctx.diffuse_toggle, "");
    TwAddVarRW(tweakbar, "Specular", TW_TYPE_BOOLCPP, &ctx.specular_toggle, "");
    TwAddVarRW(tweakbar, "Gamma",    TW_TYPE_BOOLCPP, &ctx.gamma_toggle, "");
    TwAddVarRW(tweakbar, "Invert",   TW_TYPE_BOOLCPP, &ctx.invert_toggle, "");
    TwAddVarRW(tweakbar, "Normal",   TW_TYPE_BOOLCPP, &ctx.normal_toggle, "");
    TwAddVarRW(tweakbar, "Orthogonal Projection", TW_TYPE_BOOLCPP, &ctx.ortho_projection, "");

    // Misc
    TwAddSeparator(tweakbar, NULL, "");
    TwAddVarRW(tweakbar, "Specular Power", TW_TYPE_FLOAT, &ctx.specular_power, "");
    TwAddVarRW(tweakbar, "Zoom",           TW_TYPE_FLOAT, &ctx.zoom_factor,    "min=0.1 max=1.9 step=0.01");
    TwAddVarRW(tweakbar, "Cubemap",        TW_TYPE_INT32, &ctx.cubemap_choice, "min=0 max=8");
    TwAddVarRW(tweakbar, "Eye Direction",  TW_TYPE_DIR3F, &ctx.eye_position, "");
#endif // WITH_TWEAKBAR

    // Initialize rendering
    glGenVertexArrays(1, &ctx.defaultVAO);
    glBindVertexArray(ctx.defaultVAO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    init(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsed_time = glfwGetTime();
        display(ctx);
#ifdef WITH_TWEAKBAR
        TwDraw();
#endif // WITH_TWEAKBAR
        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
#ifdef WITH_TWEAKBAR
    TwTerminate();
#endif // WITH_TWEAKBAR
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
