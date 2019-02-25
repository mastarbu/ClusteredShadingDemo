# version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec3 f_pos;
layout (location = 1) out vec3 f_norm;
layout (location = 2) out vec2 f_uv;

layout(set = 0, binding = 0) uniform UBO {
    mat4 MVP;
    vec4 veiw;
    vec4 lightPos;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    f_pos = pos;
    f_norm = norm;
    f_uv = uv;
    gl_Position = ubo.MVP * vec4(pos.xyz, 1.0); 
}