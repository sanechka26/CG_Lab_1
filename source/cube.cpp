#include "cube.hpp"

namespace cube {
    void generate(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        vertices.clear();
        indices.clear();

        vertices = {
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},
        };

        indices = {
            0, 1, 2, 2, 3, 0,
            4, 6, 5, 6, 7, 4,
            0, 4, 5, 5, 1, 0,
            3, 2, 6, 6, 7, 3,
            0, 3, 7, 7, 4, 0,
            1, 5, 6, 6, 2, 1,
        };
    }
}