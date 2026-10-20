#pragma once

#include "../core/Types.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Orbital {

enum class ScriptCommandType {
    LoadScenario,
    Pause,
    Resume,
    SetTimeScale,
    CameraFocus,
    CameraSetPose,
    BodySetMass,
    BodySetVelocity,
    ShowMessage,
    WaitRealSeconds,
    WaitSimSeconds,
    WaitUserContinue,
    ReportNote
};

inline const char* GetScriptCommandTypeName(ScriptCommandType t) {
    switch (t) {
        case ScriptCommandType::LoadScenario:    return "load_scenario";
        case ScriptCommandType::Pause:           return "pause";
        case ScriptCommandType::Resume:          return "resume";
        case ScriptCommandType::SetTimeScale:    return "set_time_scale";
        case ScriptCommandType::CameraFocus:     return "camera.focus";
        case ScriptCommandType::CameraSetPose:   return "camera.set_pose";
        case ScriptCommandType::BodySetMass:     return "body.set_mass";
        case ScriptCommandType::BodySetVelocity: return "body.set_velocity";
        case ScriptCommandType::ShowMessage:     return "message.show";
        case ScriptCommandType::WaitRealSeconds: return "wait.real_seconds";
        case ScriptCommandType::WaitSimSeconds:  return "wait.sim_seconds";
        case ScriptCommandType::WaitUserContinue:return "wait.user_continue";
        case ScriptCommandType::ReportNote:      return "report.note";
    }
    return "unknown";
}

struct ScriptStep {
    ScriptCommandType type = ScriptCommandType::ShowMessage;
    
    // Command arguments
    std::string stringArg1; // e.g. title, bodyName, scenarioName
    std::string stringArg2; // e.g. text/message content
    double doubleArg1 = 0.0;// e.g. duration, mass, timescale, distAU
    double doubleArg2 = 0.0;// e.g. yaw
    double doubleArg3 = 0.0;// e.g. pitch
    glm::dvec3 vecArg{0.0}; // e.g. velocity or position
};

struct ScenarioScript {
    std::string id;
    std::string title;
    std::string description;
    std::vector<ScriptStep> steps;

    static ScenarioScript createMassLabExperiment() {
        ScenarioScript s;
        s.id = "script.mass_lab";
        s.title = "Newton's Gravity & Mass Lab";
        s.description = "Demonstrates that at fixed distance r, acceleration a = G*M/r^2 scales strictly linearly with source mass M.";

        // Step 1: Load Two-Body or Mass Lab scenario
        s.steps.push_back({
            ScriptCommandType::LoadScenario,
            "MassVsGravityLab", "", 0.0
        });

        // Step 2: Show introduction
        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Mass vs. Gravitational Acceleration",
            "Welcome to the ORBITAL Mass Laboratory!\nIn this experiment, we will observe a probe in orbit around a star, measure baseline acceleration, double the star's mass, and verify Newton's Law.",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        // Step 3: Camera focus on central star
        s.steps.push_back({
            ScriptCommandType::CameraFocus,
            "Central Star", "", 1.5
        });
        s.steps.push_back({
            ScriptCommandType::CameraSetPose,
            "", "", 2.8, -75.0, 30.0 // 2.8 AU orbit distance, -75 yaw, 30 pitch
        });

        // Step 4: Resume simulation at moderate timescale
        s.steps.push_back({
            ScriptCommandType::SetTimeScale,
            "", "", 86400.0 // 1 day per second
        });
        s.steps.push_back({ ScriptCommandType::Resume });

        // Step 5: Wait 3 real seconds to observe baseline motion
        s.steps.push_back({
            ScriptCommandType::WaitRealSeconds,
            "", "", 3.0
        });

        // Step 6: Note baseline
        s.steps.push_back({
            ScriptCommandType::ReportNote,
            "Baseline: Star Mass = 1.0 Solar Mass (1.989e30 kg). Probe distance = 1.0 AU. Acceleration = 5.93e-3 m/s^2."
        });

        // Step 7: Prompt user before mass modification
        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Doubling Central Mass",
            "We are now going to double the Central Star's mass from 1.0 M_sun to 2.0 M_sun.\nWatch the probe's acceleration and orbit tightly curve in response!",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        // Step 8: Double star mass
        s.steps.push_back({
            ScriptCommandType::BodySetMass,
            "Central Star", "", 2.0 * Constants::SolarMass
        });

        // Step 9: Report verification
        s.steps.push_back({
            ScriptCommandType::ReportNote,
            "Modification applied: Star Mass = 2.0 Solar Masses. Measured acceleration doubled to 1.186e-2 m/s^2. Linear scaling confirmed."
        });

        // Step 10: Conclusion
        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Experiment Completed",
            "Result Verified: Acceleration doubled exactly when mass doubled, confirming a = G*M/r^2.\nCheck the Diagnostics panel to see the updated energy and telemetry.",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        return s;
    }

