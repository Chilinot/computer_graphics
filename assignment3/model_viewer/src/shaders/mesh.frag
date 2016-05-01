// Fragment shader
#version 150

uniform vec3 u_ambient_color, u_diffuse_color, u_specular_color, u_light_color;
uniform float u_specular_power;

in vec3 v_normal, n_normal, l_normal;

out vec4 frag_color;

void main()
{
    //vec3 N = normalize(v_normal);
    //frag_color = vec4(0.5 * N + 0.5, 1.0);

    vec3 I = vec3(0.0f);

    vec3 halfway = normalize(l_normal + v_normal);

    // Ambient
    I += u_ambient_color;

    // Diffuse
    I += u_diffuse_color * u_light_color * max(dot(n_normal, l_normal),0);

    // Specular
    I += u_specular_color * u_light_color * ((8.0 + u_specular_color) / 8.0) * pow(dot(n_normal, halfway), u_specular_power);


    frag_color = vec4(I, 1.0f);
}
