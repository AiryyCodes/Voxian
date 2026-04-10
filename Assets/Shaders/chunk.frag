#version 420 core

in vec3 v_Normal;
in vec2 v_UV;
flat in int v_TextureIndex;

out vec4 v_FragColor;

uniform sampler2DArray u_Texture;

void main()
{
    v_FragColor = texture(u_Texture, vec3(v_UV, v_TextureIndex));
    // v_FragColor = vec4(abs(v_Normal), 1.0);
}
