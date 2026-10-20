#pragma once

#include "../core/Types.h"
#include "../physics/CelestialBody.h"
#include "../physics/GravitySolver.h"
#include "../physics/Integrator.h"
#include "../physics/CollisionSystem.h"
#include "../physics/OrbitAnalyzer.h"
#include "../physics/EnergyDiagnostics.h"
#include "../relativity/RelativitySolver.h"
#include "../core/Json.h"
#include "../core/VFS.h"
#include "../catalog/DeepSpaceCatalog.h"
#include "../catalog/CatalogLoader.h"
#include "../scripting/ScenarioScript.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

namespace Orbital {

struct TestReport {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
};

class PhysicsTests {
public:
    static TestReport runAll() {
        TestReport report;
        std::cout << "\n======================================================\n";
        std::cout << "         ORBITAL SCIENTIFIC PHYSICS TEST SUITE        \n";
        std::cout << "======================================================\n";

        runTest(report, "1. Newtonian Gravitational Acceleration matches analytical formula", testGravitationalAcceleration);
        runTest(report, "2. Acceleration doubles when source mass doubles at fixed distance", testMassScaling);
        runTest(report, "3. Acceleration falls by a factor of 4 when distance doubles", testInverseSquareLaw);
        runTest(report, "4. Escape velocity matches analytical formula", testEscapeVelocity);
        runTest(report, "5. Schwarzschild radius scales linearly with mass", testSchwarzschildRadius);
        runTest(report, "6. Two-body Keplerian initial conditions produce bound orbit (e < 1.0)", testTwoBodyBoundOrbit);
        runTest(report, "7. Velocity Verlet integration conserves energy within tolerance", testVerletEnergyConservation);
        runTest(report, "8. Inelastic collision merger strictly conserves mass and linear momentum", testCollisionMergerConservation);
        runTest(report, "9. Center of mass calculation matches analytical result", testCenterOfMass);
        runTest(report, "10. Numerical non-finite / zero-distance handling produces no NaNs", testSingularityProtection);
        runTest(report, "11. Photon sphere (1.5 rs) and ISCO (3.0 rs) metric exactness", testBlackHoleMetrics);
        runTest(report, "12. Gravitational time dilation asymptotically approaches 0 at horizon", testTimeDilation);
        runTest(report, "13. Kepler's Third Law orbital period verification (T = 2*pi*sqrt(a^3/mu))", testKeplersThirdLaw);
        runTest(report, "14. JSON Parser & Serializer round-trip parsing of nested data", testJsonParserAndSerializer);
        runTest(report, "15. Virtual File System path traversal rejection & security checks", testVirtualFileSystemPathSecurity);
        runTest(report, "16. Scenario catalog serialization & deserialization round-trip", testScenarioSerializationRoundTrip);
        runTest(report, "17. Deep-space equatorial coordinate to Cartesian 3D projection", testDeepSpaceCoordinateTransform);
        runTest(report, "18. Scenario script command validation and step integrity", testScenarioScriptRunnerSequencing);

        std::cout << "------------------------------------------------------\n";
        std::cout << " Test Summary: " << report.passedTests << "/" << report.totalTests << " Passed ("
                  << (report.failedTests == 0 ? "ALL PASSED" : "FAILURES DETECTED") << ")\n";
        std::cout << "======================================================\n\n";

        return report;
    }

private:
    static void runTest(TestReport& report, const char* testName, bool (*testFunc)()) {
        report.totalTests++;
        bool ok = false;
        try {
            ok = testFunc();
        } catch (...) {
            ok = false;
        }

        if (ok) {
            report.passedTests++;
            std::cout << " [PASS] " << testName << "\n";
        } else {
            report.failedTests++;
            std::cerr << " [FAIL] " << testName << "\n";
        }
    }

    // Test 1: g = G * M / r^2
    static bool testGravitationalAcceleration() {
        double m = Constants::EarthMass;
        double r = Constants::EarthRadius;
        double gExpected = (Constants::G * m) / (r * r); // ~9.8196 m/s^2

        CelestialBody earth;
        earth.mass = m;
        earth.position = glm::dvec3(0.0);

        CelestialBody probe;
        probe.mass = 1.0;
        probe.position = glm::dvec3(r, 0.0, 0.0);

        std::vector<CelestialBody> bodies = { earth, probe };
        GravitySolver solver;
        solver.softening = 0.0;
        solver.computeAccelerations(bodies);

        double gCalculated = glm::length(bodies[1].acceleration);
        return std::abs(gCalculated - gExpected) / gExpected < 1e-6;
    }

