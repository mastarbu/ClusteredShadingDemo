#version 450 core
layout (location = 0) in vec3 pos;

layout(set = 0, binding = 0) uniform UBO {
    mat4 MVP;
    vec4 veiw;
    vec4 lightPos;
} ;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = MVP * vec4(pos, 1);
}