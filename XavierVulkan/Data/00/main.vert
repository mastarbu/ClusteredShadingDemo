#version 450 core 

layout(location = 0) in vec3 i_vertex_pos;
layout(location = 1) in vec4 i_vertex_color;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec4 v_color;
void main()
{
    v_color = i_vertex_color;
    gl_Position = vec4(i_vertex_pos, 1.0);
}