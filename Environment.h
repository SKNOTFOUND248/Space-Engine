#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Environment {
public:
    unsigned int gridVAO = 0;
    unsigned int gridVBO = 0;
    int gridVertexCount = 0;

    unsigned int boxVAO = 0;
    unsigned int boxVBO = 0;
    int boxVertexCount = 0;

    Environment(glm::vec3 boxMin, glm::vec3 boxMax) {
        initGrid(boxMin);
        initBox(boxMin, boxMax);
    }

    ~Environment() {
        if (gridVAO != 0) glDeleteVertexArrays(1, &gridVAO);
        if (gridVBO != 0) glDeleteBuffers(1, &gridVBO);
        if (boxVAO != 0) glDeleteVertexArrays(1, &boxVAO);
        if (boxVBO != 0) glDeleteBuffers(1, &boxVBO);
    }

    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;

    void drawGrid() const {
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, gridVertexCount);
    }

    void drawBox() const {
        glBindVertexArray(boxVAO);
        glDrawArrays(GL_LINES, 0, boxVertexCount);
    }

private:
    void initGrid(glm::vec3 boxMin) {
        std::vector<float> gridVertices;
        float y = boxMin.y - 0.005f;
        float extent = 4.0f;
        float step = 0.4f;

        for (float x = -extent; x <= extent + 1e-4f; x += step) {
            gridVertices.push_back(x); gridVertices.push_back(y); gridVertices.push_back(-extent);
            gridVertices.push_back(x); gridVertices.push_back(y); gridVertices.push_back(extent);
        }
        for (float z = -extent; z <= extent + 1e-4f; z += step) {
            gridVertices.push_back(-extent); gridVertices.push_back(y); gridVertices.push_back(z);
            gridVertices.push_back(extent);  gridVertices.push_back(y); gridVertices.push_back(z);
        }

        gridVertexCount = static_cast<int>(gridVertices.size() / 3);

        glGenVertexArrays(1, &gridVAO);
        glGenBuffers(1, &gridVBO);

        glBindVertexArray(gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);
    }

    void initBox(glm::vec3 min, glm::vec3 max) {
        // 12 edges of the bounding tank
        std::vector<float> boxVertices = {
            // Bottom 4 edges
            min.x, min.y, min.z,   max.x, min.y, min.z,
            max.x, min.y, min.z,   max.x, min.y, max.z,
            max.x, min.y, max.z,   min.x, min.y, max.z,
            min.x, min.y, max.z,   min.x, min.y, min.z,

            // Top 4 edges
            min.x, max.y, min.z,   max.x, max.y, min.z,
            max.x, max.y, min.z,   max.x, max.y, max.z,
            max.x, max.y, max.z,   min.x, max.y, max.z,
            min.x, max.y, max.z,   min.x, max.y, min.z,

            // 4 Vertical Pillars
            min.x, min.y, min.z,   min.x, max.y, min.z,
            max.x, min.y, min.z,   max.x, max.y, min.z,
            max.x, min.y, max.z,   max.x, max.y, max.z,
            min.x, min.y, max.z,   min.x, max.y, max.z
        };

        boxVertexCount = static_cast<int>(boxVertices.size() / 3);

        glGenVertexArrays(1, &boxVAO);
        glGenBuffers(1, &boxVBO);

        glBindVertexArray(boxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
        glBufferData(GL_ARRAY_BUFFER, boxVertices.size() * sizeof(float), boxVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);
    }
};
