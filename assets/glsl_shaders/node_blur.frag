#version 460
#extension GL_EXT_nonuniform_qualifier:enable

layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    int blur_texture_id;
    float intensity;
    int intensity_texture_id;
    int samples;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D nodeTextures[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

#define pow2(x) (x * x)

#define M_PI 3.14159265358979323846

float gaussian(vec2 i, float sigma) {
    return 1.0 / (2.0 * M_PI * pow2(sigma)) * exp(-((pow2(i.x) + pow2(i.y)) / (2.0 * pow2(sigma))));
}

void main() {
    if(ubo.blur_texture_id < 0) {
        outColor = vec4(0, 0, 0, 1.0);
        return;
    }

    vec3 col = vec3(0.0);
    const vec2 ps = vec2(1.0, 1.0) / textureSize(nodeTextures[ubo.blur_texture_id], 0);

    float intensity;
    if(ubo.intensity_texture_id < 0) {
        intensity = ubo.intensity;
    } else {
        intensity = texture(nodeTextures[ubo.intensity_texture_id], fragUV).r;
    }

    const vec2 scale = ps * intensity;
    float accum = 0.0;
    float weight;
    vec2 offset;
    const float sigma = float(ubo.samples) * 0.25;

    for(int x = -ubo.samples / 2; x < ubo.samples / 2; ++x) {
        for(int y = -ubo.samples / 2; y < ubo.samples / 2; ++y) {
            offset = vec2(x, y);
            weight = gaussian(offset, sigma);
            col += texture(nodeTextures[ubo.blur_texture_id], fragUV + scale * offset).rgb * weight;
            accum += weight;
        }
    }
    outColor = vec4(col / accum, 1.0);
}
