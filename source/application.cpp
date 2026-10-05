#include "application.hpp"
#include "graphics.hpp"
#include "cube.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <vector>
#include <cstring>

namespace {

constexpr int max_objects = 5;

graphics::Pipeline pipeline;
graphics::Buffer vertex_buffer;
graphics::Buffer index_buffer;
std::vector<graphics::DescriptorSet> descriptor_sets;
VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
uint32_t index_count = 0;

struct ObjectState {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
    glm::vec4 color{1.0f, 0.8f, 0.2f, 1.0f};
    bool use_vertex_color = false;
    float animation_radius = 2.0f;
    float animation_speed = 1.0f;
    bool animation_playing = true;
    float animation_time = 0.0f;
};

std::vector<ObjectState> objects;
int object_count = 3;
bool use_perspective = true;
double last_time = 0.0;

glm::mat4 view_matrix = glm::lookAt(
    glm::vec3(0.0f, 2.5f, 6.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

}

namespace application {

bool initialize() {
    std::vector<cube::Vertex> vertices;
    std::vector<uint32_t> indices;
    cube::generate(vertices, indices);
    index_count = static_cast<uint32_t>(indices.size());

    VkDeviceSize vertex_size = sizeof(cube::Vertex) * vertices.size();
    VkDeviceSize index_size = sizeof(uint32_t) * indices.size();

    if (!graphics::createBuffer(vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                VMA_MEMORY_USAGE_CPU_TO_GPU, vertex_buffer)) {
        return false;
    }
    if (!graphics::createBuffer(index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                VMA_MEMORY_USAGE_CPU_TO_GPU, index_buffer)) {
        return false;
    }

    void* data = nullptr;
    vmaMapMemory(graphics::internal::context.allocator, vertex_buffer.allocation, &data);
    memcpy(data, vertices.data(), vertex_size);
    vmaUnmapMemory(graphics::internal::context.allocator, vertex_buffer.allocation);

    vmaMapMemory(graphics::internal::context.allocator, index_buffer.allocation, &data);
    memcpy(data, indices.data(), index_size);
    vmaUnmapMemory(graphics::internal::context.allocator, index_buffer.allocation);

    if (!graphics::createPipeline(graphics::internal::context.render_pass,
                                  graphics::internal::context.swapchain_extent,
                                  pipeline)) {
        return false;
    }

    if (!graphics::createDescriptorPool(max_objects, descriptor_pool)) {
        return false;
    }

    descriptor_sets.resize(max_objects);
    for (int i = 0; i < max_objects; ++i) {
        if (!graphics::allocateDescriptorSet(descriptor_pool,
                                             pipeline.descriptor_set_layout,
                                             descriptor_sets[i])) {
            return false;
        }
    }

    objects.resize(max_objects);
    for (int i = 0; i < max_objects; ++i) {
        objects[i].color = glm::vec4(
            0.5f + 0.5f * std::cos((float)i * 2.0f),
            0.5f + 0.5f * std::sin((float)i * 2.0f),
            0.5f + 0.5f * std::cos((float)i * 2.0f + 1.5f),
            1.0f);
    }

    return true;
}

void shutdown() {
    vkDeviceWaitIdle(graphics::internal::context.device);

    for (auto& set : descriptor_sets) {
        graphics::destroyDescriptorSet(set);
    }
    descriptor_sets.clear();

    graphics::destroyDescriptorPool(descriptor_pool);
    graphics::destroyPipeline(pipeline);
    graphics::destroyBuffer(vertex_buffer);
    graphics::destroyBuffer(index_buffer);
}

void update(double time) {
    double delta = time - last_time;
    last_time = time;
    if (delta < 0.0) delta = 0.0;
    if (delta > 0.1) delta = 0.1;

    ImGui::Begin("Lab 1 Control");
    ImGui::Text("Variant 1: Cube");
    ImGui::Checkbox("Perspective Projection", &use_perspective);
    ImGui::SliderInt("Object Count", &object_count, 1, max_objects);

    for (int i = 0; i < object_count; ++i) {
        ImGui::PushID(i);
        ImGui::SeparatorText("Object");
        ImGui::SliderFloat3("Position", &objects[i].position.x, -5.0f, 5.0f);
        ImGui::SliderFloat3("Rotation", &objects[i].rotation.x, -180.0f, 180.0f);
        ImGui::SliderFloat3("Scale", &objects[i].scale.x, 0.1f, 3.0f);
        ImGui::ColorEdit4("Color", &objects[i].color.x);
        ImGui::Checkbox("Use Vertex Color", &objects[i].use_vertex_color);
        ImGui::Checkbox("Animation Playing", &objects[i].animation_playing);
        ImGui::SliderFloat("Animation Speed", &objects[i].animation_speed, 0.1f, 5.0f);
        ImGui::SliderFloat("Animation Radius", &objects[i].animation_radius, 0.0f, 5.0f);
        ImGui::PopID();
    }

    if (ImGui::Button("Reset Time")) {
        for (int i = 0; i < max_objects; ++i) objects[i].animation_time = 0.0f;
    }
    ImGui::End();

    for (int i = 0; i < object_count; ++i) {
        auto& obj = objects[i];

        if (obj.animation_playing) {
            obj.animation_time += static_cast<float>(delta) * obj.animation_speed;
        }

        float phase = (float)i * (2.0f * 3.14159265f / (float)object_count);
        float angle = obj.animation_time + phase;
        float radius = obj.animation_radius;
        glm::vec3 animated_pos(radius * std::cos(angle), 0.0f, radius * std::sin(angle));

        graphics::UniformBufferObject ubo{};
        ubo.model = glm::translate(glm::mat4(1.0f), animated_pos);
        ubo.model = glm::rotate(ubo.model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        ubo.model = glm::rotate(ubo.model, glm::radians(obj.rotation.y + glm::degrees(angle * 2.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.model = glm::rotate(ubo.model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.model = glm::scale(ubo.model, obj.scale);

        ubo.view = view_matrix;

        float aspect = (float)graphics::internal::context.swapchain_extent.width
                     / (float)graphics::internal::context.swapchain_extent.height;
        if (use_perspective) {
            ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        } else {
            float s = 5.0f;
            ubo.proj = glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, 100.0f);
        }
        ubo.proj[1][1] *= -1.0f;

        ubo.base_color = obj.color;
        ubo.use_vertex_color = obj.use_vertex_color ? 1 : 0;

        memcpy(descriptor_sets[i].mapped_data, &ubo, sizeof(ubo));
    }
}

void render(const graphics::internal::FrameData& fd) {
    VkCommandBuffer cmd = fd.command_buffer;
    VkExtent2D extent = graphics::internal::context.swapchain_extent;

    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &begin_info);

    VkRenderPassBeginInfo rp{};
    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass = graphics::internal::context.render_pass;
    rp.framebuffer = fd.framebuffer;
    rp.renderArea.offset = {0, 0};
    rp.renderArea.extent = extent;

    VkClearValue clear_values[2]{};
    clear_values[0].color = {{0.05f, 0.05f, 0.10f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};
    rp.clearValueCount = 2;
    rp.pClearValues = clear_values;

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkBuffer vbs[] = { vertex_buffer.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vbs, offsets);
    vkCmdBindIndexBuffer(cmd, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    for (int i = 0; i < object_count; ++i) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout,
                                0, 1, &descriptor_sets[i].set, 0, nullptr);
        vkCmdDrawIndexed(cmd, index_count, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

}