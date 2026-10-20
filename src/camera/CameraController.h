#pragma once

#include "../core/Types.h"
#include "../physics/CelestialBody.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Orbital {

enum class CameraMode {
    OrbitTarget,
    FreeFly
};

class CameraController {
public:
    CameraMode mode = CameraMode::OrbitTarget;
    ScaleMode scaleMode = ScaleMode::LaboratoryScale;

    // Camera world position in double precision SI meters
    glm::dvec3 worldPosition{0.0, 1.5e11, 3.5e11};
    glm::dvec3 targetPosition{0.0, 0.0, 0.0};

    // Orbit parameters
    double orbitDistance = 4.0e11; // meters
    double orbitYaw = -90.0;       // degrees
    double orbitPitch = 25.0;      // degrees
    double minOrbitDistance = 1.0e6;
    double maxOrbitDistance = 2.0e13;

    // Free fly orientation
    glm::dvec3 freeFlyFront{0.0, 0.0, -1.0};
    glm::dvec3 freeFlyUp{0.0, 1.0, 0.0};
    double freeFlySpeed = 1.0e9;   // meters per second

    float fov = 45.0f;             // degrees
    float nearPlane = 0.01f;
    float farPlane = 10000.0f;

    // Visual distance unit scale: 1 world display unit = 1e9 meters (1 million kilometers)
    static constexpr double WorldUnitToMeters = 1.0e9;
    static constexpr double MetersToWorldUnits = 1.0 / WorldUnitToMeters;

    void update(const CelestialBody* target, double dt) {
        if (mode == CameraMode::OrbitTarget && target != nullptr) {
            targetPosition = target->position;

            // Compute orbit position from spherical coordinates around target
            double yawRad = glm::radians(orbitYaw);
            double pitchRad = glm::radians(orbitPitch);

            glm::dvec3 offset;
            offset.x = orbitDistance * std::cos(pitchRad) * std::cos(yawRad);
            offset.y = orbitDistance * std::sin(pitchRad);
            offset.z = orbitDistance * std::cos(pitchRad) * std::sin(yawRad);

            worldPosition = targetPosition + offset;
        }
    }

    void processOrbitRotation(double xOffset, double yOffset) {
        if (mode == CameraMode::OrbitTarget) {
            orbitYaw += xOffset * 0.25;
            orbitPitch += yOffset * 0.25;
            orbitPitch = std::clamp(orbitPitch, -89.0, 89.0);
        }
    }

    void processZoom(double yOffset) {
        // Exponential zoom scaling for smooth navigation across orders of magnitude
        double factor = (yOffset > 0) ? 0.85 : 1.18;
        orbitDistance = std::clamp(orbitDistance * factor, minOrbitDistance, maxOrbitDistance);
    }

    void focusOnBody(const CelestialBody& body) {
        targetPosition = body.position;
        // Place orbit distance proportional to body radius
        if (scaleMode == ScaleMode::LaboratoryScale) {
            orbitDistance = std::max(body.radius * 20.0, 2.0e9);
        } else {
            orbitDistance = std::max(body.radius * 6.0, 1.0e7);
        }
    }

    // Calculates camera-relative float matrix for rendering (floating origin)
    [[nodiscard]] glm::mat4 getViewMatrix() const {
        glm::vec3 camPosView(0.0f); // Floating origin: camera is always at origin in render space
        glm::dvec3 lookDir = glm::normalize(targetPosition - worldPosition);
        glm::vec3 front = glm::vec3(lookDir);
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        if (mode == CameraMode::OrbitTarget) {
            return glm::lookAt(camPosView, front, up);
        } else {
            return glm::lookAt(camPosView, glm::vec3(freeFlyFront), glm::vec3(freeFlyUp));
        }
    }

    [[nodiscard]] glm::mat4 getProjectionMatrix(float aspectRatio) const {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    // Converts a double-precision world position (m) into camera-relative render space (float)
    [[nodiscard]] glm::vec3 toRenderSpace(const glm::dvec3& worldPos) const {
        glm::dvec3 rel = (worldPos - worldPosition) * MetersToWorldUnits;
        return glm::vec3(rel);
    }

    // Converts physical body radius into visual display radius according to active ScaleMode
    [[nodiscard]] float getVisualRadius(const CelestialBody& body) const {
        if (scaleMode == ScaleMode::TruePhysical) {
            return static_cast<float>(body.radius * MetersToWorldUnits);
        } else {
            // Enhanced visual scale: log-based scaling so Earth, Mars, and Moon can be seen clearly in solar view
            if (body.type == BodyType::Star) {
                return static_cast<float>(std::log10(body.radius + 1.0) * 0.45);
            } else if (body.type == BodyType::BlackHole) {
                return static_cast<float>(std::log10(body.getSchwarzschildRadius() + 1.0) * 0.35);
            } else if (body.type == BodyType::Moon || body.type == BodyType::Asteroid || body.type == BodyType::Spacecraft) {
                return static_cast<float>(std::pow(body.radius / 1.0e6, 0.40) * 0.18 + 0.08);
            } else {
                // Planets
                return static_cast<float>(std::pow(body.radius / 1.0e6, 0.45) * 0.32 + 0.15);
            }
        }
    }
};

} // namespace Orbital
