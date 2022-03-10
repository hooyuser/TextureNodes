#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    int texture_id;
    int lookup_id;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D nodeTextures[];

layout(set = 2, binding = 0) uniform sampler1D lookupTextures[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    if(ubo.texture_id >= 0) {
        float factor = texture(nodeTextures[ubo.texture_id], fragUV).r;
        outColor = texture(lookupTextures[ubo.lookup_id], factor);
    } else {
        outColor = texture(lookupTextures[ubo.lookup_id], fragUV.x);
    }
}
