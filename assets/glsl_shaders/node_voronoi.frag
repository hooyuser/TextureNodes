#version 460
#include "include/hash.glsl"

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
  int dimension;
  int method;
  int metric;
  float x;
  float y;
  float z;
  float scale;
  float randomness;
  float smoothness;
  float exponent;
} ubo;

vec3 texcoord = vec3(fragUV.x + ubo.x, fragUV.y + ubo.y, ubo.z);

/* #define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

#define mix3(a, b, c) \
  { \
    a -= c; \
    a ^= rot(c, 4); \
    c += b; \
    b -= a; \
    b ^= rot(a, 6); \
    a += c; \
    c -= b; \
    c ^= rot(b, 8); \
    b += a; \
    a -= c; \
    a ^= rot(c, 16); \
    c += b; \
    b -= a; \
    b ^= rot(a, 19); \
    a += c; \
    c -= b; \
    c ^= rot(b, 4); \
    b += a; \
  }

#define final(a, b, c) \
  { \
    c ^= b; \
    c -= rot(b, 14); \
    a ^= c; \
    a -= rot(c, 11); \
    b ^= a; \
    b -= rot(a, 25); \
    c ^= b; \
    c -= rot(b, 16); \
    a ^= c; \
    a -= rot(c, 4); \
    b ^= a; \
    b -= rot(a, 14); \
    c ^= b; \
    c -= rot(b, 24); \
  }

uint hash_uint2(uint kx, uint ky)
{
  uint a, b, c;
  a = b = c = 0xdeadbeefu + (2u << 2u) + 13u;
  b += ky;
  a += kx;
  final(a, b, c);
  return c;
}

uint hash_uint3(uint kx, uint ky, uint kz)
{
  uint a, b, c;
  a = b = c = 0xdeadbeefu + (3u << 2u) + 13u;
  c += kz;
  b += ky;
  a += kx;
  final(a, b, c);
  return c;
}

uint hash_uint4(uint kx, uint ky, uint kz, uint kw)
{
  uint a, b, c;
  a = b = c = 0xdeadbeefu + (4u << 2u) + 13u;
  a += kx;
  b += ky;
  c += kz;
  mix3(a, b, c);
  a += kw;
  final(a, b, c);
  return c;
}

float hash_uint2_to_float(uint kx, uint ky) {
  return float(hash_uint2(kx, ky)) / float(0xFFFFFFFFu);
}

float hash_uint3_to_float(uint kx, uint ky, uint kz) {
  return float(hash_uint3(kx, ky, kz)) / float(0xFFFFFFFFu);
}

float hash_uint4_to_float(uint kx, uint ky, uint kz, uint kw) {
  return float(hash_uint4(kx, ky, kz, kw)) / float(0xFFFFFFFFu);
}

float hash_vec2_to_float(vec2 k) {
  return hash_uint2_to_float(floatBitsToUint(k.x), floatBitsToUint(k.y));
}

float hash_vec3_to_float(vec3 k) {
  return hash_uint3_to_float(floatBitsToUint(k.x), floatBitsToUint(k.y), floatBitsToUint(k.z));
}

float hash_vec4_to_float(vec4 k) {
  return hash_uint4_to_float(floatBitsToUint(k.x), floatBitsToUint(k.y), floatBitsToUint(k.z), floatBitsToUint(k.w));
}

vec2 hash_vec2_to_vec2(vec2 k) {
  return vec2(hash_vec2_to_float(k), hash_vec3_to_float(vec3(k, 1.0)));
}

vec3 hash_vec3_to_vec3(vec3 k) {
  return vec3(hash_vec3_to_float(k), hash_vec4_to_float(vec4(k, 1.0)), hash_vec4_to_float(vec4(k, 2.0)));
}

vec3 hash_vec2_to_vec3(vec2 k) {
  return vec3(hash_vec2_to_float(k), hash_vec3_to_float(vec3(k, 1.0)), hash_vec3_to_float(vec3(k, 2.0)));
} */

float voronoi_distance(vec2 a, vec2 b, int metric, float exponent) {
  if(metric == 0) {// SHD_VORONOI_EUCLIDEAN
    return distance(a, b);
  } 
  else if(metric == 1) {  // SHD_VORONOI_MANHATTAN
    return abs(a.x - b.x) + abs(a.y - b.y);
  }
  else if(metric == 2) {  // SHD_VORONOI_CHEBYCHEV
    return max(abs(a.x - b.x), abs(a.y - b.y));
  } 
  else if(metric == 3) {  // SHD_VORONOI_MINKOWSKI
    return pow(pow(abs(a.x - b.x), exponent) + pow(abs(a.y - b.y), exponent), 1.0 / exponent);
  }
  else {
    return 0.0;
  }
}

