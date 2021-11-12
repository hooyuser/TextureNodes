#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragTexCoord;

void main() {
    mat4 view = ubo.view;
    view[3][0] = 0.0;
    view[3][1] = 0.0;
    view[3][2] = 0.0;
    vec4 pos = ubo.proj * view * vec4(inPosition, 1.0);
    gl_Position = pos.xyww;
    fragTexCoord = inPosition;
}