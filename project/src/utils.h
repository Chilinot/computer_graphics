#pragma once

#include <GL/glew.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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
    std::string rootDir = getEnvVar("PROJECT_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: PROJECT_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/src/shaders/";
}

std::string resourceDir(void)
{
    std::string rootDir = getEnvVar("PROJECT_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: PROJECT_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/src/resources/";
}

std::string readShaderSource(const std::string &filename)
{
    std::ifstream file(filename);
    std::stringstream stream;
    stream << file.rdbuf();

    return stream.str();
}

void showShaderInfoLog(GLuint shader)
{
    GLint infoLogLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    std::vector<char> infoLog(infoLogLength);
    glGetShaderInfoLog(shader, infoLogLength, &infoLogLength, &infoLog[0]);
    std::string infoLogStr(infoLog.begin(), infoLog.end());
    std::cerr << infoLogStr << std::endl;
}

void showProgramInfoLog(GLuint program)
{
    GLint infoLogLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    std::vector<char> infoLog(infoLogLength);
    glGetProgramInfoLog(program, infoLogLength, &infoLogLength, &infoLog[0]);
    std::string infoLogStr(infoLog.begin(), infoLog.end());
    std::cerr << infoLogStr << std::endl;
}

GLuint loadShaderProgram(const std::string &vertexShaderFilename,
                         const std::string &fragmentShaderFilename)
{
    // Load and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::string vertexShaderSource = readShaderSource(vertexShaderFilename);
    const char *vertexShaderSourcePtr = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);

    glCompileShader(vertexShader);
    GLint compiled = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cerr << "Vertex shader compilation failed:" << std::endl;
        showShaderInfoLog(vertexShader);
        glDeleteShader(vertexShader);
        return 0;
    }

    // Load and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fragmentShaderSource = readShaderSource(fragmentShaderFilename);
    const char *fragmentShaderSourcePtr = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);

    glCompileShader(fragmentShader);
    compiled = 0;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cerr << "Fragment shader compilation failed:" << std::endl;
        showShaderInfoLog(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    // Create program object
    GLuint program = glCreateProgram();

    // Attach shaders to the program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    // Link program
    glLinkProgram(program);

    // Check linking status
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        std::cerr << "Linking failed:" << std::endl;
        showProgramInfoLog(program);
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    // Clean up
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    return program;
}