void node_tex_voronoi_f1_2d(vec3 coord, float scale, float exponent, float randomness, int metric, out float outDistance) {
  randomness = clamp(randomness, 0.0, 1.0);

  vec2 scaledCoord = coord.xy * scale;
  vec2 cellPosition = floor(scaledCoord);
  vec2 localPosition = scaledCoord - cellPosition;

  float minDistance = 8.0;
  vec2 targetOffset, targetPosition;
  for(int j = -1; j <= 1; j++) {
    for(int i = -1; i <= 1; i++) {
      vec2 cellOffset = vec2(i, j);
      vec2 pointPosition = cellOffset + hash_vec2_to_vec2(cellPosition + cellOffset) * randomness;
      float distanceToPoint = voronoi_distance(pointPosition, localPosition, metric, exponent);
      if(distanceToPoint < minDistance) {
        targetOffset = cellOffset;
        minDistance = distanceToPoint;
        targetPosition = pointPosition;
      }
    }
  }
  outDistance = minDistance;
}

void node_tex_voronoi_smooth_f1_2d(vec3 coord, float scale, float smoothness, float exponent, float randomness, int metric, out float outDistance) {
  randomness = clamp(randomness, 0.0, 1.0);
  smoothness = clamp(smoothness / 2.0, 0.0, 0.5);

  vec2 scaledCoord = coord.xy * scale;
  vec2 cellPosition = floor(scaledCoord);
  vec2 localPosition = scaledCoord - cellPosition;

  float smoothDistance = 8.0;
  vec3 smoothColor = vec3(0.0);
  vec2 smoothPosition = vec2(0.0);
  for(int j = -2; j <= 2; j++) {
    for(int i = -2; i <= 2; i++) {
      vec2 cellOffset = vec2(i, j);
      vec2 pointPosition = cellOffset + hash_vec2_to_vec2(cellPosition + cellOffset) * randomness;
      float distanceToPoint = voronoi_distance(pointPosition, localPosition, metric, exponent);
      float h = smoothstep(0.0, 1.0, 0.5 + 0.5 * (smoothDistance - distanceToPoint) / smoothness);
      float correctionFactor = smoothness * h * (1.0 - h);
      smoothDistance = mix(smoothDistance, distanceToPoint, h) - correctionFactor;
      correctionFactor /= 1.0 + 3.0 * smoothness;
      vec3 cellColor = hash_vec2_to_vec3(cellPosition + cellOffset);
      smoothColor = mix(smoothColor, cellColor, h) - correctionFactor;
      smoothPosition = mix(smoothPosition, pointPosition, h) - correctionFactor;
    }
  }
  outDistance = smoothDistance;
}

void node_tex_voronoi_f2_2d(vec3 coord, float scale, float exponent, float randomness, int metric, out float outDistance) {
  randomness = clamp(randomness, 0.0, 1.0);

  vec2 scaledCoord = coord.xy * scale;
  vec2 cellPosition = floor(scaledCoord);
  vec2 localPosition = scaledCoord - cellPosition;

  float distanceF1 = 8.0;
  float distanceF2 = 8.0;
  vec2 offsetF1 = vec2(0.0);
  vec2 positionF1 = vec2(0.0);
  vec2 offsetF2, positionF2;
  for(int j = -1; j <= 1; j++) {
    for(int i = -1; i <= 1; i++) {
      vec2 cellOffset = vec2(i, j);
      vec2 pointPosition = cellOffset + hash_vec2_to_vec2(cellPosition + cellOffset) * randomness;
      float distanceToPoint = voronoi_distance(pointPosition, localPosition, metric, exponent);
      if(distanceToPoint < distanceF1) {
        distanceF2 = distanceF1;
        distanceF1 = distanceToPoint;
        offsetF2 = offsetF1;
        offsetF1 = cellOffset;
        positionF2 = positionF1;
        positionF1 = pointPosition;
      } 
      else if(distanceToPoint < distanceF2) {
        distanceF2 = distanceToPoint;
        offsetF2 = cellOffset;
        positionF2 = pointPosition;
      }
    }
  }
  outDistance = distanceF2;
}

