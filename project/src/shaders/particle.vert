#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 a_squareVertices;
layout(location = 1) in vec4 a_particle; // Position of the center of the particule and size of the square
layout(location = 2) in vec4 a_color; // Position of the center of the particule and size of the square

// Output data ; will be interpolated for each fragment.
out vec2 UV;
out vec4 particlecolor;

// Values that stay constant for the whole mesh.
uniform vec3 u_camera_right;
uniform vec3 u_camera_up;
uniform mat4 u_VP; // Model-View-Projection matrix, but without the Model (the position is in BillboardPos; the orientation depends on the camera)

void main()
{
    // The first three values represent the particles center position
    vec3 p_center = a_particle.xyz;

    // The fourth value represents the size of the particle
    float p_size = a_particle.w;

    // Position of the vertex in the world space
    vec3 v_pos = p_center + u_camera_right * a_squareVertices.x * p_size + u_camera_up * a_squareVertices.y * p_size;

    gl_Position = u_VP * vec4(v_pos, 1.0f);

    // Pass values to fragment shader
    UV = a_squareVertices.xy + vec2(0.5, 0.5);
    particlecolor = a_color;
}
