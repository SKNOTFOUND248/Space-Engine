#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <numbers>

namespace Orbital {

class RingMesh {
public:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    GLsizei indexCount = 0;

    RingMesh(unsigned int segments = 64) {
        std::vector<float> data;
        std::vector<unsigned int> indices;

        // Unit annular ring (inner radius 1.0, outer radius 2.0)
        // Scaled by matrix in shader
        for (unsigned int i = 0; i <= segments; ++i) {
            float u = static_cast<float>(i) / static_cast<float>(segments);
            float theta = u * 2.0f * static_cast<float>(std::numbers::pi);

            float cosT = std::cos(theta);
            float sinT = std::sin(theta);

            // Inner vertex (radius 1.0)
            data.push_back(cosT * 1.0f);
            data.push_back(0.0f);
            data.push_back(sinT * 1.0f);
            // Normal (upwards)
            data.push_back(0.0f); data.push_back(1.0f); data.push_back(0.0f);
            // UV (radial u=0, angle v)
            data.push_back(0.0f); data.push_back(u);

            // Outer vertex (radius 2.0)
            data.push_back(cosT * 2.0f);
            data.push_back(0.0f);
            data.push_back(sinT * 2.0f);
            // Normal (upwards)
            data.push_back(0.0f); data.push_back(1.0f); data.push_back(0.0f);
            // UV (radial u=1, angle v)
            data.push_back(1.0f); data.push_back(u);
        }

        for (unsigned int i = 0; i < segments; ++i) {
            unsigned int i0 = i * 2;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = i0 + 2;
            unsigned int i3 = i0 + 3;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }

        indexCount = static_cast<GLsizei>(indices.size());

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        GLsizei stride = 8 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    ~RingMesh() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
    }

    RingMesh(const RingMesh&) = delete;
    RingMesh& operator=(const RingMesh&) = delete;

    void draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

} // namespace Orbital