void node_tex_voronoi_distance_to_edge_2d(vec3 coord, float scale, float randomness, out float outDistance) {
  randomness = clamp(randomness, 0.0, 1.0);

  vec2 scaledCoord = coord.xy * scale;
  vec2 cellPosition = floor(scaledCoord);
  vec2 localPosition = scaledCoord - cellPosition;

  vec2 vectorToClosest;
  float minDistance = 8.0;
  for(int j = -1; j <= 1; j++) {
    for(int i = -1; i <= 1; i++) {
      vec2 cellOffset = vec2(i, j);
      vec2 vectorToPoint = cellOffset + hash_vec2_to_vec2(cellPosition + cellOffset) * randomness -
        localPosition;
      float distanceToPoint = dot(vectorToPoint, vectorToPoint);
      if(distanceToPoint < minDistance) {
        minDistance = distanceToPoint;
        vectorToClosest = vectorToPoint;
      }
    }
  }

  minDistance = 8.0;
  for(int j = -1; j <= 1; j++) {
    for(int i = -1; i <= 1; i++) {
      vec2 cellOffset = vec2(i, j);
      vec2 vectorToPoint = cellOffset + hash_vec2_to_vec2(cellPosition + cellOffset) * randomness -
        localPosition;
      vec2 perpendicularToEdge = vectorToPoint - vectorToClosest;
      if(dot(perpendicularToEdge, perpendicularToEdge) > 0.0001) {
        float distanceToEdge = dot((vectorToClosest + vectorToPoint) / 2.0, normalize(perpendicularToEdge));
        minDistance = min(minDistance, distanceToEdge);
      }
    }
  }
  outDistance = minDistance;
}

void node_tex_voronoi_n_sphere_radius_2d(vec3 coord, float scale, float randomness, out float outRadius) {
  randomness = clamp(randomness, 0.0, 1.0);

  vec2 scaledCoord = coord.xy * scale;
  vec2 cellPosition = floor(scaledCoord);
  vec2 localPosition = scaledCoord - cellPosition;

  vec2 closestPoint;
  vec2 closestPointOffset;
  float minDistance = 8.0;
  for(int j = -1; j <= 1; j++) {
    for(int i = -1; i <= 1; i++) {
      vec2 cellOffset = vec2(i, j);
      vec2 pointPosition = cellOffset + hash_vec2_to_vec2(cellPosition + cellOffset) * randomness;
      float distanceToPoint = distance(pointPosition, localPosition);
      if(distanceToPoint < minDistance) {
        minDistance = distanceToPoint;
        closestPoint = pointPosition;
        closestPointOffset = cellOffset;
      }
    }
  }

  minDistance = 8.0;
  vec2 closestPointToClosestPoint;
  for(int j = -1; j <= 1; j++) {
    for(int i = -1; i <= 1; i++) {
      if(i == 0 && j == 0) {
        continue;
      }
      vec2 cellOffset = vec2(i, j) + closestPointOffset;
      vec2 pointPosition = cellOffset + hash_vec2_to_vec2(cellPosition + cellOffset) * randomness;
      float distanceToPoint = distance(closestPoint, pointPosition);
      if(distanceToPoint < minDistance) {
        minDistance = distanceToPoint;
        closestPointToClosestPoint = pointPosition;
      }
    }
  }
  outRadius = distance(closestPointToClosestPoint, closestPoint) / 2.0;
}

/* **** 3D Voronoi **** */

float voronoi_distance(vec3 a, vec3 b, float metric, float exponent) {
  if(metric == 0.0)  // SHD_VORONOI_EUCLIDEAN
  {
    return distance(a, b);
  } else if(metric == 1.0)  // SHD_VORONOI_MANHATTAN
  {
    return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
  } else if(metric == 2.0)  // SHD_VORONOI_CHEBYCHEV
  {
    return max(abs(a.x - b.x), max(abs(a.y - b.y), abs(a.z - b.z)));
  } else if(metric == 3.0)  // SHD_VORONOI_MINKOWSKI
  {
    return pow(pow(abs(a.x - b.x), exponent) + pow(abs(a.y - b.y), exponent) +
      pow(abs(a.z - b.z), exponent), 1.0 / exponent);
  } else {
    return 0.0;
  }
}

void node_tex_voronoi_f1_3d(vec3 coord, float scale, float exponent, float randomness, float metric, out float outDistance) {
  randomness = clamp(randomness, 0.0, 1.0);

  vec3 scaledCoord = coord * scale;
  vec3 cellPosition = floor(scaledCoord);
  vec3 localPosition = scaledCoord - cellPosition;

  float minDistance = 8.0;
  vec3 targetOffset, targetPosition;
  for(int k = -1; k <= 1; k++) {
    for(int j = -1; j <= 1; j++) {
      for(int i = -1; i <= 1; i++) {
        vec3 cellOffset = vec3(i, j, k);
        vec3 pointPosition = cellOffset +
          hash_vec3_to_vec3(cellPosition + cellOffset) * randomness;
        float distanceToPoint = voronoi_distance(pointPosition, localPosition, metric, exponent);
        if(distanceToPoint < minDistance) {
          targetOffset = cellOffset;
          minDistance = distanceToPoint;
          targetPosition = pointPosition;
        }
      }
    }
  }
  outDistance = minDistance;

}

