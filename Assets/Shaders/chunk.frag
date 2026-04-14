#version 410 core

in vec3 v_Normal;
in vec2 v_UV;
flat in int v_TextureIndex;
in float v_AO;

out vec4 v_FragColor;

uniform sampler2DArray u_Texture;

void main()
{
    vec4 texColor = texture(u_Texture, vec3(v_UV, v_TextureIndex));
    if (texColor.a < 0.5)
        discard;

    v_FragColor = vec4(texColor.rgb * v_AO, texColor.a);

    // v_FragColor = vec4(abs(v_Normal), 1.0);
}
