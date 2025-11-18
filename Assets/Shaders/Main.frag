#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_UV;
in float v_AO;

out vec4 v_FragColor;

uniform sampler2D u_Texture;

void main()
{
    vec3 color = vec3(v_AO);

    v_FragColor = vec4(color, 1.0);
    // FragColor = vec4(0.9, 0.5, 0.2, 1.0);
}
