#version 460
#extension GL_EXT_nonuniform_qualifier:enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    uint mode;
    float opacity;
    int foreground;
    int background;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D nodeTextures[];

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

float screen(float fg, float bg) {
    float res = (1. - fg) * (1. - bg);
    return 1. - res;
}

void main() {
    vec4 colA = texture(nodeTextures[ubo.foreground], fragUV);
    vec4 colB = texture(nodeTextures[ubo.background], fragUV);

    float alpha_a = ubo.opacity * colA.a;
    float alpha_b = colB.a;
    float alpha_o = alpha_a + alpha_b - alpha_a * alpha_b;

    vec3 rgb_compute;
    if(ubo.mode == 0) { // normal
        rgb_compute = colA.rgb;
    } else if(ubo.mode == 1) { // add
        rgb_compute = colA.rgb + colB.rgb;
    } else if(ubo.mode == 2) { // subtract
        rgb_compute = colB.rgb - colA.rgb;
    } else if(ubo.mode == 3) { // multiply
        rgb_compute = colB.rgb * colA.rgb;
    } else if(ubo.mode == 4) { // divide
        rgb_compute = colB.rgb / colA.rgb;
    }

    // if (prop_type==4) {// add sub
        //     if (colA.r > 0.5) col.r = colB.r + colA.r; else col.r = colB.r - colA.r;
        //     if (colA.g > 0.5) col.g = colB.g + colA.g; else col.g = colB.g - colA.g;
        //     if (colA.b > 0.5) col.b = colB.b + colA.b; else col.b = colB.b - colA.b;
    // }
    else if(ubo.mode == 5) {// max
        rgb_compute = max(colA.rgb, colB.rgb);
    } else if(ubo.mode == 6) {// min
        rgb_compute = min(colA.rgb, colB.rgb);
    } else if(ubo.mode == 7) {// overlay
        if(colB.r < .5) {
            rgb_compute.r = colB.r * colA.r;
        } else {
            rgb_compute.r = screen(colB.r, colA.r);
        }
        if(colB.g < .5) {
            rgb_compute.g = colB.g * colA.g;
        } else {
            rgb_compute.g = screen(colB.g, colA.g);
        }
        if(colB.b < .5) {
            rgb_compute.b = colB.b * colA.b;
        } else {
            rgb_compute.b = screen(colB.b, colA.b);
        }
    } else if(ubo.mode == 8) {// screen
        rgb_compute.r = screen(colA.r, colB.r);
        rgb_compute.g = screen(colA.g, colB.g);
        rgb_compute.b = screen(colA.b, colB.b);
    }

    vec3 rgb_o = (colA.rgb * alpha_a + colB.rgb * alpha_b - alpha_a * alpha_b * (colA.rgb + colB.rgb - rgb_compute)) / alpha_o;

    outColor = vec4(rgb_o, alpha_o);
}