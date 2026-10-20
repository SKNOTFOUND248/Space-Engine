#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <cmath>
#include <numbers>

namespace Orbital {

class Starfield {
public:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    GLsizei starCount = 3500;

    Starfield() {
        std::vector<float> starData;
        starData.reserve(starCount * 6); // Pos (vec3) + Color (vec3)

        std::mt19937 rng(42); // Deterministic seed
        std::uniform_real_distribution<float> uDist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);

        float sphereRadius = 4000.0f; // Far in background

        for (int i = 0; i < starCount; ++i) {
            // Uniform point on sphere
            float u = uDist(rng);
            float theta = colorDist(rng) * 2.0f * static_cast<float>(std::numbers::pi);
            float r = std::sqrt(std::max(0.0f, 1.0f - u * u));

            float x = sphereRadius * r * std::cos(theta);
            float y = sphereRadius * u;
            float z = sphereRadius * r * std::sin(theta);

            starData.push_back(x);
            starData.push_back(y);
            starData.push_back(z);

            // Spectral color distribution (blue giants, white, yellow, red dwarfs)
            float cChoice = colorDist(rng);
            glm::vec3 col;
            if (cChoice < 0.2f) {
                col = glm::vec3(0.65f, 0.8f, 1.0f); // O/B Blue
            } else if (cChoice < 0.5f) {
                col = glm::vec3(0.95f, 0.95f, 1.0f); // A/F White
            } else if (cChoice < 0.85f) {
                col = glm::vec3(1.0f, 0.90f, 0.70f); // G/K Yellow
            } else {
                col = glm::vec3(1.0f, 0.55f, 0.40f); // M Red
            }

            // Brightness variation
            float brightness = 0.4f + colorDist(rng) * 0.6f;
            col *= brightness;

            starData.push_back(col.r);
            starData.push_back(col.g);
            starData.push_back(col.b);
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, starData.size() * sizeof(float), starData.data(), GL_STATIC_DRAW);

        GLsizei stride = 6 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    ~Starfield() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
    }

    Starfield(const Starfield&) = delete;
    Starfield& operator=(const Starfield&) = delete;

    void draw() const {
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, starCount);
        glBindVertexArray(0);
    }
};

} // namespace Orbital
