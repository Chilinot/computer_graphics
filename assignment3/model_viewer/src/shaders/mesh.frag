// Fragment shader
#version 150

in vec3 v_color;

in vec3 v_normal;
in vec3 n_normal;
in vec3 l_normal;

out vec4 frag_color;

void main()
{
    vec3 N = normalize(v_normal);
    //frag_color = vec4(0.5 * N + 0.5, 1.0);

    // Copy pasted from assignement 2
    frag_color = vec4(v_color, 1.0);
}
