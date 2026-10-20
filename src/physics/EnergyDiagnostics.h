#pragma once

#include "CelestialBody.h"
#include <vector>
#include <deque>
#include <cmath>

namespace Orbital {

struct SystemDiagnostics {
    double kineticEnergy = 0.0;       // Joules
    double potentialEnergy = 0.0;     // Joules
    double totalEnergy = 0.0;         // Joules
    double initialTotalEnergy = 0.0;  // Reference at t=0
    double energyDrift = 0.0;         // Relative drift (E - E0) / |E0|

    glm::dvec3 totalLinearMomentum{0.0};  // kg * m / s
    glm::dvec3 totalAngularMomentum{0.0}; // kg * m^2 / s
    glm::dvec3 centerOfMass{0.0};         // m
    double totalMass = 0.0;               // kg
    size_t activeBodyCount = 0;
};

class EnergyDiagnostics {
public:
    SystemDiagnostics current;
    bool hasInitialEnergy = false;

    // Rolling history for live scientific graphs
    std::deque<float> timeHistory;
    std::deque<float> energyHistory;
    std::deque<float> driftHistory;
    size_t maxHistoryPoints = 200;

    void reset() {
        hasInitialEnergy = false;
        current = SystemDiagnostics{};
        timeHistory.clear();
        energyHistory.clear();
        driftHistory.clear();
    }

    void update(const std::vector<CelestialBody>& bodies, double simTime) {
        current.kineticEnergy = 0.0;
        current.potentialEnergy = 0.0;
        current.totalLinearMomentum = glm::dvec3(0.0);
        current.totalAngularMomentum = glm::dvec3(0.0);
        current.centerOfMass = glm::dvec3(0.0);
        current.totalMass = 0.0;
        current.activeBodyCount = 0;

        size_t n = bodies.size();

        // 1. Kinetic energy, linear momentum, angular momentum, center of mass
        for (size_t i = 0; i < n; ++i) {
            if (bodies[i].isDestroyed) continue;
            current.activeBodyCount++;

            const auto& b = bodies[i];
            current.totalMass += b.mass;
            current.centerOfMass += b.position * b.mass;

            double speedSq = glm::dot(b.velocity, b.velocity);
            current.kineticEnergy += 0.5 * b.mass * speedSq;

            current.totalLinearMomentum += b.velocity * b.mass;
            current.totalAngularMomentum += glm::cross(b.position, b.velocity) * b.mass;

            // 2. Pairwise Gravitational Potential Energy U = -G * m1 * m2 / r (unique pairs only!)
            for (size_t j = i + 1; j < n; ++j) {
                if (bodies[j].isDestroyed) continue;
                double dist = glm::length(bodies[j].position - b.position);
                if (dist > 1e-3) {
                    current.potentialEnergy -= (Constants::G * b.mass * bodies[j].mass) / dist;
                }
            }
        }

        if (current.totalMass > 0.0) {
            current.centerOfMass /= current.totalMass;
        }

        current.totalEnergy = current.kineticEnergy + current.potentialEnergy;

        if (!hasInitialEnergy && current.activeBodyCount > 0) {
            current.initialTotalEnergy = current.totalEnergy;
            hasInitialEnergy = true;
        }

        if (hasInitialEnergy && std::abs(current.initialTotalEnergy) > 1e-9) {
            current.energyDrift = (current.totalEnergy - current.initialTotalEnergy) / std::abs(current.initialTotalEnergy);
        } else {
            current.energyDrift = 0.0;
        }

        // Store sample for plots
        timeHistory.push_back(static_cast<float>(simTime));
        energyHistory.push_back(static_cast<float>(current.totalEnergy * 1e-32)); // Scaled for GUI plot
        driftHistory.push_back(static_cast<float>(current.energyDrift * 100.0)); // in percent

        if (timeHistory.size() > maxHistoryPoints) {
            timeHistory.pop_front();
            energyHistory.pop_front();
            driftHistory.pop_front();
        }
    }
};

} // namespace Orbital