    // Test 2: Doubling mass doubles acceleration
    static bool testMassScaling() {
        double r = 1e8;
        CelestialBody b1; b1.mass = 1e24; b1.position = glm::dvec3(0.0);
        CelestialBody b2; b2.mass = 2e24; b2.position = glm::dvec3(0.0);
        CelestialBody probe; probe.mass = 1.0; probe.position = glm::dvec3(r, 0.0, 0.0);

        GravitySolver solver;
        solver.softening = 0.0;

        std::vector<CelestialBody> sys1 = { b1, probe };
        solver.computeAccelerations(sys1);
        double a1 = glm::length(sys1[1].acceleration);

        std::vector<CelestialBody> sys2 = { b2, probe };
        solver.computeAccelerations(sys2);
        double a2 = glm::length(sys2[1].acceleration);

        return std::abs(a2 - 2.0 * a1) / a2 < 1e-9;
    }

    // Test 3: Doubling distance reduces acceleration by 4x
    static bool testInverseSquareLaw() {
        double r = 1e8;
        CelestialBody b; b.mass = 1e24; b.position = glm::dvec3(0.0);
        CelestialBody p1; p1.mass = 1.0; p1.position = glm::dvec3(r, 0.0, 0.0);
        CelestialBody p2; p2.mass = 1.0; p2.position = glm::dvec3(2.0 * r, 0.0, 0.0);

        GravitySolver solver;
        solver.softening = 0.0;

        std::vector<CelestialBody> sys1 = { b, p1 };
        solver.computeAccelerations(sys1);
        double a1 = glm::length(sys1[1].acceleration);

        std::vector<CelestialBody> sys2 = { b, p2 };
        solver.computeAccelerations(sys2);
        double a2 = glm::length(sys2[1].acceleration);

        return std::abs(a1 - 4.0 * a2) / a1 < 1e-9;
    }

    // Test 4: Escape velocity v_esc = sqrt(2 * G * M / R)
    static bool testEscapeVelocity() {
        CelestialBody earth;
        earth.mass = Constants::EarthMass;
        earth.radius = Constants::EarthRadius;

        double vEscExpected = std::sqrt((2.0 * Constants::G * earth.mass) / earth.radius); // ~11186 m/s
        double vEscCalc = earth.getEscapeVelocity();

        return std::abs(vEscCalc - vEscExpected) / vEscExpected < 1e-9;
    }

    // Test 5: Schwarzschild radius r_s = 2 * G * M / c^2
    static bool testSchwarzschildRadius() {
        double m1 = Constants::SolarMass;
        double m2 = 10.0 * Constants::SolarMass;

        auto r1 = RelativitySolver::computeMetrics(m1);
        auto r2 = RelativitySolver::computeMetrics(m2);

        // For 1 Solar Mass, rs ~ 2953.25 meters
        double rsExpected = (2.0 * Constants::G * m1) / Constants::c2;
        if (std::abs(r1.schwarzschildRadius - rsExpected) > 1e-3) return false;

        // Scaling linearity: r_s(10M) must be 10 * r_s(M)
        return std::abs(r2.schwarzschildRadius - 10.0 * r1.schwarzschildRadius) / r2.schwarzschildRadius < 1e-9;
    }

    // Test 6: Two-body Keplerian initial conditions produce bound orbit (e < 1.0)
    static bool testTwoBodyBoundOrbit() {
        CelestialBody sun;
        sun.mass = Constants::SolarMass;
        sun.position = glm::dvec3(0.0);
        sun.velocity = glm::dvec3(0.0);

        CelestialBody earth;
        earth.mass = Constants::EarthMass;
        double r = Constants::AU;
        earth.position = glm::dvec3(r, 0.0, 0.0);
        double vCirc = std::sqrt((Constants::G * sun.mass) / r);
        earth.velocity = glm::dvec3(0.0, 0.0, vCirc);

        auto elem = OrbitAnalyzer::calculate(earth, sun);
        return elem.isBound && elem.eccentricity < 1e-4; // Perfect circular orbit
    }

