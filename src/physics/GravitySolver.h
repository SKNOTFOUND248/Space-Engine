#pragma once

#include "CelestialBody.h"
#include <vector>
#include <cmath>

namespace Orbital {

class GravitySolver {
public:
    double softening = 1e5; // Softening parameter epsilon in meters (prevents division by zero in close encounters)
    uint64_t numericalWarnings = 0;

    void computeAccelerations(std::vector<CelestialBody>& bodies) {
        size_t n = bodies.size();
        double eps2 = softening * softening;

        // Reset accelerations
        for (auto& b : bodies) {
            b.acceleration = glm::dvec3(0.0);
        }

        // Direct N-body O(N^2) pairwise mutual gravitation
        for (size_t i = 0; i < n; ++i) {
            if (bodies[i].isDestroyed) continue;

            for (size_t j = i + 1; j < n; ++j) {
                if (bodies[j].isDestroyed) continue;

                glm::dvec3 rVec = bodies[j].position - bodies[i].position;
                double distSq = glm::dot(rVec, rVec);
                double denom = std::pow(distSq + eps2, 1.5);

                if (denom > 1e-12) {
                    double gFactor = Constants::G / denom;
                    glm::dvec3 forceDir = rVec * gFactor;

                    if (!bodies[i].isPinned) {
                        bodies[i].acceleration += forceDir * bodies[j].mass;
                    }
                    if (!bodies[j].isPinned) {
                        bodies[j].acceleration -= forceDir * bodies[i].mass;
                    }
                } else {
                    numericalWarnings++;
                }
            }
        }

        // Verify sanity
        for (auto& b : bodies) {
            if (!std::isfinite(b.acceleration.x) || 
                !std::isfinite(b.acceleration.y) || 
                !std::isfinite(b.acceleration.z)) {
                b.acceleration = glm::dvec3(0.0);
                numericalWarnings++;
            }
        }
    }

    [[nodiscard]] glm::dvec3 computeGravitationalFieldAt(const glm::dvec3& point, const std::vector<CelestialBody>& bodies) const {
        glm::dvec3 field(0.0);
        double eps2 = softening * softening;

        for (const auto& b : bodies) {
            if (b.isDestroyed) continue;
            glm::dvec3 rVec = b.position - point;
            double distSq = glm::dot(rVec, rVec);
            double denom = std::pow(distSq + eps2, 1.5);
            if (denom > 1e-12) {
                field += rVec * (Constants::G * b.mass / denom);
            }
        }
        return field;
    }

    [[nodiscard]] double computeGravitationalPotentialAt(const glm::dvec3& point, const std::vector<CelestialBody>& bodies) const {
        double potential = 0.0;
        double eps2 = softening * softening;

        for (const auto& b : bodies) {
            if (b.isDestroyed) continue;
            double dist = std::sqrt(glm::dot(b.position - point, b.position - point) + eps2);
            if (dist > 1e-6) {
                potential -= (Constants::G * b.mass) / dist;
            }
        }
        return potential;
    }
};

} // namespace Orbital
