#version 460
#extension GL_EXT_nonuniform_qualifier:enable

#define PI 3.14159265359
#define MAX_REFLECTION_LOD 4.0

layout(set = 0, binding = 0) uniform CameraUniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 pos;
} cam_ubo;

layout(set = 0, binding = 1) uniform sampler2D textureArray[];
layout(set = 0, binding = 2) uniform samplerCube cubemapArray[];

layout(set = 1, binding = 0) uniform UniformBufferObject {
    vec4 base_color;
    int base_color_texture_id;
    float metalness;
    int metalness_texture_id;
    float roughness;
    int roughness_texture_id;
    int normal_texture_id;
    int irradiance_map_id; 
    int brdf_LUT_id;    
    int prefiltered_map_id;
} ubo;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
	float lod = 1.01 + roughness * (MAX_REFLECTION_LOD-1.0);
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(cubemapArray[ubo.prefiltered_map_id], R, lodf).rgb;
	vec3 b = textureLod(cubemapArray[ubo.prefiltered_map_id], R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

void main() {
    vec3 base_color;
    if(ubo.base_color_texture_id >= 0) {
        base_color = texture(textureArray[ubo.base_color_texture_id], fragTexCoord).rgb;
    } else {
        base_color = ubo.base_color.rgb;
    }
    float metalness;
    if(ubo.metalness_texture_id >= 0) {
        metalness = texture(textureArray[ubo.metalness_texture_id], fragTexCoord).r;
    } else {
        metalness = ubo.metalness;
    }
    float roughness;
    if(ubo.roughness_texture_id >= 0) {
        roughness = texture(textureArray[ubo.roughness_texture_id], fragTexCoord).r;
    } else {
        roughness = ubo.roughness;
    }
    vec3 normal;
    if(ubo.normal_texture_id >= 0) {
        normal = texture(textureArray[ubo.normal_texture_id], fragTexCoord).rgb;
    } else {
        normal = fragNormal;
    }

    vec3 F0 = mix(vec3(0.16 * 0.5 * 0.5), base_color, metalness);
    vec3 F = max(vec3(1.0) - roughness, F0);
    vec3 diffuse = base_color * (1.0 - metalness) * F * texture(cubemapArray[ubo.irradiance_map_id], normal).xyz / PI;

    float alpha = roughness * roughness;
    vec3 wo = normalize(cam_ubo.pos - fragPos.xyz);

    vec3 r = reflect(-wo , normal);

    //vec3 specular1 = prefilteredReflection(r, roughness);
    vec3 specular1 = textureLod(cubemapArray[ubo.prefiltered_map_id], r, roughness * MAX_REFLECTION_LOD).xyz;

    vec2 AB = texture(textureArray[ubo.brdf_LUT_id], vec2(max(dot(wo, normal), 0.0), roughness)).xy;
    vec3 specular2 = AB.x * F0 + AB.y;

    vec3 color = diffuse + specular1 * specular2;
    //vec3 color = baseColor;

    // Tone mapping
	//color = Uncharted2Tonemap(color);
	//color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
    //color = pow(color, vec3(1.0f / 2.2f));

    outColor = vec4(color, 1.0);


    //outColor = vec4(diffuse, 1.0);
    //outColor = vec4(specular1 * specular2, 1.0);
    //outColor = vec4(specular2, 1.0);
    //outColor = vec4(specular1, 1.0);
}