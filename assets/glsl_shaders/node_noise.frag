//From Blender: https://github.com/blender/blender/blob/master/source/blender/gpu/shaders/material/gpu_shader_material_tex_noise.glsl
#version 460
#include "include/hash.glsl"
#include "include/noise.glsl"
#include "include/fractal_noise.glsl"

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
  int format;
  int dimension;
  float x;
  float y;
  float z;
  float scale;
  float detail;
  float roughness;
  float distortion;
} ubo;

vec3 texcoord = vec3(fragUV.x + ubo.x, fragUV.y + ubo.y, ubo.z);

float random_float_offset(float seed) {
  return 100.0 + hash_float_to_float(seed) * 100.0;
}

vec2 random_vec2_offset(float seed) {
  return vec2(100.0 + hash_vec2_to_float(vec2(seed, 0.0)) * 100.0, 100.0 + hash_vec2_to_float(vec2(seed, 1.0)) * 100.0);
}

vec3 random_vec3_offset(float seed) {
  return vec3(100.0 + hash_vec2_to_float(vec2(seed, 0.0)) * 100.0, 100.0 + hash_vec2_to_float(vec2(seed, 1.0)) * 100.0, 100.0 + hash_vec2_to_float(vec2(seed, 2.0)) * 100.0);
}

vec4 random_vec4_offset(float seed) {
  return vec4(100.0 + hash_vec2_to_float(vec2(seed, 0.0)) * 100.0, 100.0 + hash_vec2_to_float(vec2(seed, 1.0)) * 100.0, 100.0 + hash_vec2_to_float(vec2(seed, 2.0)) * 100.0, 100.0 + hash_vec2_to_float(vec2(seed, 3.0)) * 100.0);
}

void node_noise_texture_1d(float w, float scale, float detail, float roughness, float distortion, out float value) {
  float p = w * scale;
  if(distortion != 0.0) {
    p += snoise(p + random_float_offset(0.0)) * distortion;
  }

  value = fractal_noise(p, detail, roughness);
}

void node_noise_texture_1d_color(float w, float scale, float detail, float roughness, float distortion, out vec4 color) {
  float p = w * scale;
  if(distortion != 0.0) {
    p += snoise(p + random_float_offset(0.0)) * distortion;
  }
  color = vec4(fractal_noise(p, detail, roughness), fractal_noise(p + random_float_offset(1.0), detail, roughness), fractal_noise(p + random_float_offset(2.0), detail, roughness), 1.0);
}

void node_noise_texture_2d(vec3 co, float scale, float detail, float roughness, float distortion, out float value) {
  vec2 p = co.xy * scale;
  if(distortion != 0.0) {
    p += vec2(snoise(p + random_vec2_offset(0.0)) * distortion, snoise(p + random_vec2_offset(1.0)) * distortion);
  }

  value = fractal_noise(p, detail, roughness);
}

void node_noise_texture_2d_color(vec3 co, float scale, float detail, float roughness, float distortion, out vec4 color) {
  vec2 p = co.xy * scale;
  if(distortion != 0.0) {
    p += vec2(snoise(p + random_vec2_offset(0.0)) * distortion, snoise(p + random_vec2_offset(1.0)) * distortion);
  }

  color = vec4(fractal_noise(p, detail, roughness), fractal_noise(p + random_vec2_offset(2.0), detail, roughness), fractal_noise(p + random_vec2_offset(3.0), detail, roughness), 1.0);
}

void node_noise_texture_3d(vec3 co, float scale, float detail, float roughness, float distortion, out float value) {
  vec3 p = co * scale;
  if(distortion != 0.0) {
    p += vec3(snoise(p + random_vec3_offset(0.0)) * distortion, snoise(p + random_vec3_offset(1.0)) * distortion, snoise(p + random_vec3_offset(2.0)) * distortion);
  }

  value = fractal_noise(p, detail, roughness);

}

void node_noise_texture_3d_color(vec3 co, float scale, float detail, float roughness, float distortion, out vec4 color) {
  vec3 p = co * scale;
  if(distortion != 0.0) {
    p += vec3(snoise(p + random_vec3_offset(0.0)) * distortion, snoise(p + random_vec3_offset(1.0)) * distortion, snoise(p + random_vec3_offset(2.0)) * distortion);
  }

  color = vec4(fractal_noise(p, detail, roughness), fractal_noise(p + random_vec3_offset(3.0), detail, roughness), fractal_noise(p + random_vec3_offset(4.0), detail, roughness), 1.0);
}

void node_noise_texture_4d(vec3 co, float w, float scale, float detail, float roughness, float distortion, out float value) {
  vec4 p = vec4(co, w) * scale;
  if(distortion != 0.0) {
    p += vec4(snoise(p + random_vec4_offset(0.0)) * distortion, snoise(p + random_vec4_offset(1.0)) * distortion, snoise(p + random_vec4_offset(2.0)) * distortion, snoise(p + random_vec4_offset(3.0)) * distortion);
  }

  value = fractal_noise(p, detail, roughness);

}

void node_noise_texture_4d_color(vec3 co, float w, float scale, float detail, float roughness, float distortion, out vec4 color) {
  vec4 p = vec4(co, w) * scale;
  if(distortion != 0.0) {
    p += vec4(snoise(p + random_vec4_offset(0.0)) * distortion, snoise(p + random_vec4_offset(1.0)) * distortion, snoise(p + random_vec4_offset(2.0)) * distortion, snoise(p + random_vec4_offset(3.0)) * distortion);
  }

  color = vec4(fractal_noise(p, detail, roughness), fractal_noise(p + random_vec4_offset(4.0), detail, roughness), fractal_noise(p + random_vec4_offset(5.0), detail, roughness), 1.0);
}

void main() {
  float grey_scale = 1.0;
  float color;
  float scale = ubo.scale * 5.0f;

  if(ubo.format == 0) {
    if(ubo.dimension == 0) {
      node_noise_texture_1d_color(texcoord.x, scale, ubo.detail, ubo.roughness, ubo.distortion, outColor);
    } else if(ubo.dimension == 1) {
      node_noise_texture_2d_color(texcoord, scale, ubo.detail, ubo.roughness, ubo.distortion, outColor);
    } else if(ubo.dimension == 2) {
      node_noise_texture_3d_color(texcoord, scale, ubo.detail, ubo.roughness, ubo.distortion, outColor);
    }
  } else {
    float grey_scale = 1.0;
    if(ubo.dimension == 0) {
      node_noise_texture_1d(texcoord.x, scale, ubo.detail, ubo.roughness, ubo.distortion, grey_scale);
    } else if(ubo.dimension == 1) {
      node_noise_texture_2d(texcoord, scale, ubo.detail, ubo.roughness, ubo.distortion, grey_scale);
    } else if(ubo.dimension == 2) {
      node_noise_texture_3d(texcoord, scale, ubo.detail, ubo.roughness, ubo.distortion, grey_scale);
    }
    outColor = vec4(vec3(grey_scale), 1.0);
  }
}