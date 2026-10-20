#pragma once

#include "../physics/CelestialBody.h"
#include "../physics/GravitySolver.h"
#include "../physics/Integrator.h"
#include "../physics/CollisionSystem.h"
#include "../physics/OrbitAnalyzer.h"
#include "../physics/EnergyDiagnostics.h"
#include "../relativity/RelativitySolver.h"
#include "../core/Time.h"

#include <vector>
#include <memory>
#include <string>
#include <algorithm>

namespace Orbital {

enum class ScenarioPreset {
    SolarSystem,
    TwoBodyLaboratory,
    ThreeBodyChaotic,
    BinaryStarSystem,
    PlanetaryCollision,
    SupermassiveBlackHole,
    MassVsGravityLab,
    RelativisticPrecession,
    EmptyUniverse
};

inline const char* GetScenarioName(ScenarioPreset s) {
    switch (s) {
        case ScenarioPreset::SolarSystem:            return "Complete Solar System";
        case ScenarioPreset::TwoBodyLaboratory:      return "Two-Body Keplerian Orbit";
        case ScenarioPreset::ThreeBodyChaotic:       return "Three-Body Gravitational Chaos";
        case ScenarioPreset::BinaryStarSystem:       return "Binary Star System";
        case ScenarioPreset::PlanetaryCollision:     return "Planetary Collision Experiment";
        case ScenarioPreset::SupermassiveBlackHole:  return "Supermassive Black Hole & Accretion Disk";
        case ScenarioPreset::MassVsGravityLab:       return "Mass vs. Gravity Laboratory";
        case ScenarioPreset::RelativisticPrecession: return "General Relativistic Precession (1PN)";
        case ScenarioPreset::EmptyUniverse:          return "Custom Empty Universe";
    }
    return "Unknown";
}

class Universe {
public:
    std::vector<CelestialBody> bodies;
    BodyID nextBodyId = 1;

    BodyID selectedBodyId = INVALID_BODY_ID;
    BodyID primaryBodyId = INVALID_BODY_ID;

    GravitySolver gravitySolver;
    CollisionSystem collisionSystem;
    EnergyDiagnostics energyDiagnostics;
    SimulationClock clock;

    IntegratorType integratorType = IntegratorType::VelocityVerlet;
    bool enableRelativisticCorrections = false; // Post-Newtonian precession

    Universe() {
        loadScenario(ScenarioPreset::SolarSystem);
    }

    BodyID addBody(CelestialBody body) {
        body.id = nextBodyId++;
        bodies.push_back(body);
        return body.id;
    }

    void removeBody(BodyID id) {
        bodies.erase(
            std::remove_if(bodies.begin(), bodies.end(), [id](const CelestialBody& b) {
                return b.id == id;
            }),
            bodies.end()
        );
        if (selectedBodyId == id) selectedBodyId = INVALID_BODY_ID;
        if (primaryBodyId == id) updatePrimaryBody();
    }

    CelestialBody* getBody(BodyID id) {
        for (auto& b : bodies) {
            if (b.id == id && !b.isDestroyed) return &b;
        }
        return nullptr;
    }

    const CelestialBody* getBody(BodyID id) const {
        for (const auto& b : bodies) {
            if (b.id == id && !b.isDestroyed) return &b;
        }
        return nullptr;
    }

    CelestialBody* getPrimaryBody() {
        return getBody(primaryBodyId);
    }

    const CelestialBody* getPrimaryBody() const {
        return getBody(primaryBodyId);
    }

    void updatePrimaryBody() {
        // Defaults to the most massive body
        double maxMass = -1.0;
        primaryBodyId = INVALID_BODY_ID;
        for (const auto& b : bodies) {
            if (!b.isDestroyed && b.mass > maxMass) {
                maxMass = b.mass;
                primaryBodyId = b.id;
            }
        }
    }

    void step(double dt) {
        if (bodies.empty()) return;

        // 1. Advance numerical integration of orbits
        Integrator::step(integratorType, bodies, gravitySolver, dt);

        // 2. Optional Post-Newtonian relativistic correction
        if (enableRelativisticCorrections && primaryBodyId != INVALID_BODY_ID) {
            const auto* primary = getPrimaryBody();
            if (primary) {
                for (auto& b : bodies) {
                    if (b.id != primary->id && !b.isDestroyed && !b.isPinned) {
                        glm::dvec3 rVec = b.position - primary->position;
                        glm::dvec3 vVec = b.velocity - primary->velocity;
                        glm::dvec3 a_gr = RelativitySolver::computePostNewtonianCorrection(rVec, vVec, primary->mass);
                        b.velocity += a_gr * dt;
                    }
                }
            }
        }

        // 3. Physical collisions
        collisionSystem.detectAndResolve(bodies, clock.simulatedTime);

        // 4. Update orbital trajectory trails
        for (auto& b : bodies) {
            b.trailSampleTimer += dt;
            if (b.trailSampleTimer >= b.trailSampleInterval) {
                b.trailSampleTimer = 0.0;
                b.addTrailPoint(b.position);
            }
        }

        // 5. Compute conservation laws & energy drift
        energyDiagnostics.update(bodies, clock.simulatedTime);
    }