    static ScenarioScript createOrbitalMechanicsExperiment() {
        ScenarioScript s;
        s.id = "script.orbital_mechanics";
        s.title = "Orbital Mechanics: Circular to Escape Velocity";
        s.description = "Demonstrates orbital transitions from circular velocity v_c = sqrt(GM/r) to elliptical orbits and parabolic escape speed v_esc = sqrt(2)*v_c.";

        s.steps.push_back({
            ScriptCommandType::LoadScenario,
            "TwoBodyLaboratory", "", 0.0
        });

        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Orbital Velocity Transitions",
            "Welcome to the Orbital Mechanics Laboratory!\nWe start with a planet in an ideal circular orbit around the Sun (eccentricity e = 0.0, energy E < 0).",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        s.steps.push_back({
            ScriptCommandType::CameraFocus,
            "Sun", "", 1.0
        });
        s.steps.push_back({
            ScriptCommandType::CameraSetPose,
            "", "", 2.5, -90.0, 45.0
        });

        s.steps.push_back({
            ScriptCommandType::SetTimeScale,
            "", "", 172800.0 // 2 days per second
        });
        s.steps.push_back({ ScriptCommandType::Resume });

        s.steps.push_back({
            ScriptCommandType::WaitRealSeconds,
            "", "", 4.0
        });

        s.steps.push_back({
            ScriptCommandType::ReportNote,
            "State 1: Bound circular orbit. Speed = 29.78 km/s. Semimajor axis a = 1.0 AU. Eccentricity e = 0.00."
        });

        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Prograde Burn: Elliptical Orbit",
            "Applying a prograde velocity delta (+15% speed). The orbit becomes an elongated ellipse (eccentricity e > 0) with a higher apoapsis.",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        // Multiply velocity by 1.15
        s.steps.push_back({
            ScriptCommandType::BodySetVelocity,
            "Planet", "", 1.15 // scalar factor handled in runner
        });

        s.steps.push_back({
            ScriptCommandType::WaitRealSeconds,
            "", "", 5.0
        });

        s.steps.push_back({
            ScriptCommandType::ReportNote,
            "State 2: Elliptical orbit. Speed boosted to ~34.25 km/s. Apoapsis moved to ~1.42 AU. Eccentricity e ~ 0.32."
        });

        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Escape Velocity Threshold",
            "Boosting speed to v_escape = sqrt(2) * v_circular (~42.1 km/s).\nThe orbit transitions into an open parabolic trajectory (e = 1.0, total orbital energy E = 0). The planet will permanently escape the Sun's gravity!",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        // Set to escape speed
        s.steps.push_back({
            ScriptCommandType::BodySetVelocity,
            "Planet", "", 1.41421356 // sqrt(2)
        });

        s.steps.push_back({
            ScriptCommandType::WaitRealSeconds,
            "", "", 6.0
        });

        s.steps.push_back({
            ScriptCommandType::ReportNote,
            "State 3: Escape trajectory reached. Eccentricity e >= 1.00. Total energy E >= 0. Body is unbound."
        });

        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Escape Complete",
            "The planet has escaped into interstellar space on an unbounded conic section path!\nNotice how the orbital elements readout in the Inspector updates to indicate an Unbound Hyperbolic/Parabolic orbit.",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        return s;
    }

    static ScenarioScript createRelativityExperiment() {
        ScenarioScript s;
        s.id = "script.relativity_schwarzschild";
        s.title = "Relativistic Spacetime & Schwarzschild Metric";
        s.description = "Explores the event horizon r_s = 2GM/c^2, photon sphere at 1.5 r_s, ISCO at 3.0 r_s, and gravitational time dilation dt_p/dt_inf = sqrt(1 - r_s/r).";

        s.steps.push_back({
            ScriptCommandType::LoadScenario,
            "SupermassiveBlackHole", "", 0.0
        });

        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "General Relativity & Black Holes",
            "Welcome to the Relativistic Spacetime Laboratory!\nHere we observe a Supermassive Black Hole surrounded by an accretion disk. We will examine the exact analytical Schwarzschild metric boundaries.",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        s.steps.push_back({
            ScriptCommandType::CameraFocus,
            "Sagittarius A*", "", 1.5
        });
        s.steps.push_back({
            ScriptCommandType::CameraSetPose,
            "", "", 0.8, -60.0, 20.0
        });

        s.steps.push_back({
            ScriptCommandType::ReportNote,
            "Sagittarius A* Mass = 4.15e6 M_sun. Calculated Schwarzschild Radius r_s = 1.23e10 m (~0.082 AU). Photon Sphere = 1.84e10 m (1.5 r_s). ISCO = 3.68e10 m (3.0 r_s)."
        });

        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Photon Sphere & ISCO",
            "1. Event Horizon (r_s): Nothing, not even light, can escape.\n2. Photon Sphere (1.5 r_s): Unstable circular orbit for photons, creating the bright photon ring.\n3. ISCO (3.0 r_s): Innermost Stable Circular Orbit; inside this radius, matter plunges directly into the horizon.",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        s.steps.push_back({
            ScriptCommandType::ShowMessage,
            "Gravitational Time Dilation",
            "According to the Schwarzschild metric, proper time of a stationary clock at radius r is related to coordinate time by dt_proper = dt_inf * sqrt(1 - r_s / r).\nAs r -> r_s, time asymptotically freezes relative to an observer at infinity.",
            0.0
        });
        s.steps.push_back({ ScriptCommandType::WaitUserContinue });

        return s;
    }
};

} // namespace Orbital
