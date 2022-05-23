#version 460
#extension GL_EXT_nonuniform_qualifier:enable

layout (constant_id = 0) const int baseColorTextureId = -1;

layout(set = 1, binding = 0) uniform samplerCube cubemapSampler[];

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    if (fragTexCoord.y>0.0){
     outColor = vec4(fragTexCoord.y, vec3(1.0));
	}
	else{
		outColor = vec4(texture(cubemapSampler[baseColorTextureId], normalize(fragTexCoord)).rgb*5.0, 1.0);
	}
	//outColor = vec4(texture(cubemapSampler[baseColorTextureId], normalize(vec3(1.0))).rgb, 1.0);
    //outColor = vec4(vec3(fract(fragTexCoord.x*10.0)), 1.0);
}