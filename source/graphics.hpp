#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace graphics {

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 base_color;
    int use_vertex_color;
};

bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, Buffer& out_buffer);
void destroyBuffer(Buffer& buffer);

bool createShaderModule(const std::string& filename, VkShaderModule& out_module);
void destroyShaderModule(VkShaderModule module);

struct Pipeline {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
};

bool createPipeline(VkRenderPass render_pass, VkExtent2D extent, Pipeline& out_pipeline);
void destroyPipeline(Pipeline& pipeline);

struct DescriptorSet {
    VkDescriptorSet set = VK_NULL_HANDLE;
    Buffer uniform_buffer;
    void* mapped_data = nullptr;
};

bool createDescriptorPool(uint32_t max_sets, VkDescriptorPool& out_pool);
bool allocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout, DescriptorSet& out_set);
void destroyDescriptorSet(DescriptorSet& set);
void destroyDescriptorPool(VkDescriptorPool pool);

}