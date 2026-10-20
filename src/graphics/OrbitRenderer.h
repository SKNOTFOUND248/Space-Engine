#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../physics/CelestialBody.h"
#include "../camera/CameraController.h"
#include <vector>

namespace Orbital {

class OrbitRenderer {
public:
    unsigned int VAO = 0;
    unsigned int VBO = 0;

    OrbitRenderer() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Position attribute (vec3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    ~OrbitRenderer() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
    }

    OrbitRenderer(const OrbitRenderer&) = delete;
    OrbitRenderer& operator=(const OrbitRenderer&) = delete;

    void drawLineStrip(const std::vector<glm::vec3>& points) const {
        if (points.size() < 2) return;

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);

        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(points.size()));
        glBindVertexArray(0);
    }

    void drawTrail(const CelestialBody& body, const CameraController& camera) const {
        if (body.trail.size() < 2) return;

        std::vector<glm::vec3> renderPoints;
        renderPoints.reserve(body.trail.size());
        for (const auto& pt : body.trail) {
            renderPoints.push_back(camera.toRenderSpace(pt));
        }

        drawLineStrip(renderPoints);
    }

    void drawConicPrediction(const std::vector<glm::dvec3>& worldPoints, const CameraController& camera) const {
        if (worldPoints.size() < 2) return;

        std::vector<glm::vec3> renderPoints;
        renderPoints.reserve(worldPoints.size());
        for (const auto& pt : worldPoints) {
            renderPoints.push_back(camera.toRenderSpace(pt));
        }

        drawLineStrip(renderPoints);
    }
};

} // namespace Orbital
