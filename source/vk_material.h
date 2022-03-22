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

	int textureCubemapArraySize = 0;
	int baseColorTextureID = -1;
	
	REFLECT(HDRi,
		textureCubemapArraySize, 
		baseColorTextureID)
};

namespace engine {
	class Shader;

	using ShaderPtr = std::shared_ptr<Shader>;

	struct Empty_Type{};
	template <typename ParaT = Empty_Type>
	class Material {
	public:
		ShaderPtr pShaders;
		ParaT paras;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		std::unordered_map<std::string, int> textureArrayIndex;
	};

	using PbrMaterial = Material<Pbr>;
	using HDRiMaterial = Material<HDRi>;
	using MaterialPtr = std::shared_ptr<Material<>>;
	using PbrMaterialPtr = std::shared_ptr<PbrMaterial>;
	using HDRiMaterialPtr = std::shared_ptr<HDRiMaterial>;

	using MaterialV = std::variant<Material<>, PbrMaterial, HDRiMaterial>;
	using MaterialPtrV = std::variant<MaterialPtr, PbrMaterialPtr, HDRiMaterialPtr>;
}



