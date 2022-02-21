#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject {
    float radius;
    float angle;
    int sides;
    float gradient;
} ubo;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

#define PI 3.14159265359
#define TWO_PI 6.28318530718

float linearstep(float a, float b, float t) {
    if(t <= a) {
        return 0.0;
    }
    if(t >= b) {
        return 1.0;
    }
    return (t - a) / (b - a);
}

void main() {
    vec2 uv = fragUV * 2. - 1.;
    float a = atan(uv.x, uv.y) + radians(ubo.angle);
    float r = TWO_PI / float(ubo.sides);
    float d = cos(floor(.5 + a / r) * r - a) * length(uv) / ubo.radius;
    vec3 color = vec3(1.0 - linearstep(0.8 - ubo.gradient, 0.8, d));
    outColor = vec4(color, 1.0);
}