    // Test 7: Velocity Verlet energy conservation over orbits
    static bool testVerletEnergyConservation() {
        CelestialBody sun;
        sun.mass = Constants::SolarMass;
        sun.position = glm::dvec3(0.0);
        sun.velocity = glm::dvec3(0.0);

        CelestialBody planet;
        planet.mass = Constants::EarthMass;
        double r = Constants::AU;
        planet.position = glm::dvec3(r, 0.0, 0.0);
        double vCirc = std::sqrt((Constants::G * sun.mass) / r);
        planet.velocity = glm::dvec3(0.0, 0.0, vCirc * 0.9); // Elliptical

        std::vector<CelestialBody> bodies = { sun, planet };
        GravitySolver solver;
        solver.softening = 0.0;
        solver.computeAccelerations(bodies);

        EnergyDiagnostics diag;
        diag.update(bodies, 0.0);
        double e0 = diag.current.totalEnergy;

        // Step 1000 orbits with fixed timestep
        double dt = 3600.0; // 1 hour
        for (int i = 0; i < 2000; ++i) {
            Integrator::step(IntegratorType::VelocityVerlet, bodies, solver, dt);
        }

        diag.update(bodies, 2000.0 * dt);
        double eFinal = diag.current.totalEnergy;
        double drift = std::abs((eFinal - e0) / e0);

        return drift < 1e-4; // High symplectic energy conservation
    }

    // Test 8: Inelastic merger strictly conserves mass and linear momentum
    static bool testCollisionMergerConservation() {
        CelestialBody b1;
        b1.name = "Body1";
        b1.mass = 4.0e24;
        b1.radius = 5.0e6;
        b1.position = glm::dvec3(0.0);
        b1.velocity = glm::dvec3(1000.0, 0.0, 0.0);

        CelestialBody b2;
        b2.name = "Body2";
        b2.mass = 6.0e24;
        b2.radius = 6.0e6;
        b2.position = glm::dvec3(1.0e6, 0.0, 0.0); // overlapping
        b2.velocity = glm::dvec3(-500.0, 0.0, 0.0);

        double totalMassExpected = b1.mass + b2.mass;
        glm::dvec3 totalMomentumExpected = b1.velocity * b1.mass + b2.velocity * b2.mass;

        std::vector<CelestialBody> bodies = { b1, b2 };
        CollisionSystem collSystem;
        collSystem.policy = CollisionPolicy::MergeCoalesce;
        collSystem.detectAndResolve(bodies, 0.0);

        if (bodies.size() != 1) return false;

        double mergedMass = bodies[0].mass;
        glm::dvec3 mergedMomentum = bodies[0].velocity * bodies[0].mass;

        bool massConserved = std::abs(mergedMass - totalMassExpected) < 1e-6;
        bool momentumConserved = glm::length(mergedMomentum - totalMomentumExpected) / glm::length(totalMomentumExpected) < 1e-9;

        return massConserved && momentumConserved;
    }

    // Test 9: Center of Mass
    static bool testCenterOfMass() {
        CelestialBody b1; b1.mass = 2.0; b1.position = glm::dvec3(0.0, 0.0, 0.0);
        CelestialBody b2; b2.mass = 8.0; b2.position = glm::dvec3(10.0, 0.0, 0.0);

        std::vector<CelestialBody> bodies = { b1, b2 };
        EnergyDiagnostics diag;
        diag.update(bodies, 0.0);

        // Expected COM: (2*0 + 8*10) / 10 = 8.0
        return std::abs(diag.current.centerOfMass.x - 8.0) < 1e-9;
    }

    // Test 10: Singularity Protection produces finite numbers
    static bool testSingularityProtection() {
        CelestialBody b1; b1.mass = 1e24; b1.position = glm::dvec3(0.0);
        CelestialBody b2; b2.mass = 1e24; b2.position = glm::dvec3(0.0); // Exactly coinciding

        std::vector<CelestialBody> bodies = { b1, b2 };
        GravitySolver solver;
        solver.softening = 1e4; // Positive softening prevents 0 denominator
        solver.computeAccelerations(bodies);

        return std::isfinite(bodies[0].acceleration.x) && std::isfinite(bodies[1].acceleration.x);
    }

    // Test 11: Photon sphere (1.5 rs) and ISCO (3.0 rs)
    static bool testBlackHoleMetrics() {
        double m = 1e31;
        auto metrics = RelativitySolver::computeMetrics(m);

        bool phOk = std::abs(metrics.photonSphereRadius - 1.5 * metrics.schwarzschildRadius) < 1e-6;
        bool iscoOk = std::abs(metrics.iscoRadius - 3.0 * metrics.schwarzschildRadius) < 1e-6;
        return phOk && iscoOk;
    }

