#pragma once

#include "vk_buffer.h"
#include "vk_material.h"
#include <vector>
#include <variant>

namespace tinygltf {
    class Model;
    class Mesh;
}

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static std::vector<VkVertexInputBindingDescription> get_binding_descriptions();

    static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();

    bool operator==(const Vertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }
};

namespace engine {
    class Mesh;
    using MeshPtr = std::shared_ptr<Mesh>;
    class Mesh {
    public:
        std::vector<Vertex> _vertices;
        std::vector<uint32_t> _indices;
        BufferPtr vertex_buffer;
        BufferPtr index_buffer;
        MaterialPtrV material;

        static MeshPtr create_from_obj(const char* filename);
        static MeshPtr load_from_obj(VulkanEngine* engine, const char* filename);
        static MeshPtr create_from_gltf(VulkanEngine* engine, const tinygltf::Model& model, const tinygltf::Mesh& glTFMesh);
        static MeshPtr load_from_gltf(VulkanEngine* engine, const tinygltf::Model& model, const tinygltf::Mesh& glTFMesh);
        void upload(VulkanEngine* engine);
    };

}



