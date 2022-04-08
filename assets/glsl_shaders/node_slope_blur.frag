#version 460
#extension GL_EXT_nonuniform_qualifier:enable

layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    int blur_texture_id;
    int slope_texture_id;
    float intensity;
    int intensity_texture_id;
    float quality;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D nodeTextures[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

#define pow2(x) (x * x)

#define M_PI 3.14159265358979323846

vec2 calcSlope(vec2 uv) {
    const vec2 textureSize = textureSize(nodeTextures[ubo.slope_texture_id], 0);
    vec3 sl = vec3(0.0, 0.0, 0.0);
    sl.x = texture(nodeTextures[ubo.slope_texture_id], uv + vec2(0.0, 1.0 / textureSize.y)).r;
    sl.y = texture(nodeTextures[ubo.slope_texture_id], uv + vec2(-1.0 / textureSize.x, -0.5 / textureSize.y)).r;
    sl.z = texture(nodeTextures[ubo.slope_texture_id], uv + vec2(1.0 / textureSize.x, -0.5 / textureSize.y)).r;

    vec2 result = vec2(0.0);
    result.x = sl.z - sl.y;
    result.y = dot(sl, vec3(1, -0.5, -0.5));

    return result;
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

    int iterations = int(ceil(ubo.quality * intensity));

    vec3 color = vec3(0.0);
    vec2 uv = fragUV;
    const vec2 blurTextureSize = textureSize(nodeTextures[ubo.blur_texture_id], 0);
    for(int i = 0; i < iterations; i++) {
        vec2 slopeValue = calcSlope(uv) * blurTextureSize;
        // note: the uv isnt reset each iteration because the
        // blur should follow the path of the previous slope
        uv += slopeValue * (intensity / blurTextureSize.x) * (intensity / float(iterations));
        color += texture(nodeTextures[ubo.blur_texture_id], uv).rgb;
    }

    outColor = vec4(color / float(iterations), 1.0);
}
