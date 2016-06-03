#version 330 core

in vec2 UV;
in vec4 particlecolor;

out vec4 color;

uniform sampler2D u_input_texture;

void main(){
    color = texture(u_input_texture, UV) * particlecolor;
}
