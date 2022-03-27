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

layout(set = 0, binding = 3) uniform UniformBufferObject {
    vec4 base_color;
    int base_color_texture_id;
    float metallic;
    int metallic_texture_id;
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

vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), vec3(0.0), vec3(1.0));
}

float pow5(float x){
    return x * x * x * x * x;
}

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness){
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow5(1.0 - cosTheta);
}

void main() {
    vec3 base_color;
    if(ubo.base_color_texture_id >= 0) {
        base_color = texture(textureArray[ubo.base_color_texture_id], fragTexCoord).rgb;
    } else {
        base_color = ubo.base_color.rgb;
    }
    float metallic;
    if(ubo.metallic_texture_id >= 0) {
        metallic = texture(textureArray[ubo.metallic_texture_id], fragTexCoord).r;
    } else {
        metallic = ubo.metallic;
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
        normal = normalize(fragNormal);
    }

    vec3 wo = normalize(cam_ubo.pos - fragPos.xyz);
    float NdotWo = max(dot(normal, wo), 0.0);

    vec3 F0 = mix(vec3(0.16 * 0.5 * 0.5), base_color, metallic);
	vec3 F = F_SchlickR(NdotWo, F0, roughness);

    vec3 diffuse = base_color * (1.0 - metallic) * (1.0 - F) * texture(cubemapArray[ubo.irradiance_map_id], normal).xyz;

    vec3 r = reflect(-wo , normal);
    vec3 specular1 = textureLod(cubemapArray[ubo.prefiltered_map_id], r, roughness * MAX_REFLECTION_LOD).xyz;

    vec2 AB = texture(textureArray[ubo.brdf_LUT_id], vec2(max(dot(wo, normal), 0.0), roughness)).xy;
    vec3 specular2 = AB.x * F0 + AB.y;

    vec3 color = diffuse + specular1 * specular2;
  
    color = ACESFilm(color);

    // Tone mapping
	//color = Uncharted2Tonemap(color);
	//color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
    //color = pow(color, vec3(1.0f / 2.2f));

    outColor = vec4(color, 1.0);
}