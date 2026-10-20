#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <numbers>

namespace Orbital {

class SphereMesh {
public:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    GLsizei indexCount = 0;

    SphereMesh(unsigned int xSegments = 48, unsigned int ySegments = 32) {
        std::vector<float> data;
        std::vector<unsigned int> indices;

        for (unsigned int y = 0; y <= ySegments; ++y) {
            float v = static_cast<float>(y) / static_cast<float>(ySegments);
            float phi = v * static_cast<float>(std::numbers::pi);

            for (unsigned int x = 0; x <= xSegments; ++x) {
                float u = static_cast<float>(x) / static_cast<float>(xSegments);
                float theta = u * 2.0f * static_cast<float>(std::numbers::pi);

                float xPos = std::cos(theta) * std::sin(phi);
                float yPos = std::cos(phi);
                float zPos = std::sin(theta) * std::sin(phi);

                // Position (Unit sphere)
                data.push_back(xPos);
                data.push_back(yPos);
                data.push_back(zPos);

                // Normal
                data.push_back(xPos);
                data.push_back(yPos);
                data.push_back(zPos);

                // UV
                data.push_back(u);
                data.push_back(v);
            }
        }

        for (unsigned int y = 0; y < ySegments; ++y) {
            for (unsigned int x = 0; x < xSegments; ++x) {
                unsigned int i0 = y * (xSegments + 1) + x;
                unsigned int i1 = (y + 1) * (xSegments + 1) + x;
                unsigned int i2 = (y + 1) * (xSegments + 1) + (x + 1);
                unsigned int i3 = y * (xSegments + 1) + (x + 1);

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);

                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i3);
            }
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
        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // UV
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    ~SphereMesh() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
    }

    SphereMesh(const SphereMesh&) = delete;
    SphereMesh& operator=(const SphereMesh&) = delete;

    void draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

} // namespace Orbital
