#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    int texture_id;
    float strength;
    float max_range;
    bool match_size;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D node2dTextures[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

ivec2 texture_size = textureSize(node2dTextures[ubo.texture_id], 0);

vec2 clamp_uv(vec2 uv){
	return clamp(uv, vec2(0.0001), vec2(0.9999));
}

float get_height(vec2 uv) {
  return texture(node2dTextures[ubo.texture_id], uv).r;
}


void main() {
    vec2 sample_step;
    if(ubo.match_size) {
        sample_step = (1.0 / texture_size) * ubo.max_range;
    } else {
        sample_step = (1.0 / vec2(1024.0)) * ubo.max_range;
    }
    float strength = -0.1 * ubo.strength;
    float height = get_height(fragUV);
    vec2 dxy = height - vec2(get_height(fragUV + vec2(sample_step.x, 0.)), get_height(fragUV + vec2(0., sample_step.y)));
    vec3 normal = normalize(vec3(dxy * strength / sample_step, 1.0));
	outColor = vec4(normal * 0.5 + 0.5, 1.0);
}