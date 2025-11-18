#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;
in float AO;

out vec4 FragColor;

uniform sampler2D u_Texture;

void main()
{
    vec3 color = vec3(AO);

    FragColor = vec4(color, 1.0);
    // FragColor = vec4(0.9, 0.5, 0.2, 1.0);
}
