#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../catalog/DeepSpaceCatalog.h"
#include "../camera/CameraController.h"
#include <vector>
#include <random>
#include <cmath>
#include <numbers>

namespace Orbital {

struct DSOParticle {
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float alpha;
};

class DeepSpaceRenderer {
public:
    GLuint galaxyVAO = 0;
    GLuint galaxyVBO = 0;
    size_t galaxyParticleCount = 0;

    GLuint nebulaVAO = 0;
    GLuint nebulaVBO = 0;
    size_t nebulaParticleCount = 0;

    GLuint shaderProgram = 0;

    DeepSpaceRenderer() = default;

    void init() {
        buildShaders();
        buildGalaxyMesh();
        buildNebulaMesh();
    }

    void shutdown() {
        if (galaxyVAO) { glDeleteVertexArrays(1, &galaxyVAO); galaxyVAO = 0; }
        if (galaxyVBO) { glDeleteBuffers(1, &galaxyVBO); galaxyVBO = 0; }
        if (nebulaVAO) { glDeleteVertexArrays(1, &nebulaVAO); nebulaVAO = 0; }
        if (nebulaVBO) { glDeleteBuffers(1, &nebulaVBO); nebulaVBO = 0; }
        if (shaderProgram) { glDeleteProgram(shaderProgram); shaderProgram = 0; }
    }

    void render(const CameraController& camera, const DeepSpaceCatalog& catalog, const glm::mat4& projection) {
        if (shaderProgram == 0) return;

        glUseProgram(shaderProgram);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for luminous cosmic structures
        glDepthMask(GL_FALSE); // Cosmic background does not write to depth buffer
        glEnable(GL_PROGRAM_POINT_SIZE);

        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Render each cataloged DSO at scaled astronomical positions
        for (const auto& dso : catalog.objects) {
            // Place DSO on a distant celestial skybox sphere (e.g. radius ~ 3000 world units)
            glm::dvec3 cartPos = dso.getCartesianPosition();
            glm::dvec3 dir = (glm::length(cartPos) > 1e-6) ? glm::normalize(cartPos) : glm::dvec3(0.0, 1.0, 0.0);
            
            // Scaled distance on celestial sphere
            glm::vec3 renderPos = glm::vec3(dir * 2500.0);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), renderPos);
            // Apply DSO tilt
            model = glm::rotate(model, glm::radians(dso.tiltAngleDegrees), glm::vec3(1.0f, 0.3f, 0.0f));
            
            // Scale based on visual magnitude / diameter
            float scale = (dso.type == DSOType::Galaxy) ? 220.0f : 160.0f;
            model = glm::scale(model, glm::vec3(scale));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shaderProgram, "tintColor"), 1, glm::value_ptr(dso.baseColor));

            if (dso.type == DSOType::Galaxy && galaxyVAO != 0) {
                glBindVertexArray(galaxyVAO);
                glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(galaxyParticleCount));
            } else if ((dso.type == DSOType::Nebula || dso.type == DSOType::StarCluster) && nebulaVAO != 0) {
                glBindVertexArray(nebulaVAO);
                glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(nebulaParticleCount));
            }
        }

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

