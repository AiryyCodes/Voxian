#version 330 core

in vec3 v_Normal;

out vec4 v_FragColor;

void main()
{
    v_FragColor = vec4(abs(v_Normal), 1.0);
}