    void update(double frameDeltaTime) {
        int substeps = clock.update(frameDeltaTime);
        for (int i = 0; i < substeps; ++i) {
            step(clock.fixedTimestep);
        }
    }

    void loadScenario(ScenarioPreset preset) {
        bodies.clear();
        nextBodyId = 1;
        selectedBodyId = INVALID_BODY_ID;
        primaryBodyId = INVALID_BODY_ID;
        clock.reset();
        energyDiagnostics.reset();

        switch (preset) {
            case ScenarioPreset::SolarSystem:
                buildSolarSystem();
                break;
            case ScenarioPreset::TwoBodyLaboratory:
                buildTwoBodyLab();
                break;
            case ScenarioPreset::ThreeBodyChaotic:
                buildThreeBodyChaos();
                break;
            case ScenarioPreset::BinaryStarSystem:
                buildBinaryStar();
                break;
            case ScenarioPreset::PlanetaryCollision:
                buildPlanetaryCollision();
                break;
            case ScenarioPreset::SupermassiveBlackHole:
                buildBlackHoleLab();
                break;
            case ScenarioPreset::MassVsGravityLab:
                buildMassVsGravityLab();
                break;
            case ScenarioPreset::RelativisticPrecession:
                buildRelativisticPrecession();
                break;
            case ScenarioPreset::EmptyUniverse:
                break;
        }

        updatePrimaryBody();
        if (!bodies.empty()) {
            selectedBodyId = bodies[0].id;
        }
    }

private:
    void buildSolarSystem() {
        clock.fixedTimestep = 3600.0; // 1 hour per step
        clock.timeScale = 86400.0;    // 1 day per real-world second

        // Sun
        CelestialBody sun;
        sun.name = "Sun";
        sun.type = BodyType::Star;
        sun.mass = Constants::SolarMass;
        sun.radius = Constants::SolarRadius;
        sun.position = glm::dvec3(0.0);
        sun.velocity = glm::dvec3(0.0);
        sun.color = glm::vec3(1.0f, 0.92f, 0.65f);
        sun.emission = 1.0f;
        addBody(sun);

        // Helper for circular planetary initial conditions: v = sqrt(G*M/r)
        auto addPlanet = [this](std::string name, double mass, double radius, double distAU, glm::vec3 color, int biome = 0, float seed = 42.0f, bool hasAtmosphere = false, glm::vec3 atmoCol = glm::vec3(0.0f)) -> BodyID {
            CelestialBody p;
            p.name = name;
            p.type = BodyType::Planet;
            p.mass = mass;
            p.radius = radius;
            double r = distAU * Constants::AU;
            p.position = glm::dvec3(r, 0.0, 0.0);
            double v = std::sqrt((Constants::G * Constants::SolarMass) / r);
            p.velocity = glm::dvec3(0.0, 0.0, v);
            p.color = color;
            p.biomeType = biome;
            p.seed = seed;
            p.hasAtmosphere = hasAtmosphere;
            p.atmosphereColor = atmoCol;
            return addBody(p);
        };

        // Mercury (Rocky/Desert, no atmosphere)
        addPlanet("Mercury", 3.3011e23, 2.4397e6, 0.387, glm::vec3(0.7f, 0.65f, 0.6f), 1, 101.0f);

        // Venus (Dense shrouded atmosphere)
        BodyID venusId = addPlanet("Venus", 4.8675e24, 6.0518e6, 0.723, glm::vec3(0.85f, 0.75f, 0.5f), 1, 202.0f, true, glm::vec3(0.9f, 0.8f, 0.6f));
        if (auto* v = getBody(venusId)) { v->cloudCoverage = 0.95f; v->atmosphereDensity = 2.5f; }

        // Earth & Moon
        BodyID earthId = addPlanet("Earth", Constants::EarthMass, Constants::EarthRadius, 1.0, glm::vec3(0.15f, 0.45f, 0.9f), 0, 303.0f, true, glm::vec3(0.3f, 0.7f, 1.0f));
        if (auto* e = getBody(earthId)) { e->seaLevel = 0.48f; e->cloudCoverage = 0.52f; e->atmosphereDensity = 1.0f; }
        const auto* earth = getBody(earthId);

        CelestialBody moon;
        moon.name = "Moon";
        moon.type = BodyType::Moon;
        moon.mass = Constants::MoonMass;
        moon.radius = Constants::MoonRadius;
        double moonDist = 3.844e8; // m
        moon.position = earth->position + glm::dvec3(0.0, 0.0, moonDist);
        double moonV = std::sqrt((Constants::G * Constants::EarthMass) / moonDist);
        moon.velocity = earth->velocity + glm::dvec3(moonV, 0.0, 0.0);
        moon.color = glm::vec3(0.75f, 0.75f, 0.78f);
        moon.biomeType = 1;
        moon.seed = 404.0f;
        addBody(moon);

        // Mars (Red desert dunes, thin atmosphere)
        BodyID marsId = addPlanet("Mars", 6.4171e23, 3.3895e6, 1.524, glm::vec3(0.88f, 0.38f, 0.22f), 1, 505.0f, true, glm::vec3(0.85f, 0.5f, 0.35f));
        if (auto* m = getBody(marsId)) { m->cloudCoverage = 0.06f; m->atmosphereDensity = 0.3f; }

        // Jupiter (Gas Giant with turbulent zonal bands and storm spots)
        addPlanet("Jupiter", Constants::JupiterMass, Constants::JupiterRadius, 5.204, glm::vec3(0.82f, 0.70f, 0.55f), 5, 606.0f, true, glm::vec3(0.8f, 0.7f, 0.6f));

        // Saturn with Rings (Gas Giant)
        BodyID saturnId = addPlanet("Saturn", 5.6834e26, 5.8232e7, 9.582, glm::vec3(0.90f, 0.82f, 0.62f), 5, 707.0f, true, glm::vec3(0.85f, 0.8f, 0.65f));
        auto* saturn = getBody(saturnId);
        saturn->hasRings = true;
        saturn->ringInnerRadius = 7.0e7;  // m
        saturn->ringOuterRadius = 1.4e8;  // m
        saturn->ringColor = glm::vec4(0.85f, 0.75f, 0.60f, 0.8f);

        // Uranus (Cyan Ice/Gas Giant)
        addPlanet("Uranus", 8.6810e25, 2.5362e7, 19.20, glm::vec3(0.55f, 0.80f, 0.85f), 5, 808.0f, true, glm::vec3(0.6f, 0.85f, 0.9f));

        // Neptune (Deep azure Gas Giant)
        addPlanet("Neptune", 1.0241e26, 2.4622e7, 30.05, glm::vec3(0.22f, 0.40f, 0.95f), 5, 909.0f, true, glm::vec3(0.3f, 0.5f, 1.0f));
    }

