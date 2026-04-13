#version 410 core

layout(location = 0) in uint Data1;
layout(location = 1) in uint Data2;
layout(location = 2) in vec4 UVBounds;

uniform mat4 u_Transform;
uniform mat4 u_View;
uniform mat4 u_Projection;

uniform vec3 u_ChunkOrigin;

out vec2  v_UV;
out vec3  v_Normal;
flat out int v_TextureIndex;
out float v_AO;

const vec3 normals[6] = vec3[](
    vec3( 1, 0, 0),  // Right
    vec3(-1, 0, 0),  // Left
    vec3( 0, 1, 0),  // Up
    vec3( 0,-1, 0),  // Down
    vec3( 0, 0, 1),  // Forward
    vec3( 0, 0,-1)   // Backward
);

const vec2 uvs[4] = vec2[](
    vec2(0, 0),
    vec2(1, 0),
    vec2(1, 1),
    vec2(0, 1)
);

void main()
{
    float x = float(Data1 & 0x3FFu) / 16.0;
    float y = float((Data1 >> 10) & 0xFFFu) / 16.0;
    float z = float((Data1 >> 22) & 0x3FFu) / 16.0;
    int normalIndex  = int((Data2 >> 18) & 0x7u);
    int cornerIndex  = int(Data2 & 0x3u);
    v_TextureIndex   = int((Data2 >> 2) & 0xFFFFu);

    v_Normal = normals[normalIndex];
    vec2 uvCorner = uvs[cornerIndex];
    v_UV = mix(UVBounds.xy, UVBounds.zw, uvCorner);
    
    float ao = float((Data2 >> 21) & 0x3u) / 3.0;
    v_AO = ao;

    gl_Position = u_Projection * u_View * u_Transform * vec4(u_ChunkOrigin + vec3(x, y, z), 1.0);
}
