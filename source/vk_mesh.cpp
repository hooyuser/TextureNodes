#include "vk_mesh.h"
#include "vk_engine.h"
#include "util.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <iostream>

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions{};

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindingDescriptions.emplace_back(bindingDescription);
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	attributeDescriptions.reserve(CountMember<Vertex>::value);


	VkVertexInputAttributeDescription posAttributeDescription = {};
	posAttributeDescription.binding = 0;
	posAttributeDescription.location = 0;
	posAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	posAttributeDescription.offset = offsetof(Vertex, pos);

	VkVertexInputAttributeDescription normalAttributeDescription = {};
	normalAttributeDescription.binding = 0;
	normalAttributeDescription.location = 1;
	normalAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttributeDescription.offset = offsetof(Vertex, normal);

	VkVertexInputAttributeDescription texCoordAttributeDescription = {};
	texCoordAttributeDescription.binding = 0;
	texCoordAttributeDescription.location = 2;
	texCoordAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	texCoordAttributeDescription.offset = offsetof(Vertex, texCoord);

	attributeDescriptions.emplace_back(posAttributeDescription);
	attributeDescriptions.emplace_back(normalAttributeDescription);
	attributeDescriptions.emplace_back(texCoordAttributeDescription);

	return attributeDescriptions;
}


namespace engine {
	using MeshPtr = std::shared_ptr<Mesh>;

	MeshPtr Mesh::createFromObj(const char* filename) {
		auto mesh = std::make_shared<Mesh>();
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(mesh->_vertices.size());
					mesh->_vertices.emplace_back(vertex);
				}

				mesh->_indices.emplace_back(uniqueVertices[vertex]);
			}
		}

		return mesh;
	}

	MeshPtr Mesh::createFromGLTF(VulkanEngine* engine, const tinygltf::Model& model, const tinygltf::Mesh& glTFMesh) {
		auto mesh = std::make_shared<Mesh>();
		for (auto& glTFPrimitive : glTFMesh.primitives) {
			//const tinygltf::Primitive& glTFPrimitive = glTFMesh.primitives[i];
			uint32_t firstIndex = static_cast<uint32_t>(mesh->_indices.size());
			uint32_t vertexStart = static_cast<uint32_t>(mesh->_vertices.size());
			uint32_t indexCount = 0;
			// Vertices
			{
				const float* positionBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				size_t vertexCount = 0;

				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}
				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				// Append data to model's vertex buffer
				for (size_t v = 0; v < vertexCount; v++) {
					Vertex vertex{};
					vertex.pos = glm::make_vec3(&positionBuffer[v * 3]);
					vertex.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
					vertex.texCoord = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
					mesh->_vertices.push_back(vertex);
				}
			}
			// Indices
			{
				const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.indices];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				indexCount += static_cast<uint32_t>(accessor.count);

				// glTF supports different component types of indices
				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					uint32_t* buf = new uint32_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
					for (size_t index = 0; index < accessor.count; index++) {
						mesh->_indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					uint16_t* buf = new uint16_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
					for (size_t index = 0; index < accessor.count; index++) {
						mesh->_indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					uint8_t* buf = new uint8_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
					for (size_t index = 0; index < accessor.count; index++) {
						mesh->_indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return mesh;
				}
			}
			mesh->pMaterial = engine->loadedMaterials[glTFPrimitive.material];  //Does not support different materials for different primitives 
		}
		return mesh;
	}


	MeshPtr Mesh::loadFromObj(VulkanEngine* engine, const char* filename) {
		auto mesh = Mesh::createFromObj(filename);

		mesh->upload(engine);

		return mesh;
	}

	MeshPtr Mesh::loadFromGLTF(VulkanEngine* engine, const tinygltf::Model& model, const tinygltf::Mesh& glTFMesh) {
		auto mesh = Mesh::createFromGLTF(engine, model, glTFMesh);

		mesh->upload(engine);

		return mesh;
	}

	void Mesh::upload(VulkanEngine* engine) {
		VkDeviceSize vertexbufferSize = sizeof(_vertices[0]) * _vertices.size();

		auto pStagingVertexBuffer = engine::Buffer::createBuffer(engine,
			vertexbufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			TEMP_BIT);

		pStagingVertexBuffer->copyFromHost(_vertices.data());

		pVertexBuffer = engine::Buffer::createBuffer(engine,
			vertexbufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);

		pVertexBuffer->copyFromBuffer(engine, pStagingVertexBuffer->buffer);

		VkDeviceSize indexBufferSize = sizeof(_indices[0]) * _indices.size();

		auto pStagingIndexBuffer = engine::Buffer::createBuffer(engine,
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			TEMP_BIT);

		pStagingIndexBuffer->copyFromHost(_indices.data());

		pIndexBuffer = engine::Buffer::createBuffer(engine,
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);

		pIndexBuffer->copyFromBuffer(engine, pStagingIndexBuffer->buffer);
	}
}
