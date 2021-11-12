#version 450

layout (constant_id = 0) const int textureCubemapArraySize = 1;
layout (constant_id = 1) const int baseColorTextureId = -1;

layout(set = 0, binding = 2) uniform samplerCube cubemapSampler[textureCubemapArraySize];

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(cubemapSampler[baseColorTextureId], fragTexCoord);
}