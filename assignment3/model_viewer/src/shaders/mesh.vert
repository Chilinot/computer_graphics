// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_normal;

out vec3 v_color;

out vec3 v_normal;
out vec3 n_normal;
out vec3 l_normal;

uniform mat4  u_mvp;
uniform mat4  u_mv;
uniform float u_time;
uniform vec3  u_light_position;

void main()
{
    //v_normal = a_normal;

    gl_Position = u_mvp * a_position;

    // Copy-pasted from assignment 2
    // Transform the vertex position to view space (eye coordinates)
    vec3 position_eye = vec3(u_mv * a_position);

    // Calculate the view-space normal
    vec3 N = normalize(mat3(u_mv) * a_normal);

    // Calculate the view-space light direction
    vec3 L = normalize(u_light_position - position_eye);

    // Calculate the diffuse (Lambertian) reflection term
    float diffuse = max(0.0, dot(N, L));

    // Multiply the diffuse reflection term with the surface color
    v_color = diffuse * vec3(0.0, 1.0, 0.0);

    v_normal = normalize(-position_eye);
    n_normal = N;
    l_normal = L;
}
