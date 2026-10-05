#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 frag_color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 base_color;
    int use_vertex_color;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 1.0);
    if (ubo.use_vertex_color == 1) {
        frag_color = in_color * ubo.base_color.rgb;
    } else {
        frag_color = ubo.base_color.rgb;
    }
}