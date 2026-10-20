#pragma once

#include "CelestialBody.h"
#include <vector>
#include <string>
#include <deque>
#include <cmath>
#include <iostream>

namespace Orbital {

struct CollisionEvent {
    double timestamp = 0.0;
    std::string body1Name;
    std::string body2Name;
    glm::dvec3 position{0.0};
    double relativeVelocity = 0.0; // m/s
    double kineticEnergyReleased = 0.0; // Joules
    std::string outcome;
};

class CollisionSystem {
public:
    CollisionPolicy policy = CollisionPolicy::MergeCoalesce;
    double restitution = 0.2; // Coefficient of restitution for bounce policy (0 = perfectly inelastic, 1 = elastic)
    uint64_t totalCollisionCount = 0;
    std::deque<CollisionEvent> eventLog;
    size_t maxLogEntries = 50;

    void detectAndResolve(std::vector<CelestialBody>& bodies, double currentTime) {
        size_t n = bodies.size();

        for (size_t i = 0; i < n; ++i) {
            if (bodies[i].isDestroyed) continue;

            for (size_t j = i + 1; j < n; ++j) {
                if (bodies[j].isDestroyed) continue;

                glm::dvec3 rVec = bodies[j].position - bodies[i].position;
                double dist = glm::length(rVec);
                double touchDist = bodies[i].radius + bodies[j].radius;

                if (dist < touchDist && dist > 1e-6) {
                    handleCollision(bodies[i], bodies[j], dist, rVec, currentTime);
                }
            }
        }

        // Clean up destroyed bodies
        bodies.erase(
            std::remove_if(bodies.begin(), bodies.end(), [](const CelestialBody& b) {
                return b.isDestroyed;
            }),
            bodies.end()
        );
    }

private:
    void handleCollision(CelestialBody& b1, CelestialBody& b2, double dist, const glm::dvec3& rVec, double currentTime) {
        totalCollisionCount++;

        glm::dvec3 vRel = b1.velocity - b2.velocity;
        double relSpeed = glm::length(vRel);

        // Reduced mass mu = m1 * m2 / (m1 + m2)
        double reducedMass = (b1.mass * b2.mass) / (b1.mass + b2.mass);
        double impactEnergy = 0.5 * reducedMass * relSpeed * relSpeed; // Joules

        CollisionEvent evt;
        evt.timestamp = currentTime;
        evt.body1Name = b1.name;
        evt.body2Name = b2.name;
        evt.position = (b1.position + b2.position) * 0.5;
        evt.relativeVelocity = relSpeed;
        evt.kineticEnergyReleased = impactEnergy;

        if (policy == CollisionPolicy::MergeCoalesce) {
            // Determine primary (larger mass absorbs smaller)
            CelestialBody* primary = (b1.mass >= b2.mass) ? &b1 : &b2;
            CelestialBody* secondary = (b1.mass >= b2.mass) ? &b2 : &b1;

            double totalMass = primary->mass + secondary->mass;

            // Center of mass position: r_com = (m1*r1 + m2*r2) / totalMass
            glm::dvec3 comPos = (primary->position * primary->mass + secondary->position * secondary->mass) / totalMass;

            // Conservation of linear momentum: v = (m1*v1 + m2*v2) / totalMass
            glm::dvec3 comVel = (primary->velocity * primary->mass + secondary->velocity * secondary->mass) / totalMass;

            // Volume-conserving effective radius: R_merged = (R1^3 + R2^3)^(1/3)
            double mergedRadius = std::cbrt(std::pow(primary->radius, 3) + std::pow(secondary->radius, 3));

            primary->position = comPos;
            primary->velocity = comVel;
            primary->mass = totalMass;
            primary->radius = mergedRadius;

            // Mark secondary as destroyed
            secondary->isDestroyed = true;

            evt.outcome = "Merged into " + primary->name;
            std::cout << "[COLLISION] " << b1.name << " & " << b2.name 
                      << " merged! Energy: " << impactEnergy << " J\n";
        }
        else if (policy == CollisionPolicy::ElasticBounce) {
            glm::dvec3 normal = rVec / dist;
            double vRelDotNormal = glm::dot(vRel, normal);

            if (vRelDotNormal < 0.0) { // Moving towards each other
                double impulseMag = -(1.0 + restitution) * vRelDotNormal / (1.0 / b1.mass + 1.0 / b2.mass);
                glm::dvec3 impulse = normal * impulseMag;

                if (!b1.isPinned) b1.velocity += impulse / b1.mass;
                if (!b2.isPinned) b2.velocity -= impulse / b2.mass;

                // Positional separation to prevent sticking
                double overlap = (b1.radius + b2.radius) - dist;
                if (!b1.isPinned) b1.position -= normal * (overlap * 0.5);
                if (!b2.isPinned) b2.position += normal * (overlap * 0.5);
            }
            evt.outcome = "Bounced with e=" + std::to_string(restitution);
        }
        else {
            evt.outcome = "Passed Through";
        }

        eventLog.push_front(evt);
        if (eventLog.size() > maxLogEntries) {
            eventLog.pop_back();
        }
    }
};

} // namespace Orbital
