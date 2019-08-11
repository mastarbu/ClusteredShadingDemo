#version 450 core

layout (location = 0) out vec4 o_color;

void main()
{
    o_color = vec4(vec3(gl_FragCoord.z), 1.f);
}