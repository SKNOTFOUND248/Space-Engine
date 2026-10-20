#pragma once

#include "../core/Types.h"
#include <glm/glm.hpp>
#include <cmath>
#include <string>

namespace Orbital {

struct RelativisticMetrics {
    double mass = 0.0;                 // kg
    double schwarzschildRadius = 0.0;  // r_s = 2GM / c^2 (meters)
    double photonSphereRadius = 0.0;   // r_ph = 1.5 r_s (meters)
    double iscoRadius = 0.0;           // r_ISCO = 3.0 r_s (meters)
    double gravitationalRadius = 0.0;  // r_g = GM / c^2 (meters)
};

class RelativitySolver {
public:
    // Computes analytical relativistic metrics for a static (Schwarzschild) black hole
    static RelativisticMetrics computeMetrics(double mass) {
        RelativisticMetrics m;
        m.mass = mass;
        m.gravitationalRadius = (Constants::G * mass) / Constants::c2;
        m.schwarzschildRadius = 2.0 * m.gravitationalRadius;
        m.photonSphereRadius = 1.5 * m.schwarzschildRadius;
        m.iscoRadius = 3.0 * m.schwarzschildRadius;
        return m;
    }

    // Gravitational time dilation factor d(tau)/dt for a stationary observer at distance r
    // Returns fraction between 0.0 (at horizon) and 1.0 (at infinity)
    static double computeTimeDilation(double mass, double r) {
        double rs = (2.0 * Constants::G * mass) / Constants::c2;
        if (r <= rs) return 0.0; // Frozen at horizon to an asymptotic observer
        return std::sqrt(1.0 - (rs / r));
    }

    // Gravitational spectral redshift z = 1 / sqrt(1 - rs/r) - 1
    static double computeGravitationalRedshift(double mass, double r) {
        double rs = (2.0 * Constants::G * mass) / Constants::c2;
        if (r <= rs) return std::numeric_limits<double>::infinity();
        return (1.0 / std::sqrt(1.0 - (rs / r))) - 1.0;
    }

    // Tidal acceleration estimate across an object of length deltaR: a_tidal = (2 * G * M / r^3) * deltaR
    static double computeTidalAcceleration(double mass, double r, double deltaR = 2.0) {
        if (r <= 0.0) return 0.0;
        return ((2.0 * Constants::G * mass) / std::pow(r, 3)) * deltaR;
    }

    // Post-Newtonian (1PN) acceleration correction for relativistic periapsis precession
    // Delta a_GR = (G * M / (c^2 * r^3)) * [ (4 * G * M / r - v^2) * r_vec + 4 * (r_vec . v_vec) * v_vec ]
    static glm::dvec3 computePostNewtonianCorrection(const glm::dvec3& rVec, const glm::dvec3& vVec, double primaryMass) {
        double r = glm::length(rVec);
        if (r < 1e3) return glm::dvec3(0.0);

        double GM = Constants::G * primaryMass;
        double c2 = Constants::c2;
        double v2 = glm::dot(vVec, vVec);
        double rDotV = glm::dot(rVec, vVec);

        double term1 = (4.0 * GM / r) - v2;
        glm::dvec3 a_1pn = (GM / (c2 * std::pow(r, 3))) * (term1 * rVec + 4.0 * rDotV * vVec);
        return a_1pn;
    }
};

} // namespace Orbital
