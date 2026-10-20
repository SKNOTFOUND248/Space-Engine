#pragma once

#include "CelestialBody.h"
#include <glm/glm.hpp>
#include <cmath>
#include <numbers>
#include <vector>

namespace Orbital {

enum class OrbitType {
    Circular,
    Elliptical,
    Parabolic,
    Hyperbolic,
    Undefined
};

struct OrbitalElements {
    OrbitType type = OrbitType::Undefined;
    double semiMajorAxis = 0.0;       // a (meters)
    double eccentricity = 0.0;        // e (dimensionless)
    double inclination = 0.0;         // i (degrees)
    double periapsis = 0.0;           // r_p (meters)
    double apoapsis = 0.0;            // r_a (meters, NaN for hyperbolic)
    double period = 0.0;              // T (seconds, only valid for bound orbits)
    double specificEnergy = 0.0;      // E (J/kg)
    double specificAngularMomentum = 0.0; // h (m^2/s)
    double currentDistance = 0.0;     // r (meters)
    double currentSpeed = 0.0;        // v (m/s)
    bool isBound = false;
};

class OrbitAnalyzer {
public:
    static OrbitalElements calculate(const CelestialBody& body, const CelestialBody& primary) {
        OrbitalElements elem;
        double mu = Constants::G * (primary.mass + body.mass);
        if (mu <= 0.0) return elem;

        glm::dvec3 rVec = body.position - primary.position;
        glm::dvec3 vVec = body.velocity - primary.velocity;

        double r = glm::length(rVec);
        double v = glm::length(vVec);
        elem.currentDistance = r;
        elem.currentSpeed = v;

        if (r < 1e-3) return elem;

        // Specific mechanical energy E = v^2/2 - mu/r
        double energy = (0.5 * v * v) - (mu / r);
        elem.specificEnergy = energy;

        // Specific angular momentum h = r x v
        glm::dvec3 hVec = glm::cross(rVec, vVec);
        double h = glm::length(hVec);
        elem.specificAngularMomentum = h;

        // Eccentricity vector e = (v x h)/mu - r/|r|
        glm::dvec3 eVec = (glm::cross(vVec, hVec) / mu) - (rVec / r);
        double e = glm::length(eVec);
        elem.eccentricity = e;

        // Semi-major axis a = -mu / (2 * E)
        if (std::abs(energy) > 1e-12) {
            elem.semiMajorAxis = -mu / (2.0 * energy);
        } else {
            elem.semiMajorAxis = 1e20; // Parabolic limit
        }

        // Inclination i = acos(hz / h)
        if (h > 1e-12) {
            double cosI = std::clamp(hVec.y / h, -1.0, 1.0); // Assuming Y is up in 3D scene
            elem.inclination = glm::degrees(std::acos(cosI));
        }

        elem.periapsis = elem.semiMajorAxis * (1.0 - e);

        // Orbit classification
        if (e < 1e-4) {
            elem.type = OrbitType::Circular;
            elem.isBound = true;
            elem.apoapsis = elem.periapsis;
            elem.period = 2.0 * std::numbers::pi * std::sqrt(std::pow(elem.semiMajorAxis, 3) / mu);
        } else if (e < 1.0) {
            elem.type = OrbitType::Elliptical;
            elem.isBound = true;
            elem.apoapsis = elem.semiMajorAxis * (1.0 + e);
            elem.period = 2.0 * std::numbers::pi * std::sqrt(std::pow(elem.semiMajorAxis, 3) / mu);
        } else if (std::abs(e - 1.0) < 1e-3) {
            elem.type = OrbitType::Parabolic;
            elem.isBound = false;
            elem.apoapsis = std::numeric_limits<double>::infinity();
            elem.period = 0.0;
        } else {
            elem.type = OrbitType::Hyperbolic;
            elem.isBound = false;
            elem.apoapsis = std::numeric_limits<double>::infinity();
            elem.period = 0.0;
        }

        return elem;
    }

    // Generates 3D points representing the predicted Keplerian orbit ellipse / conic section
    static std::vector<glm::dvec3> generateConicPoints(const CelestialBody& body, const CelestialBody& primary, int numSegments = 180) {
        std::vector<glm::dvec3> points;
        double mu = Constants::G * (primary.mass + body.mass);
        if (mu <= 0.0) return points;

        glm::dvec3 rVec = body.position - primary.position;
        glm::dvec3 vVec = body.velocity - primary.velocity;
        double r = glm::length(rVec);
        if (r < 1e-3) return points;

        glm::dvec3 hVec = glm::cross(rVec, vVec);
        double h = glm::length(hVec);
        if (h < 1e-9) return points; // Radial plunge trajectory

        glm::dvec3 eVec = (glm::cross(vVec, hVec) / mu) - (rVec / r);
        double e = glm::length(eVec);

        glm::dvec3 P = (e > 1e-6) ? glm::normalize(eVec) : glm::normalize(rVec);
        glm::dvec3 W = glm::normalize(hVec);
        glm::dvec3 Q = glm::cross(W, P);

        // Parameter p = h^2 / mu
        double p = (h * h) / mu;

        double thetaMax = std::numbers::pi;
        if (e >= 1.0) {
            // Hyperbolic true anomaly asymptote
            thetaMax = std::acos(-1.0 / e) * 0.90;
        }

        points.reserve(numSegments + 1);
        double startAngle = (e < 1.0) ? 0.0 : -thetaMax;
        double endAngle = (e < 1.0) ? 2.0 * std::numbers::pi : thetaMax;
        double step = (endAngle - startAngle) / static_cast<double>(numSegments);

        for (int i = 0; i <= numSegments; ++i) {
            double theta = startAngle + static_cast<double>(i) * step;
            double radius = p / (1.0 + e * std::cos(theta));
            if (radius > 0.0 && radius < 1e14) {
                glm::dvec3 orbitPos = primary.position + (P * std::cos(theta) + Q * std::sin(theta)) * radius;
                points.push_back(orbitPos);
            }
        }

        return points;
    }
};

} // namespace Orbital
