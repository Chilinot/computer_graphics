// Fragment shader
#version 150

// Uniforms
uniform vec3 u_ambient_color, u_diffuse_color, u_specular_color, u_light_color;
uniform float u_specular_power;

// Toggle bools
uniform bool u_ambient_toggle, u_diffuse_toggle, u_specular_toggle, u_gamma_toggle, u_invert_toggle, u_normal_toggle;

// From vertex shader
in vec3 v_normal, n_normal, l_normal;

// Output
out vec4 frag_color;

void main()
{
    //vec3 N = normalize(v_normal);
    //frag_color = vec4(0.5 * N + 0.5, 1.0);

    vec3 I = vec3(0.0f);

    vec3 halfway = normalize(l_normal + v_normal);

    // Ambient
    if(u_ambient_toggle) {
        I += u_ambient_color;
    }

    // Diffuse
    if(u_diffuse_toggle) {
        I += u_diffuse_color * u_light_color * max(dot(n_normal, l_normal),0);
    }

    // Specular
    if(u_specular_toggle) {
        I += ((8.0 + u_specular_power) / 8.0) * u_specular_color * u_light_color * pow(dot(n_normal, halfway), u_specular_power);
    }

    // Normal colors
    if(u_normal_toggle) {
        I = n_normal;
    }

    // Invert colors
    if(u_invert_toggle) {
        I = vec3(1.0f) - I;
    }

    // gamma correction
    if(u_gamma_toggle) {
        I = pow(I, vec3(1 / 2.2));
    }


    frag_color = vec4(I, 1.0f);
}
