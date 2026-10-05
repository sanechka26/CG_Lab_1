#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace cube {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    void generate(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
}