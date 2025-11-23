#version 330 core

struct Block
{
    sampler2DArray Diffuse;
    vec2 MaxTexSize;
};

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_UV;
flat in ivec2 v_TexSize;
flat in int v_Layer;
in float v_AO;

out vec4 v_FragColor;

uniform Block u_Block;
uniform float u_AlphaCutoff = 0.5;

void main()
{
    vec2 scaledUV = v_UV * (v_TexSize / u_Block.MaxTexSize);
    vec4 texColor = texture(u_Block.Diffuse, vec3(scaledUV, v_Layer));

    vec3 color = texColor.rgb * v_AO;

    if (texColor.a < u_AlphaCutoff)
        discard;   // Alpha test

    v_FragColor = vec4(color, texColor.a);
    // FragColor = vec4(0.9, 0.5, 0.2, 1.0);
}