    void buildTwoBodyLab() {
        clock.fixedTimestep = 60.0;
        clock.timeScale = 1000.0;

        CelestialBody primary;
        primary.name = "Primary Star";
        primary.type = BodyType::Star;
        primary.mass = Constants::SolarMass;
        primary.radius = Constants::SolarRadius;
        primary.position = glm::dvec3(0.0);
        primary.velocity = glm::dvec3(0.0);
        primary.color = glm::vec3(1.0f, 0.85f, 0.4f);
        primary.emission = 1.0f;
        addBody(primary);

        CelestialBody planet;
        planet.name = "Elliptical Planet";
        planet.type = BodyType::Planet;
        planet.mass = Constants::EarthMass;
        planet.radius = Constants::EarthRadius * 2.0;
        double r = 1.0 * Constants::AU;
        planet.position = glm::dvec3(r, 0.0, 0.0);
        // Eccentric velocity (80% of circular velocity produces e ~ 0.36 ellipse)
        double vCirc = std::sqrt((Constants::G * primary.mass) / r);
        planet.velocity = glm::dvec3(0.0, 0.0, vCirc * 0.85);
        planet.color = glm::vec3(0.2f, 0.7f, 0.9f);
        addBody(planet);
    }

    void buildThreeBodyChaos() {
        clock.fixedTimestep = 300.0;
        clock.timeScale = 3600.0;

        double m = Constants::SolarMass * 0.8;
        double r = 1.2 * Constants::AU;
        double v = std::sqrt(Constants::G * m / (r * std::sqrt(3.0)));

        // Equilateral triangle configuration with tangential velocities
        for (int i = 0; i < 3; ++i) {
            double angle = i * (2.0 * std::numbers::pi / 3.0);
            CelestialBody star;
            star.name = "Star " + std::to_string(i + 1);
            star.type = BodyType::Star;
            star.mass = m;
            star.radius = Constants::SolarRadius * 0.9;
            star.position = glm::dvec3(r * std::cos(angle), 0.0, r * std::sin(angle));
            star.velocity = glm::dvec3(-v * std::sin(angle), 0.0, v * std::cos(angle));
            
            if (i == 0) star.color = glm::vec3(1.0f, 0.4f, 0.3f);
            else if (i == 1) star.color = glm::vec3(0.4f, 0.7f, 1.0f);
            else star.color = glm::vec3(1.0f, 0.9f, 0.4f);

            star.emission = 1.0f;
            addBody(star);
        }
    }

