// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;

out vec3 v_color;

void main()
{
    gl_Position = vec4(-a_position.xyz, 1.0);
    v_color = vec3(1.0, 1.0, 0.0);
}
