#pragma once
#include "vk_types.h"
#include "Reflect.h"
#include <variant>
#include <string>

using namespace Reflect;

struct Pbr {
	int textureCubemapArraySize = 1;
	int irradianceMapId = 0;
	int brdfLUTId = 0;
	int prefilteredMapId = 0;
	int texture2DArraySize = 1;
	int baseColorTextureID = -1;
	float baseColorRed = 1.0f;
	float baseColorGreen = 0.0f;
	float baseColorBlue = 1.0f;
	int metallicRoughnessTextureId = -1;
	float metalnessFactor = 0.0f;
	float roughnessFactor = 0.4f;

	REFLECT(Pbr,
		textureCubemapArraySize,
		irradianceMapId,
		brdfLUTId,
		prefilteredMapId,
		texture2DArraySize,
		baseColorTextureID,
		baseColorRed,
		baseColorGreen,
		baseColorBlue,
		metallicRoughnessTextureId,
		metalnessFactor,
		roughnessFactor)
};

struct HDRi {
	int base_color_texture_id = -1;

	REFLECT(HDRi,
		base_color_texture_id)
};

struct PbrTexture {
	uint32_t base_color_texture_id = -1;
	uint32_t matallic_texture_id = -1;
	uint32_t roughness_texture_id = -1;
	uint32_t normal_texture_id = -1;

	REFLECT(PbrTexture,
		base_color_texture_id,
		matallic_texture_id,
		roughness_texture_id,
		normal_texture_id)
};

namespace engine {
	class Shader;

	using ShaderPtr = std::shared_ptr<Shader>;

	struct Empty_Type {};
	template <typename ParaT = Empty_Type>
	class Material {
	public:
		ShaderPtr shaders;
		ParaT paras;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		std::unordered_map<std::string, int> textureArrayIndex;
	};

	using PbrMaterial = Material<Pbr>;
	using HDRiMaterial = Material<HDRi>;
	using PbrTextureMaterial = Material<PbrTexture>;
	using MaterialPtr = std::shared_ptr<Material<>>;
	using PbrMaterialPtr = std::shared_ptr<PbrMaterial>;
	using PbrTextureMaterialPtr = std::shared_ptr<PbrTextureMaterial>;
	using HDRiMaterialPtr = std::shared_ptr<HDRiMaterial>;

	using MaterialV = std::variant<PbrMaterial, PbrTextureMaterial, HDRiMaterial>;
	using MaterialPtrV = std::variant<PbrMaterialPtr, PbrTextureMaterialPtr, HDRiMaterialPtr>;
}



