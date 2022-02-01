#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 1) out vec3 fragPos;

void main() {
    fragPos = inPosition;
}