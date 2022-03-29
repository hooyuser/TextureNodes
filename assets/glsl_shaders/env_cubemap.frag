#version 460
#extension GL_EXT_nonuniform_qualifier:enable

layout (constant_id = 0) const int baseColorTextureId = -1;

layout(set = 1, binding = 0) uniform samplerCube cubemapSampler[];

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(cubemapSampler[baseColorTextureId], fragTexCoord);
}