#include "vk_mesh.h"
#include "vk_engine.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <iostream>

template<>
struct std::hash<Vertex> {
	size_t operator()(Vertex const& vertex) const noexcept {
		return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};

std::vector<VkVertexInputBindingDescription> Vertex::get_binding_descriptions() {
	constexpr VkVertexInputBindingDescription binding_description{
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	return { binding_description };
}

std::vector<VkVertexInputAttributeDescription> Vertex::get_attribute_descriptions() {

	constexpr VkVertexInputAttributeDescription pos_attribute_description{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, pos),
	};

	constexpr VkVertexInputAttributeDescription normal_attribute_description{
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, normal),
	};

	constexpr VkVertexInputAttributeDescription tex_coordinate_attribute_description{
		.location = 2,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(Vertex, texCoord),
	};

	return {
		pos_attribute_description ,
		normal_attribute_description,
		tex_coordinate_attribute_description
	};
}


namespace engine {
	using MeshPtr = std::shared_ptr<Mesh>;

	MeshPtr Mesh::create_from_obj(const char* filename) {
		auto mesh = std::make_shared<Mesh>();
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<Vertex, uint32_t> unique_vertices{};

		for (const auto& shape : shapes) {
			for (const auto& [vertex_index, normal_index, texcoord_index] : shape.mesh.indices) {
				Vertex vertex{
					.pos = {
						attrib.vertices[3 * vertex_index + 0],
						attrib.vertices[3 * vertex_index + 1],
						attrib.vertices[3 * vertex_index + 2]
					},
					.normal = {
						attrib.normals[3 * normal_index + 0],
						attrib.normals[3 * normal_index + 1],
						attrib.normals[3 * normal_index + 2]
					},
					.texCoord = {
						attrib.texcoords[2 * texcoord_index + 0],
						1.0f - attrib.texcoords[2 * texcoord_index + 1]
					}
				};

				if (!unique_vertices.contains(vertex)) {
					unique_vertices[vertex] = static_cast<uint32_t>(mesh->_vertices.size());
					mesh->_vertices.emplace_back(vertex);
				}

				mesh->_indices.emplace_back(unique_vertices[vertex]);
			}
		}

		return mesh;
	}

	MeshPtr Mesh::create_from_gltf(VulkanEngine* engine, const tinygltf::Model& model, const tinygltf::Mesh& glTFMesh) {
		auto mesh = std::make_shared<Mesh>();
		for (auto& glTFPrimitive : glTFMesh.primitives) {
			//const tinygltf::Primitive& glTFPrimitive = glTFMesh.primitives[i];
			//auto firstIndex = static_cast<uint32_t>(mesh->_indices.size());
			auto const vertex_start = static_cast<uint32_t>(mesh->_vertices.size());

			// Vertices
			{
				const float* positionBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				size_t vertexCount = 0;

				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.contains("POSITION")) {
					const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}
				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.contains("NORMAL")) {
					const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (glTFPrimitive.attributes.contains("TEXCOORD_0")) {
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
					mesh->_vertices.emplace_back(vertex);
				}
			}
			// Indices
			{
				const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.indices];
				const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

				//uint32_t indexCount = 0;
				//indexCount += static_cast<uint32_t>(accessor.count);

				// glTF supports different component types of indices
				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					auto* buf = new uint32_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + buffer_view.byteOffset], accessor.count * sizeof(uint32_t));
					for (size_t index = 0; index < accessor.count; index++) {
						mesh->_indices.emplace_back(buf[index] + vertex_start);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					auto* buf = new uint16_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + buffer_view.byteOffset], accessor.count * sizeof(uint16_t));
					for (size_t index = 0; index < accessor.count; index++) {
						mesh->_indices.emplace_back(buf[index] + vertex_start);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					auto* buf = new uint8_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + buffer_view.byteOffset], accessor.count * sizeof(uint8_t));
					for (size_t index = 0; index < accessor.count; index++) {
						mesh->_indices.emplace_back(buf[index] + vertex_start);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return mesh;
				}
			}
			mesh->material = engine->loaded_materials[glTFPrimitive.material];  //Does not support different materials for different primitives 
		}
		return mesh;
	}


	MeshPtr Mesh::load_from_obj(VulkanEngine* engine, const char* filename) {
		auto mesh = create_from_obj(filename);

		mesh->upload(engine);

		return mesh;
	}

	MeshPtr Mesh::load_from_gltf(VulkanEngine* engine, const tinygltf::Model& model, const tinygltf::Mesh& glTFMesh) {
		auto mesh = create_from_gltf(engine, model, glTFMesh);

		mesh->upload(engine);

		return mesh;
	}

	void Mesh::upload(VulkanEngine* engine) {
		const VkDeviceSize vertex_buffer_size = sizeof(_vertices[0]) * _vertices.size();

		auto const staging_vertex_buffer = engine::Buffer::create_buffer(engine,
			vertex_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			TEMP_BIT);

		staging_vertex_buffer->copy_from_host(_vertices.data());

		vertex_buffer = engine::Buffer::create_buffer(engine,
			vertex_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);

		vertex_buffer->copy_from_buffer(engine, staging_vertex_buffer->buffer);

		const VkDeviceSize index_buffer_size = sizeof(_indices[0]) * _indices.size();

		auto const staging_index_buffer = engine::Buffer::create_buffer(engine,
			index_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			TEMP_BIT);

		staging_index_buffer->copy_from_host(_indices.data());

		index_buffer = engine::Buffer::create_buffer(engine,
			index_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			SWAPCHAIN_INDEPENDENT_BIT);

		index_buffer->copy_from_buffer(engine, staging_index_buffer->buffer);
	}
}