private:
    void buildShaders() {
        const char* vertSrc = R"(#version 430 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        layout (location = 2) in float aSize;
        layout (location = 3) in float aAlpha;

        out vec3 FragColor;
        out float FragAlpha;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform vec3 tintColor;

        void main()
        {
            FragColor = aColor * tintColor;
            FragAlpha = aAlpha;

            // Remove camera translation for background celestial objects
            mat4 viewNoTrans = mat4(mat3(view));
            vec4 posEye = viewNoTrans * model * vec4(aPos, 1.0);
            gl_Position = projection * posEye;

            // Perspective point size
            float dist = length(posEye.xyz);
            gl_PointSize = clamp(aSize * (400.0 / max(dist, 1.0)), 1.5, 32.0);
        }
        )";

        const char* fragSrc = R"(#version 430 core
        out vec4 OutColor;
        in vec3 FragColor;
        in float FragAlpha;

        void main()
        {
            // Soft gaussian radial point particle
            vec2 coord = gl_PointCoord - vec2(0.5);
            float distSq = dot(coord, coord);
            if (distSq > 0.25) discard;

            float intensity = exp(-distSq * 9.0);
            OutColor = vec4(FragColor * 1.5, intensity * FragAlpha);
        }
        )";

        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &vertSrc, nullptr);
        glCompileShader(vert);

        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &fragSrc, nullptr);
        glCompileShader(frag);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vert);
        glAttachShader(shaderProgram, frag);
        glLinkProgram(shaderProgram);

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void buildGalaxyMesh() {
        std::vector<DSOParticle> particles;
        particles.reserve(16000);

        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        std::normal_distribution<float> normDist(0.0f, 0.08f);

        // 1. Dense central galactic bulge (Warm golden core)
        for (int i = 0; i < 4000; ++i) {
            float r = std::pow(dist01(rng), 2.2f) * 0.35f;
            float theta = dist01(rng) * 2.0f * static_cast<float>(std::numbers::pi);
            float phi = (dist01(rng) - 0.5f) * static_cast<float>(std::numbers::pi) * 0.6f;

            glm::vec3 pos(
                r * std::cos(phi) * std::cos(theta),
                r * std::sin(phi) * 0.6f,
                r * std::cos(phi) * std::sin(theta)
            );

            glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.95f, 0.85f), glm::vec3(1.0f, 0.70f, 0.40f), r / 0.35f);
            particles.push_back({pos, col, 4.0f, 0.8f});
        }

        // 2. Logarithmic Spiral Arms (Blue-white hot star clusters and star-forming regions)
        constexpr int numArms = 2;
        constexpr float armTightness = 0.32f;

        for (int arm = 0; arm < numArms; ++arm) {
            float armAngleOffset = arm * (2.0f * static_cast<float>(std::numbers::pi) / numArms);

            for (int i = 0; i < 5000; ++i) {
                float t = dist01(rng); // normalized distance along spiral arm
                float r = 0.2f + 0.8f * t;
                float theta = armAngleOffset + (r / armTightness);

                // Add radial dispersion and thickness
                float spread = normDist(rng) * (0.05f + 0.15f * t);
                float height = normDist(rng) * (0.02f + 0.05f * t);

                float finalTheta = theta + spread;
                glm::vec3 pos(
                    r * std::cos(finalTheta),
                    height,
                    r * std::sin(finalTheta)
                );

                // Spiral arm stars are hot young blue/cyan stars with OB associations
                glm::vec3 starColor = glm::mix(glm::vec3(0.7f, 0.85f, 1.0f), glm::vec3(0.9f, 0.95f, 1.0f), dist01(rng));
                if (dist01(rng) < 0.15f) {
                    // Glowing pink H-II starburst regions
                    starColor = glm::vec3(1.0f, 0.35f, 0.55f);
                }

                particles.push_back({pos, starColor, 3.2f, 0.65f});
            }
        }

        galaxyParticleCount = particles.size();

        glGenVertexArrays(1, &galaxyVAO);
        glGenBuffers(1, &galaxyVBO);

        glBindVertexArray(galaxyVAO);
        glBindBuffer(GL_ARRAY_BUFFER, galaxyVBO);
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(DSOParticle), particles.data(), GL_STATIC_DRAW);

        // pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DSOParticle), (void*)offsetof(DSOParticle, position));
        glEnableVertexAttribArray(0);
        // color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DSOParticle), (void*)offsetof(DSOParticle, color));
        glEnableVertexAttribArray(1);
        // size
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(DSOParticle), (void*)offsetof(DSOParticle, size));
        glEnableVertexAttribArray(2);
        // alpha
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(DSOParticle), (void*)offsetof(DSOParticle, alpha));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
    }

    void buildNebulaMesh() {
        std::vector<DSOParticle> particles;
        particles.reserve(10000);

        std::mt19937 rng(1337);
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        std::normal_distribution<float> normDist(0.0f, 0.25f);

        for (int i = 0; i < 10000; ++i) {
            float x = normDist(rng);
            float y = normDist(rng) * 0.7f;
            float z = normDist(rng);

            float dist = std::sqrt(x*x + y*y + z*z);
            if (dist > 1.2f) continue;

            // Emission colors: Oxygen III (teal) to Hydrogen Alpha (crimson / magenta)
            float t = dist / 1.2f;
            glm::vec3 col = glm::mix(glm::vec3(0.2f, 0.85f, 0.95f), glm::vec3(0.95f, 0.25f, 0.55f), t);

            particles.push_back({glm::vec3(x, y, z), col, 8.0f, 0.25f});
        }

        nebulaParticleCount = particles.size();

        glGenVertexArrays(1, &nebulaVAO);
        glGenBuffers(1, &nebulaVBO);

        glBindVertexArray(nebulaVAO);
        glBindBuffer(GL_ARRAY_BUFFER, nebulaVBO);
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(DSOParticle), particles.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DSOParticle), (void*)offsetof(DSOParticle, position));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DSOParticle), (void*)offsetof(DSOParticle, color));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(DSOParticle), (void*)offsetof(DSOParticle, size));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(DSOParticle), (void*)offsetof(DSOParticle, alpha));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
    }
};

} // namespace Orbital