    void buildBinaryStar() {
        clock.fixedTimestep = 300.0;
        clock.timeScale = 3600.0;

        double sep = 0.5 * Constants::AU;
        double m1 = Constants::SolarMass;
        double m2 = Constants::SolarMass * 0.8;
        double r1 = sep * (m2 / (m1 + m2));
        double r2 = sep * (m1 / (m1 + m2));
        double v = std::sqrt(Constants::G * (m1 + m2) / sep);
        double v1 = v * (m2 / (m1 + m2));
        double v2 = v * (m1 / (m1 + m2));

        CelestialBody star1;
        star1.name = "Alpha Primary";
        star1.type = BodyType::Star;
        star1.mass = m1;
        star1.radius = Constants::SolarRadius;
        star1.position = glm::dvec3(-r1, 0.0, 0.0);
        star1.velocity = glm::dvec3(0.0, 0.0, -v1);
        star1.color = glm::vec3(1.0f, 0.8f, 0.4f);
        star1.emission = 1.0f;
        addBody(star1);

        CelestialBody star2;
        star2.name = "Beta Secondary";
        star2.type = BodyType::Star;
        star2.mass = m2;
        star2.radius = Constants::SolarRadius * 0.85;
        star2.position = glm::dvec3(r2, 0.0, 0.0);
        star2.velocity = glm::dvec3(0.0, 0.0, v2);
        star2.color = glm::vec3(0.5f, 0.7f, 1.0f);
        star2.emission = 1.0f;
        addBody(star2);

        // Circumbinary Planet orbiting the center of mass
        CelestialBody planet;
        planet.name = "Tatooine-I";
        planet.type = BodyType::Planet;
        planet.mass = Constants::EarthMass * 2.0;
        planet.radius = Constants::EarthRadius * 1.3;
        double rP = 2.5 * Constants::AU;
        planet.position = glm::dvec3(0.0, 0.0, rP);
        double vP = std::sqrt(Constants::G * (m1 + m2) / rP);
        planet.velocity = glm::dvec3(-vP, 0.0, 0.0);
        planet.color = glm::vec3(0.85f, 0.65f, 0.4f);
        planet.hasAtmosphere = true;
        addBody(planet);
    }

    void buildPlanetaryCollision() {
        clock.fixedTimestep = 1.0;
        clock.timeScale = 10.0;

        CelestialBody earth;
        earth.name = "Target Earth";
        earth.type = BodyType::Planet;
        earth.mass = Constants::EarthMass;
        earth.radius = Constants::EarthRadius;
        earth.position = glm::dvec3(0.0);
        earth.velocity = glm::dvec3(0.0);
        earth.color = glm::vec3(0.2f, 0.5f, 0.9f);
        addBody(earth);

        CelestialBody theia;
        theia.name = "Impactor Theia";
        theia.type = BodyType::Planet;
        theia.mass = Constants::EarthMass * 0.15; // Mars-sized impactor
        theia.radius = Constants::EarthRadius * 0.55;
        double impactDist = Constants::EarthRadius * 5.0;
        theia.position = glm::dvec3(impactDist, Constants::EarthRadius * 0.4, 0.0); // Slight glancing offset
        theia.velocity = glm::dvec3(-12000.0, 0.0, 0.0); // 12 km/s impact trajectory
        theia.color = glm::vec3(0.9f, 0.4f, 0.2f);
        addBody(theia);
    }

