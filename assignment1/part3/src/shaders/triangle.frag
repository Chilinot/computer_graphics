// Fragment shader
#version 150

out vec4 frag_color;

uniform float u_time;

void main() {
    frag_color = vec4(sin(u_time), cos(u_time), 1.0, 1.0);
}