void node_tex_voronoi_smooth_f1_3d(vec3 coord, float scale, float smoothness, float exponent, float randomness, float metric, out float outDistance) {
  randomness = clamp(randomness, 0.0, 1.0);
  smoothness = clamp(smoothness / 2.0, 0.0, 0.5);

  vec3 scaledCoord = coord * scale;
  vec3 cellPosition = floor(scaledCoord);
  vec3 localPosition = scaledCoord - cellPosition;

  float smoothDistance = 8.0;
  vec3 smoothColor = vec3(0.0);
  vec3 smoothPosition = vec3(0.0);
  for(int k = -2; k <= 2; k++) {
    for(int j = -2; j <= 2; j++) {
      for(int i = -2; i <= 2; i++) {
        vec3 cellOffset = vec3(i, j, k);
        vec3 pointPosition = cellOffset +
          hash_vec3_to_vec3(cellPosition + cellOffset) * randomness;
        float distanceToPoint = voronoi_distance(pointPosition, localPosition, metric, exponent);
        float h = smoothstep(0.0, 1.0, 0.5 + 0.5 * (smoothDistance - distanceToPoint) / smoothness);
        float correctionFactor = smoothness * h * (1.0 - h);
        smoothDistance = mix(smoothDistance, distanceToPoint, h) - correctionFactor;
        correctionFactor /= 1.0 + 3.0 * smoothness;
        vec3 cellColor = hash_vec3_to_vec3(cellPosition + cellOffset);
        smoothColor = mix(smoothColor, cellColor, h) - correctionFactor;
        smoothPosition = mix(smoothPosition, pointPosition, h) - correctionFactor;
      }
    }
  }
  outDistance = smoothDistance;
}

void node_tex_voronoi_f2_3d(vec3 coord, float scale, float exponent, float randomness, float metric, out float outDistance) {
  randomness = clamp(randomness, 0.0, 1.0);

  vec3 scaledCoord = coord * scale;
  vec3 cellPosition = floor(scaledCoord);
  vec3 localPosition = scaledCoord - cellPosition;

  float distanceF1 = 8.0;
  float distanceF2 = 8.0;
  vec3 offsetF1 = vec3(0.0);
  vec3 positionF1 = vec3(0.0);
  vec3 offsetF2, positionF2;
  for(int k = -1; k <= 1; k++) {
    for(int j = -1; j <= 1; j++) {
      for(int i = -1; i <= 1; i++) {
        vec3 cellOffset = vec3(i, j, k);
        vec3 pointPosition = cellOffset +
          hash_vec3_to_vec3(cellPosition + cellOffset) * randomness;
        float distanceToPoint = voronoi_distance(pointPosition, localPosition, metric, exponent);
        if(distanceToPoint < distanceF1) {
          distanceF2 = distanceF1;
          distanceF1 = distanceToPoint;
          offsetF2 = offsetF1;
          offsetF1 = cellOffset;
          positionF2 = positionF1;
          positionF1 = pointPosition;
        } else if(distanceToPoint < distanceF2) {
          distanceF2 = distanceToPoint;
          offsetF2 = cellOffset;
          positionF2 = pointPosition;
        }
      }
    }
  }
  outDistance = distanceF2;
}

void node_tex_voronoi_distance_to_edge_3d(vec3 coord, float scale, float randomness, out float outDistance) {
  randomness = clamp(randomness, 0.0, 1.0);

  vec3 scaledCoord = coord * scale;
  vec3 cellPosition = floor(scaledCoord);
  vec3 localPosition = scaledCoord - cellPosition;

  vec3 vectorToClosest;
  float minDistance = 8.0;
  for(int k = -1; k <= 1; k++) {
    for(int j = -1; j <= 1; j++) {
      for(int i = -1; i <= 1; i++) {
        vec3 cellOffset = vec3(i, j, k);
        vec3 vectorToPoint = cellOffset +
          hash_vec3_to_vec3(cellPosition + cellOffset) * randomness -
          localPosition;
        float distanceToPoint = dot(vectorToPoint, vectorToPoint);
        if(distanceToPoint < minDistance) {
          minDistance = distanceToPoint;
          vectorToClosest = vectorToPoint;
        }
      }
    }
  }

  minDistance = 8.0;
  for(int k = -1; k <= 1; k++) {
    for(int j = -1; j <= 1; j++) {
      for(int i = -1; i <= 1; i++) {
        vec3 cellOffset = vec3(i, j, k);
        vec3 vectorToPoint = cellOffset +
          hash_vec3_to_vec3(cellPosition + cellOffset) * randomness -
          localPosition;
        vec3 perpendicularToEdge = vectorToPoint - vectorToClosest;
        if(dot(perpendicularToEdge, perpendicularToEdge) > 0.0001) {
          float distanceToEdge = dot((vectorToClosest + vectorToPoint) / 2.0, normalize(perpendicularToEdge));
          minDistance = min(minDistance, distanceToEdge);
        }
      }
    }
  }
  outDistance = minDistance;
}