    // Test 12: Gravitational time dilation
    static bool testTimeDilation() {
        double m = Constants::SolarMass;
        double rs = (2.0 * Constants::G * m) / Constants::c2;

        double gammaAtHorizon = RelativitySolver::computeTimeDilation(m, rs);
        double gammaFarAway = RelativitySolver::computeTimeDilation(m, rs * 1000.0);

        return gammaAtHorizon == 0.0 && (gammaFarAway > 0.999 && gammaFarAway <= 1.0);
    }

    // Test 13: Kepler's Third Law (T^2 / a^3 = 4*pi^2 / (G*(M+m)))
    static bool testKeplersThirdLaw() {
        CelestialBody sun;
        sun.mass = Constants::SolarMass;
        sun.position = glm::dvec3(0.0);
        sun.velocity = glm::dvec3(0.0);

        CelestialBody earth;
        earth.mass = Constants::EarthMass;
        double a = Constants::AU;
        earth.position = glm::dvec3(a, 0.0, 0.0);
        double vCirc = std::sqrt((Constants::G * sun.mass) / a);
        earth.velocity = glm::dvec3(0.0, 0.0, vCirc);

        auto elem = OrbitAnalyzer::calculate(earth, sun);
        double expectedPeriod = Constants::YearInSeconds; // ~365.25 days = 31557600 s

        return std::abs(elem.period - expectedPeriod) / expectedPeriod < 0.005; // Matches within 0.5%
    }

    // Test 14: JSON Parser & Serializer round-trip
    static bool testJsonParserAndSerializer() {
        std::string jsonStr = R"({
            "schemaVersion": 1,
            "name": "Earth",
            "massKg": 5.972e24,
            "hasAtmosphere": true,
            "tags": ["terrestrial", "habitable"]
        })";
        std::string err;
        JsonValue parsed = JsonValue::parse(jsonStr, &err);
        if (parsed.isNull() || !err.empty()) return false;

        bool valOk = (parsed["schemaVersion"].asInt() == 1) &&
                     (parsed["name"].asString() == "Earth") &&
                     (std::abs(parsed["massKg"].asDouble() - 5.972e24) < 1e18) &&
                     (parsed["hasAtmosphere"].asBool() == true) &&
                     (parsed["tags"].asArray().size() == 2) &&
                     (parsed["tags"][0].asString() == "terrestrial");
        if (!valOk) return false;

        std::string serialized = parsed.serialize();
        JsonValue reParsed = JsonValue::parse(serialized, &err);
        return !reParsed.isNull() && reParsed["name"].asString() == "Earth";
    }

    // Test 15: VFS path traversal security
    static bool testVirtualFileSystemPathSecurity() {
        VFS& vfs = VFS::get();
        // Malicious path traversal must be rejected
        auto bad = vfs.resolvePath("catalogs/../../Windows/System32/calc.exe");
        if (bad.has_value()) return false; // Must return nullopt!

        return true;
    }

    // Test 16: Scenario serialization round-trip
    static bool testScenarioSerializationRoundTrip() {
        Universe src;
        src.loadScenario(ScenarioPreset::TwoBodyLaboratory);

        std::string json = CatalogLoader::serializeScenario(src, "test.twobody", "Two Body Test");
        if (json.empty()) return false;

        Universe dst;
        std::string err;
        bool loaded = CatalogLoader::loadScenarioFromJson(dst, json, &err);
        if (!loaded) return false;

        if (dst.bodies.size() != src.bodies.size()) return false;
        if (std::abs(dst.bodies[0].mass - src.bodies[0].mass) > 1e18) return false;
        if (std::abs(dst.bodies[1].mass - src.bodies[1].mass) > 1e18) return false;
        return true;
    }

    // Test 17: Deep-space coordinate transform
    static bool testDeepSpaceCoordinateTransform() {
        DeepSpaceObject m31;
        m31.distanceM = 2.4e22;
        m31.rightAscensionHours = 0.712;
        m31.declinationDegrees = 41.27;

        glm::dvec3 pos = m31.getCartesianPosition();
        double len = glm::length(pos);

        // Vector length must equal distance within floating-point tolerance
        return std::abs(len - m31.distanceM) / m31.distanceM < 1e-6;
    }

    // Test 18: Scenario script command validation
    static bool testScenarioScriptRunnerSequencing() {
        auto script = ScenarioScript::createMassLabExperiment();
        if (script.steps.empty()) return false;
        if (script.steps[0].type != ScriptCommandType::LoadScenario) return false;
        
        bool hasMassMod = false;
        for (const auto& step : script.steps) {
            if (step.type == ScriptCommandType::BodySetMass) {
                hasMassMod = true;
                break;
            }
        }
        return hasMassMod;
    }
};

} // namespace Orbital
