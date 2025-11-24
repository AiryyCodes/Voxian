#version 410 core

struct Block {
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

uniform vec3 u_LightDir      = normalize(vec3(0.5, -1.0, 0.0));
uniform vec3 u_LightColor    = vec3(1.2, 1.2, 1.2);
uniform vec3 u_AmbientColor  = vec3(0.7, 0.7, 0.7);
uniform float u_AlphaCutoff = 0.5;

vec3 tonemap(vec3 x)
{
    return x / (1.0 + x);
}

void main()
{
    vec2 epsilon = 0.5 / vec2(v_TexSize);
    vec2 uv = clamp(v_UV, epsilon, 1.0 - epsilon);

    vec2 scaledUV = uv * (vec2(v_TexSize) / u_Block.MaxTexSize);

    vec2 ddx = dFdx(scaledUV);
    vec2 ddy = dFdy(scaledUV);
    vec4 texColor = textureGrad(u_Block.Diffuse, vec3(scaledUV, v_Layer), ddx, ddy);

    if (texColor.a < u_AlphaCutoff)
        discard;

    vec3 N = normalize(v_Normal);

    float NdotL = max(dot(N, -u_LightDir), 0.0);

    vec3 diffuse  = u_LightColor * NdotL;
    vec3 ambient  = u_AmbientColor;

    float ao = v_AO * 1.1;

    vec3 lit = (diffuse + ambient) * ao * texColor.rgb;

    // Filmic tone mapping for soft contrast
    lit = tonemap(lit);

    v_FragColor = vec4(lit, texColor.a);
}