void node_tex_voronoi_n_sphere_radius_3d(vec3 coord, float scale, float randomness, out float outRadius) {
  randomness = clamp(randomness, 0.0, 1.0);

  vec3 scaledCoord = coord * scale;
  vec3 cellPosition = floor(scaledCoord);
  vec3 localPosition = scaledCoord - cellPosition;

  vec3 closestPoint;
  vec3 closestPointOffset;
  float minDistance = 8.0;
  for(int k = -1; k <= 1; k++) {
    for(int j = -1; j <= 1; j++) {
      for(int i = -1; i <= 1; i++) {
        vec3 cellOffset = vec3(i, j, k);
        vec3 pointPosition = cellOffset +
          hash_vec3_to_vec3(cellPosition + cellOffset) * randomness;
        float distanceToPoint = distance(pointPosition, localPosition);
        if(distanceToPoint < minDistance) {
          minDistance = distanceToPoint;
          closestPoint = pointPosition;
          closestPointOffset = cellOffset;
        }
      }
    }
  }

  minDistance = 8.0;
  vec3 closestPointToClosestPoint;
  for(int k = -1; k <= 1; k++) {
    for(int j = -1; j <= 1; j++) {
      for(int i = -1; i <= 1; i++) {
        if(i == 0 && j == 0 && k == 0) {
          continue;
        }
        vec3 cellOffset = vec3(i, j, k) + closestPointOffset;
        vec3 pointPosition = cellOffset +
          hash_vec3_to_vec3(cellPosition + cellOffset) * randomness;
        float distanceToPoint = distance(closestPoint, pointPosition);
        if(distanceToPoint < minDistance) {
          minDistance = distanceToPoint;
          closestPointToClosestPoint = pointPosition;
        }
      }
    }
  }
  outRadius = distance(closestPointToClosestPoint, closestPoint) / 2.0;
}

void main() {
  float result = 1.0;
  float scale = ubo.scale * 8.0f;
  if(ubo.dimension == 0) {
    if(ubo.method == 0) {
      node_tex_voronoi_f1_2d(texcoord, scale, ubo.exponent, ubo.randomness, ubo.metric, result);
    } 
    else if(ubo.method == 1) {
      node_tex_voronoi_smooth_f1_2d(texcoord, scale, ubo.smoothness, ubo.exponent, ubo.randomness, ubo.metric, result);
    } 
    else if(ubo.method == 2) {
      node_tex_voronoi_f2_2d(texcoord, scale, ubo.exponent, ubo.randomness, ubo.metric, result);
    }
    else if(ubo.method == 3) {
      node_tex_voronoi_distance_to_edge_2d(texcoord, scale, ubo.randomness, result);
    }
    else if(ubo.method == 4) {
      node_tex_voronoi_n_sphere_radius_2d(texcoord, scale, ubo.randomness, result);
    }
  } else if(ubo.dimension == 1) {
    if(ubo.method == 0) {
      node_tex_voronoi_f1_3d(texcoord, scale, ubo.exponent, ubo.randomness, ubo.metric, result);
    }
    else if(ubo.method == 1) {
      node_tex_voronoi_smooth_f1_3d(texcoord, scale, ubo.smoothness, ubo.exponent, ubo.randomness, ubo.metric, result);
    } 
    else if(ubo.method == 2) {
      node_tex_voronoi_f2_3d(texcoord, scale, ubo.exponent, ubo.randomness, ubo.metric, result);
    }
    else if(ubo.method == 3) {
      node_tex_voronoi_distance_to_edge_3d(texcoord, scale, ubo.randomness, result);
    }
    else if(ubo.method == 4) {
      node_tex_voronoi_n_sphere_radius_3d(texcoord, scale, ubo.randomness, result);
    }
  }
  outColor = vec4(vec3(result), 1.0);
}