#pragma once

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace Orbital {

// Stable Unique Identifier for Celestial Bodies
using BodyID = uint64_t;
inline constexpr BodyID INVALID_BODY_ID = 0;

// Fundamental Physical Constants in SI Units (kg, m, s, rad)
namespace Constants {
    // Gravitational constant (CODATA 2018) [m^3 kg^-1 s^-2]
    inline constexpr double G = 6.67430e-11;

    // Speed of light in vacuum [m/s]
    inline constexpr double c = 299792458.0;
    inline constexpr double c2 = c * c;

    // Astronomical standard values
    inline constexpr double SolarMass     = 1.98847e30;    // kg
    inline constexpr double SolarRadius   = 6.95700e8;     // m
    inline constexpr double EarthMass     = 5.9722e24;     // kg
    inline constexpr double EarthRadius   = 6.3710e6;      // m
    inline constexpr double MoonMass      = 7.342e22;      // kg
    inline constexpr double MoonRadius    = 1.7374e6;      // m
    inline constexpr double JupiterMass   = 1.89813e27;    // kg
    inline constexpr double JupiterRadius = 6.9911e7;      // m
    inline constexpr double AU            = 1.495978707e11;// m (1 Astronomical Unit)
    inline constexpr double DayInSeconds  = 86400.0;       // s
    inline constexpr double YearInSeconds = 365.25 * DayInSeconds;
}

// Celestial Body Classification
enum class BodyType {
    Star,
    Planet,
    Moon,
    Asteroid,
    Comet,
    BlackHole,
    Spacecraft,
    Debris
};

inline const char* GetBodyTypeName(BodyType type) {
    switch (type) {
        case BodyType::Star:       return "Star";
        case BodyType::Planet:     return "Planet";
        case BodyType::Moon:       return "Moon";
        case BodyType::Asteroid:   return "Asteroid";
        case BodyType::Comet:      return "Comet";
        case BodyType::BlackHole:  return "Black Hole";
        case BodyType::Spacecraft: return "Spacecraft / Probe";
        case BodyType::Debris:     return "Impact Debris";
    }
    return "Unknown";
}

// Numerical Integration Methods
enum class IntegratorType {
    VelocityVerlet,    // 2nd-order symplectic method (ideal for long-term energy conservation)
    SemiImplicitEuler, // 1st-order symplectic Euler
    RungeKutta4        // 4th-order non-symplectic (high short-term accuracy, drift over many orbits)
};

inline const char* GetIntegratorName(IntegratorType type) {
    switch (type) {
        case IntegratorType::VelocityVerlet:    return "Velocity Verlet (Symplectic 2nd-Order)";
        case IntegratorType::SemiImplicitEuler: return "Semi-Implicit Euler (1st-Order)";
        case IntegratorType::RungeKutta4:       return "Runge-Kutta 4 (RK4 4th-Order)";
    }
    return "Unknown";
}

// Collision Handling Policies
enum class CollisionPolicy {
    MergeCoalesce, // Bodies merge into one, conserving mass and linear momentum
    ElasticBounce, // Bodies bounce using coefficient of restitution
    Ignore         // Passes through (useful for ghost test probes)
};

inline const char* GetCollisionPolicyName(CollisionPolicy policy) {
    switch (policy) {
        case CollisionPolicy::MergeCoalesce: return "Inelastic Merger (Coalesce)";
        case CollisionPolicy::ElasticBounce: return "Elastic / Restitution Bounce";
        case CollisionPolicy::Ignore:        return "Pass-Through (Ghost)";
    }
    return "Unknown";
}

// Rendering Scale Modes
enum class ScaleMode {
    LaboratoryScale, // Enhanced visual sizes so planets and moons can be inspected without giant telescopes
    TruePhysical     // Exact 1:1 astronomical proportions (planets appear as dots at solar distances)
};

} // namespace Orbital
