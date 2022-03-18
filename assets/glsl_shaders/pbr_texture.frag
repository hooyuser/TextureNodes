#version 460
#extension GL_EXT_nonuniform_qualifier:enable

#define PI 3.14159265359
#define MAX_REFLECTION_LOD 4.0

layout(set = 0, binding = 1) uniform UniformBufferObject {
    int baseColor_texture_id,
    int metallic_texture_id,
    int roughness_texture_id,
    int normal_texture_id,
    int irradiance_map_Id, 
    int brdf_LUT_id,     
    int prefiltered_map_id
} ubo;

layout(set = 0, binding = 2) uniform sampler2D textureArray[];
layout(set = 0, binding = 3) uniform samplerCube cubemapArray[];

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
	vec3 a = textureLod(cubemapArray[prefilteredMapId], R, lodf).rgb;
	vec3 b = textureLod(cubemapArray[prefilteredMapId], R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

void main() {
    vec3 baseColor;
    if(baseColorTextureId >= 0) {
        baseColor = texture(textureArray[baseColorTextureId], fragTexCoord).xyz;
    } else {
        baseColor = vec3(baseColorRed, baseColorGreen, baseColorBlue);
    }
    float ao, roughness, metalness;
    if(metallicRoughnessTextureId >= 0) {
        ao = texture(textureArray[metallicRoughnessTextureId], fragTexCoord).x;
        baseColor *= ao;
        roughness = texture(textureArray[metallicRoughnessTextureId], fragTexCoord).y;
        metalness = texture(textureArray[metallicRoughnessTextureId], fragTexCoord).z;
    } else {
        roughness = roughnessFactor;
        metalness = metalnessFactor;
    }
    vec3 F0 = mix(vec3(0.16 * 0.5 * 0.5), baseColor, metalness);
    vec3 F = max(vec3(1.0) - roughness, F0);
    vec3 diffuse = baseColor * (1.0 - metalness) * F * texture(cubemapArray[irradianceMapId], fragNormal).xyz / PI;

    float alpha = roughness * roughness;
    vec3 wo = normalize(ubo.pos - fragPos.xyz);

    vec3 r = reflect(-wo , fragNormal);

    //vec3 specular1 = prefilteredReflection(r, roughness);
    vec3 specular1 = textureLod(cubemapArray[prefilteredMapId], r, roughness * MAX_REFLECTION_LOD).xyz;

    vec2 AB = texture(textureArray[brdfLUTId], vec2(max(dot(wo, fragNormal), 0.0), roughness)).xy;
    vec3 specular2 = AB.x * F0 + AB.y;

    //vec3 color = diffuse + specular1 * specular2;
    vec3 color = baseColor;

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