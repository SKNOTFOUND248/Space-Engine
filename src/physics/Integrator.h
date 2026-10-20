#pragma once

#include "CelestialBody.h"
#include "GravitySolver.h"
#include <vector>

namespace Orbital {

class Integrator {
public:
    static void step(IntegratorType type, std::vector<CelestialBody>& bodies, GravitySolver& solver, double dt) {
        switch (type) {
            case IntegratorType::VelocityVerlet:
                stepVelocityVerlet(bodies, solver, dt);
                break;
            case IntegratorType::SemiImplicitEuler:
                stepSemiImplicitEuler(bodies, solver, dt);
                break;
            case IntegratorType::RungeKutta4:
                stepRK4(bodies, solver, dt);
                break;
        }

        // Update rotation for each body
        for (auto& b : bodies) {
            if (!b.isDestroyed) {
                b.rotationAngle += b.rotationSpeed * dt;
                if (b.rotationAngle > 2.0 * std::numbers::pi) {
                    b.rotationAngle = std::fmod(b.rotationAngle, 2.0 * std::numbers::pi);
                }
            }
        }
    }

private:
    static void stepVelocityVerlet(std::vector<CelestialBody>& bodies, GravitySolver& solver, double dt) {
        // Step 1: Update positions using current velocity and acceleration
        for (auto& b : bodies) {
            if (b.isDestroyed || b.isPinned) continue;
            b.position += b.velocity * dt + 0.5 * b.acceleration * (dt * dt);
            b.prevAcceleration = b.acceleration;
        }

        // Step 2: Compute new accelerations at updated positions
        solver.computeAccelerations(bodies);

        // Step 3: Update velocities using average of previous and new accelerations
        for (auto& b : bodies) {
            if (b.isDestroyed || b.isPinned) continue;
            b.velocity += 0.5 * (b.prevAcceleration + b.acceleration) * dt;
        }
    }

    static void stepSemiImplicitEuler(std::vector<CelestialBody>& bodies, GravitySolver& solver, double dt) {
        solver.computeAccelerations(bodies);

        for (auto& b : bodies) {
            if (b.isDestroyed || b.isPinned) continue;
            b.velocity += b.acceleration * dt;
            b.position += b.velocity * dt;
        }
    }

    static void stepRK4(std::vector<CelestialBody>& bodies, GravitySolver& solver, double dt) {
        size_t n = bodies.size();
        std::vector<glm::dvec3> origPos(n), origVel(n);
        for (size_t i = 0; i < n; ++i) {
            origPos[i] = bodies[i].position;
            origVel[i] = bodies[i].velocity;
        }

        // k1
        solver.computeAccelerations(bodies);
        std::vector<glm::dvec3> k1_v(n), k1_a(n);
        for (size_t i = 0; i < n; ++i) {
            k1_v[i] = origVel[i];
            k1_a[i] = bodies[i].acceleration;
        }

        // k2
        for (size_t i = 0; i < n; ++i) {
            if (bodies[i].isDestroyed || bodies[i].isPinned) continue;
            bodies[i].position = origPos[i] + k1_v[i] * (0.5 * dt);
        }
        solver.computeAccelerations(bodies);
        std::vector<glm::dvec3> k2_v(n), k2_a(n);
        for (size_t i = 0; i < n; ++i) {
            k2_v[i] = origVel[i] + k1_a[i] * (0.5 * dt);
            k2_a[i] = bodies[i].acceleration;
        }

        // k3
        for (size_t i = 0; i < n; ++i) {
            if (bodies[i].isDestroyed || bodies[i].isPinned) continue;
            bodies[i].position = origPos[i] + k2_v[i] * (0.5 * dt);
        }
        solver.computeAccelerations(bodies);
        std::vector<glm::dvec3> k3_v(n), k3_a(n);
        for (size_t i = 0; i < n; ++i) {
            k3_v[i] = origVel[i] + k2_a[i] * (0.5 * dt);
            k3_a[i] = bodies[i].acceleration;
        }

        // k4
        for (size_t i = 0; i < n; ++i) {
            if (bodies[i].isDestroyed || bodies[i].isPinned) continue;
            bodies[i].position = origPos[i] + k3_v[i] * dt;
        }
        solver.computeAccelerations(bodies);
        std::vector<glm::dvec3> k4_v(n), k4_a(n);
        for (size_t i = 0; i < n; ++i) {
            k4_v[i] = origVel[i] + k3_a[i] * dt;
            k4_a[i] = bodies[i].acceleration;
        }

        // Final weighted update
        for (size_t i = 0; i < n; ++i) {
            if (bodies[i].isDestroyed || bodies[i].isPinned) continue;
            bodies[i].position = origPos[i] + (dt / 6.0) * (k1_v[i] + 2.0 * k2_v[i] + 2.0 * k3_v[i] + k4_v[i]);
            bodies[i].velocity = origVel[i] + (dt / 6.0) * (k1_a[i] + 2.0 * k2_a[i] + 2.0 * k3_a[i] + k4_a[i]);
        }
    }
};

} // namespace Orbital