    void buildBlackHoleLab() {
        clock.fixedTimestep = 0.5;
        clock.timeScale = 100.0;

        // Intermediate mass Black Hole (1000 Solar Masses)
        CelestialBody bh;
        bh.name = "Sagittarius Mini";
        bh.type = BodyType::BlackHole;
        bh.mass = Constants::SolarMass * 1000.0;
        bh.radius = bh.getSchwarzschildRadius();
        bh.position = glm::dvec3(0.0);
        bh.velocity = glm::dvec3(0.0);
        bh.color = glm::vec3(0.02f, 0.02f, 0.02f);
        bh.hasRings = true; // Accretion disk
        bh.ringInnerRadius = bh.getISCORadius();
        bh.ringOuterRadius = bh.getSchwarzschildRadius() * 12.0;
        bh.ringColor = glm::vec4(1.0f, 0.7f, 0.2f, 0.9f);
        addBody(bh);

        // Orbiting test probes / stars
        double rs = bh.getSchwarzschildRadius();
        double r1 = rs * 8.0;
        double v1 = std::sqrt(Constants::G * bh.mass / r1);

        CelestialBody probe1;
        probe1.name = "Probe Alpha (Near ISCO)";
        probe1.type = BodyType::Spacecraft;
        probe1.mass = 1000.0; // 1 ton
        probe1.radius = 1e5;
        probe1.position = glm::dvec3(r1, 0.0, 0.0);
        probe1.velocity = glm::dvec3(0.0, 0.0, v1);
        probe1.color = glm::vec3(0.2f, 1.0f, 0.5f);
        addBody(probe1);

        double r2 = rs * 18.0;
        double v2 = std::sqrt(Constants::G * bh.mass / r2);
        CelestialBody star;
        star.name = "Infalling Star";
        star.type = BodyType::Star;
        star.mass = Constants::SolarMass * 0.5;
        star.radius = Constants::SolarRadius * 0.6;
        star.position = glm::dvec3(0.0, 0.0, r2);
        star.velocity = glm::dvec3(-v2 * 0.85, 0.0, 0.0);
        star.color = glm::vec3(0.4f, 0.8f, 1.0f);
        star.emission = 0.9f;
        addBody(star);
    }

    void buildMassVsGravityLab() {
        clock.fixedTimestep = 1.0;
        clock.timeScale = 1.0;

        CelestialBody planet;
        planet.name = "Laboratory Planet";
        planet.type = BodyType::Planet;
        planet.mass = Constants::EarthMass;
        planet.radius = Constants::EarthRadius;
        planet.position = glm::dvec3(0.0);
        planet.velocity = glm::dvec3(0.0);
        planet.isPinned = true;
        planet.color = glm::vec3(0.3f, 0.65f, 0.9f);
        addBody(planet);

        // Test probe suspended at 2 Earth radii
        CelestialBody probe;
        probe.name = "Gravimeter Probe";
        probe.type = BodyType::Spacecraft;
        probe.mass = 100.0; // 100 kg
        probe.radius = Constants::EarthRadius * 0.05;
        probe.position = glm::dvec3(Constants::EarthRadius * 2.5, 0.0, 0.0);
        probe.velocity = glm::dvec3(0.0);
        probe.isPinned = true; // Fixed in space to measure static field
        probe.color = glm::vec3(1.0f, 0.9f, 0.1f);
        addBody(probe);
    }

    void buildRelativisticPrecession() {
        clock.fixedTimestep = 100.0;
        clock.timeScale = 10000.0;
        enableRelativisticCorrections = true;

        CelestialBody central;
        central.name = "Dense Primary Core";
        central.type = BodyType::Star;
        central.mass = Constants::SolarMass * 5.0;
        central.radius = Constants::SolarRadius * 0.3;
        central.position = glm::dvec3(0.0);
        central.velocity = glm::dvec3(0.0);
        central.isPinned = true;
        central.color = glm::vec3(0.9f, 0.95f, 1.0f);
        central.emission = 1.0f;
        addBody(central);

        // High-eccentricity close orbit (strong relativistic perihelion advance)
        CelestialBody orbiter;
        orbiter.name = "Relativistic Orbiter (Mercury-type)";
        orbiter.type = BodyType::Planet;
        orbiter.mass = Constants::EarthMass;
        orbiter.radius = Constants::EarthRadius;
        double rp = central.radius * 6.0;
        orbiter.position = glm::dvec3(rp, 0.0, 0.0);
        double vCirc = std::sqrt(Constants::G * central.mass / rp);
        orbiter.velocity = glm::dvec3(0.0, 0.0, vCirc * 1.25); // Elliptical high-speed periapsis
        orbiter.color = glm::vec3(0.95f, 0.4f, 0.85f);
        orbiter.trailSampleInterval = 50.0; // Dense trail to visualize periapsis advance
        orbiter.maxTrailPoints = 800;
        addBody(orbiter);
    }
};

} // namespace Orbital
