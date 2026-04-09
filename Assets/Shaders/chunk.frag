#version 330 core

in vec3 v_Normal;
in vec2 v_UV;

out vec4 v_FragColor;

uniform sampler2D u_Texture;

void main()
{
    v_FragColor = texture(u_Texture, v_UV);
    // v_FragColor = vec4(abs(v_Normal), 1.0);
}
