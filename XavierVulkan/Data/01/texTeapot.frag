# version 450

layout (location = 0) in vec3 f_pos;
layout (location = 1) in vec3 f_norm;
layout (location = 2) in vec2 f_uv;

layout (location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform UBO {
    mat4 MVP;
    vec4 view;
    vec4 lightPos;
} ubo;

layout(set = 1, binding = 0) uniform Material {
    vec4 kA;
    vec4 kS;
    vec4 kD;
    vec4 kE;
} material;

layout(set = 1, binding = 1) uniform sampler2D diffuseTex;

void main()
{
    vec3 V = normalize(vec3(ubo.view.xyz) - f_pos);
    vec3 L = normalize(vec3(ubo.lightPos.xyz) - f_pos);
    vec3 N = normalize(f_norm);
    vec3 H = normalize(V + L);
    vec3 color = (material.kD * texture(diffuseTex, f_uv) * dot(N, L) + material.kS * pow(dot(N, H), 10)).rgb;
    o_color = vec4(color.rgb, 1.0);
}