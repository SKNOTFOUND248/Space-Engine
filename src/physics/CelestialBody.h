#pragma once

#include "../core/Types.h"
#include <glm/glm.hpp>
#include <string>
#include <deque>
#include <cmath>
#include <numbers>

namespace Orbital {

struct CelestialBody {
    BodyID id = INVALID_BODY_ID;
    std::string name = "Unnamed Body";
    BodyType type = BodyType::Planet;

    // Physical State in SI Units
    double mass = Constants::EarthMass;            // kg
    double radius = Constants::EarthRadius;        // m
    glm::dvec3 position{0.0, 0.0, 0.0};            // m
    glm::dvec3 velocity{0.0, 0.0, 0.0};            // m/s
    glm::dvec3 acceleration{0.0, 0.0, 0.0};        // m/s^2
    glm::dvec3 prevAcceleration{0.0, 0.0, 0.0};    // For Verlet integration

    // Rotation & Orientation
    double rotationAngle = 0.0;                    // radians
    double rotationSpeed = (2.0 * std::numbers::pi) / Constants::DayInSeconds; // rad/s
    double axialTilt = 23.44;                      // degrees

    // Physical flags
    bool isPinned = false;                         // Explicit pinned body mode (forces apply to others, not this)
    bool isDestroyed = false;                      // Marked for cleanup/merger

    // Visual & Atmospheric properties
    glm::vec3 color{0.3f, 0.6f, 0.9f};             // Base albedo color
    float roughness = 0.6f;
    float metallic = 0.0f;
    float emission = 0.0f;                         // > 0 for stars/cores

    bool hasAtmosphere = false;
    glm::vec3 atmosphereColor{0.4f, 0.7f, 1.0f};
    float atmosphereDensity = 1.0f;

    // Procedural generation parameters
    int biomeType = 0; // 0=Terrestrial, 1=Desert, 2=Ocean, 3=Ice, 4=Volcanic, 5=GasGiant
    float seed = 42.0f;
    float seaLevel = 0.48f;
    float cloudCoverage = 0.45f;

    bool hasRings = false;
    double ringInnerRadius = 0.0;                  // m
    double ringOuterRadius = 0.0;                  // m
    glm::vec4 ringColor{0.8f, 0.7f, 0.55f, 0.8f};

    // Trajectory History Trail
    std::deque<glm::dvec3> trail;
    size_t maxTrailPoints = 300;
    double trailSampleTimer = 0.0;
    double trailSampleInterval = 600.0;            // Every 600s of simulation time

    // Scientific Calculations
    [[nodiscard]] double getSurfaceGravity() const {
        if (radius <= 0.0) return 0.0;
        return (Constants::G * mass) / (radius * radius);
    }

    [[nodiscard]] double getEscapeVelocity() const {
        if (radius <= 0.0) return 0.0;
        return std::sqrt((2.0 * Constants::G * mass) / radius);
    }

    [[nodiscard]] double getMeanDensity() const {
        if (radius <= 0.0) return 0.0;
        double volume = (4.0 / 3.0) * std::numbers::pi * std::pow(radius, 3);
        return mass / volume;
    }

    [[nodiscard]] double getGravitationalParameter() const {
        return Constants::G * mass; // mu = GM
    }

    [[nodiscard]] double getSchwarzschildRadius() const {
        // r_s = 2 * G * M / c^2
        return (2.0 * Constants::G * mass) / Constants::c2;
    }

    [[nodiscard]] double getPhotonSphereRadius() const {
        return 1.5 * getSchwarzschildRadius();
    }

    [[nodiscard]] double getISCORadius() const {
        return 3.0 * getSchwarzschildRadius();
    }

    void addTrailPoint(const glm::dvec3& pos) {
        trail.push_back(pos);
        if (trail.size() > maxTrailPoints) {
            trail.pop_front();
        }
    }
};

} // namespace Orbital
