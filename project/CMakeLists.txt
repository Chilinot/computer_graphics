cmake_minimum_required(VERSION 2.6.0)

# Specify project name
project(comp_graph_project)

# Set extra compiler flags
if(UNIX)
  set(CMAKE_CXX_FLAGS "-W -Wall -std=c++0x")
endif(UNIX)

# Add source directories
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src" comp_graph_project_SRCS)

# Add include directories
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

# Create variable for required libraries
set(requiredLibs)

# GLFW
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/glfw" ${CMAKE_CURRENT_BINARY_DIR}/glfw)
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include")

# OpenGL
find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
  include_directories( SYSTEM ${OPENGL_INCLUDE_DIR})
  set(requiredLibs ${requiredLibs} ${OPENGL_LIBRARIES})
endif(OPENGL_FOUND)

# GLEW
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/external/glew/src" comp_graph_project_SRCS)
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include")
add_definitions(-DGLEW_STATIC)

# GLM
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/external/glm")

# lodepng
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng" comp_graph_project_SRCS)
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng")

# AntTweakBar (uncomment to enable)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/external/anttweakbar/src" comp_graph_project_SRCS)
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/external/anttweakbar/include")
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/external/anttweakbar/src")
add_definitions(-DTW_STATIC -DTW_NO_LIB_PRAGMA)
if(UNIX)
  add_definitions(-D_UNIX)
endif(UNIX)
add_definitions(-DWITH_TWEAKBAR)

# Create build files for executable
add_executable(comp_graph_project ${comp_graph_project_SRCS})

# Link against libraries
target_link_libraries(comp_graph_project glfw ${requiredLibs} ${GLFW_LIBRARIES})

# Install executable
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/comp_graph_project DESTINATION bin)

# Specify build type
set(CMAKE_BUILD_TYPE Release)
