#version 330 core

// Layout matches Chunk::uploadMeshToGPU
layout(location = 0) in vec3 aPos;     // position
layout(location = 1) in vec3 aNormal;  // normal
layout(location = 2) in vec2 aUV;      // texture coordinate
layout(location = 3) in float aAO;      // texture coordinate

// Outputs to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec2 UV;
out float AO;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    FragPos = vec3(u_Model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(u_Model))) * aNormal; 
    UV = aUV;
    AO = aAO;

    gl_Position = u_Projection * u_View * vec4(FragPos, 1.0);
}
