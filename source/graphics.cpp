#include "graphics.hpp"
#include "graphics_internal.hpp"
#include <fstream>
#include <iostream>
#include <cstring>

namespace graphics {

bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, Buffer& out_buffer) {
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = memory_usage;
    if (memory_usage == VMA_MEMORY_USAGE_CPU_TO_GPU) {
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                         | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    if (vmaCreateBuffer(internal::context.allocator, &buffer_info, &alloc_info,
                        &out_buffer.buffer, &out_buffer.allocation, nullptr) != VK_SUCCESS) {
        std::cerr << "vmaCreateBuffer failed\n";
        return false;
    }
    out_buffer.size = size;
    return true;
}

void destroyBuffer(Buffer& buffer) {
    if (buffer.buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(internal::context.allocator, buffer.buffer, buffer.allocation);
        buffer.buffer = VK_NULL_HANDLE;
        buffer.allocation = VK_NULL_HANDLE;
    }
}

static bool loadFile(const std::string& filename, std::vector<char>& out) {
    const char* prefixes[] = { "", "../", "../../" };
    for (auto prefix : prefixes) {
        std::ifstream file(std::string(prefix) + filename, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            size_t file_size = static_cast<size_t>(file.tellg());
            out.resize(file_size);
            file.seekg(0);
            file.read(out.data(), file_size);
            return true;
        }
    }
    std::cerr << "Failed to open shader file: " << filename << std::endl;
    return false;
}

bool createShaderModule(const std::string& filename, VkShaderModule& out_module) {
    std::vector<char> buffer;
    if (!loadFile(filename, buffer)) return false;

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = buffer.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    if (vkCreateShaderModule(internal::context.device, &create_info, nullptr, &out_module) != VK_SUCCESS) {
        return false;
    }
    return true;
}

void destroyShaderModule(VkShaderModule module) {
    if (module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(internal::context.device, module, nullptr);
    }
}

bool createPipeline(VkRenderPass render_pass, VkExtent2D extent, Pipeline& out_pipeline) {
    (void)extent;
    VkShaderModule vert_module = VK_NULL_HANDLE;
    VkShaderModule frag_module = VK_NULL_HANDLE;
    if (!createShaderModule("shaders/cube.vert.spv", vert_module)) return false;
    if (!createShaderModule("shaders/cube.frag.spv", frag_module)) return false;

    VkPipelineShaderStageCreateInfo vert_stage{};
    vert_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage.module = vert_module;
    vert_stage.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage{};
    frag_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage.module = frag_module;
    frag_stage.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage, frag_stage };

    VkVertexInputBindingDescription binding_desc{};
    binding_desc.binding = 0;
    binding_desc.stride = sizeof(float) * 6;
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribute_descs[2]{};
    attribute_descs[0].binding = 0;
    attribute_descs[0].location = 0;
    attribute_descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descs[0].offset = 0;
    attribute_descs[1].binding = 0;
    attribute_descs[1].location = 1;
    attribute_descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descs[1].offset = sizeof(float) * 3;

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 2;
    vertex_input_info.pVertexAttributeDescriptions = attribute_descs;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(internal::context.device, &layout_info, nullptr,
                                    &out_pipeline.descriptor_set_layout) != VK_SUCCESS) {
        return false;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &out_pipeline.descriptor_set_layout;

    if (vkCreatePipelineLayout(internal::context.device, &pipeline_layout_info, nullptr,
                               &out_pipeline.layout) != VK_SUCCESS) {
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = out_pipeline.layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;

    if (vkCreateGraphicsPipelines(internal::context.device, VK_NULL_HANDLE, 1, &pipeline_info,
                                  nullptr, &out_pipeline.pipeline) != VK_SUCCESS) {
        std::cerr << "vkCreateGraphicsPipelines failed\n";
        return false;
    }

    destroyShaderModule(vert_module);
    destroyShaderModule(frag_module);
    return true;
}

void destroyPipeline(Pipeline& pipeline) {
    if (pipeline.pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(internal::context.device, pipeline.pipeline, nullptr);
    if (pipeline.layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(internal::context.device, pipeline.layout, nullptr);
    if (pipeline.descriptor_set_layout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(internal::context.device, pipeline.descriptor_set_layout, nullptr);
}

bool createDescriptorPool(uint32_t max_sets, VkDescriptorPool& out_pool) {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = max_sets;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = max_sets;

    return vkCreateDescriptorPool(internal::context.device, &pool_info, nullptr, &out_pool) == VK_SUCCESS;
}

bool allocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout, DescriptorSet& out_set) {
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(internal::context.device, &alloc_info, &out_set.set) != VK_SUCCESS) {
        return false;
    }

    VkDeviceSize buffer_size = sizeof(UniformBufferObject);
    if (!createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, out_set.uniform_buffer)) {
        return false;
    }

    if (vmaMapMemory(internal::context.allocator, out_set.uniform_buffer.allocation, &out_set.mapped_data) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = out_set.uniform_buffer.buffer;
    buffer_info.offset = 0;
    buffer_info.range = buffer_size;

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = out_set.set;
    descriptor_write.dstBinding = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(internal::context.device, 1, &descriptor_write, 0, nullptr);
    return true;
}

void destroyDescriptorSet(DescriptorSet& set) {
    if (set.mapped_data != nullptr) {
        vmaUnmapMemory(internal::context.allocator, set.uniform_buffer.allocation);
        set.mapped_data = nullptr;
    }
    destroyBuffer(set.uniform_buffer);
}

void destroyDescriptorPool(VkDescriptorPool pool) {
    if (pool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(internal::context.device, pool, nullptr);
}

}