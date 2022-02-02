#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 color;
} ubo;

layout(location = 0) out vec4 outColor;


void main() {
	outColor = ubo.color;
}