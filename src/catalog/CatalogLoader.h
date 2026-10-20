#pragma once

#include "../core/Types.h"
#include "../core/Json.h"
#include "../core/VFS.h"
#include "../physics/CelestialBody.h"
#include "../simulation/Universe.h"
#include <string>
#include <vector>
#include <iostream>

namespace Orbital {

class CatalogLoader {
public:
    // Serializes the current Universe state to a valid Blueprint Section 36 scenario JSON string
    static std::string serializeScenario(const Universe& universe, const std::string& scenarioId, const std::string& scenarioName) {
        JsonValue::Object root;
        root["schemaVersion"] = 1;
        root["id"] = scenarioId;
        root["name"] = scenarioName;
        root["initialPause"] = universe.clock.isPaused;
        root["timeStepSeconds"] = universe.clock.fixedTimestep;
        root["timeScale"] = universe.clock.timeScale;

        std::string integName = "velocity-verlet";
        if (universe.integratorType == IntegratorType::SemiImplicitEuler) integName = "semi-implicit-euler";
        else if (universe.integratorType == IntegratorType::RungeKutta4) integName = "runge-kutta-4";
        root["integrator"] = integName;

        JsonValue::Array bodiesArr;
        for (const auto& b : universe.bodies) {
            if (b.isDestroyed) continue;

            JsonValue::Object bObj;
            bObj["id"] = "body." + std::to_string(b.id);
            bObj["name"] = b.name;
            bObj["type"] = std::string(GetBodyTypeName(b.type));
            
            // Physics
            JsonValue::Object phys;
            phys["massKg"] = b.mass;
            phys["meanRadiusM"] = b.radius;
            phys["rotationPeriodSeconds"] = (b.rotationSpeed > 0.0) ? ((2.0 * std::numbers::pi) / b.rotationSpeed) : 86400.0;
            phys["axialTiltDegrees"] = b.axialTilt;
            phys["mode"] = b.isPinned ? "pinned-source" : "dynamic";
            bObj["physics"] = phys;

            // Initial State
            JsonValue::Object st;
            JsonValue::Array pos;
            pos.push_back(b.position.x);
            pos.push_back(b.position.y);
            pos.push_back(b.position.z);
            st["positionM"] = pos;

            JsonValue::Array vel;
            vel.push_back(b.velocity.x);
            vel.push_back(b.velocity.y);
            vel.push_back(b.velocity.z);
            st["velocityMS"] = vel;
            bObj["initialState"] = st;

            // Appearance
            JsonValue::Object app;
            JsonValue::Array col;
            col.push_back(static_cast<double>(b.color.r));
            col.push_back(static_cast<double>(b.color.g));
            col.push_back(static_cast<double>(b.color.b));
            app["colorRGB"] = col;
            app["roughness"] = static_cast<double>(b.roughness);
            app["metallic"] = static_cast<double>(b.metallic);
            app["emission"] = static_cast<double>(b.emission);
            app["atmosphereEnabled"] = b.hasAtmosphere;
            app["atmosphereDensity"] = static_cast<double>(b.atmosphereDensity);
            app["ringsEnabled"] = b.hasRings;
            app["ringInnerRadiusM"] = b.ringInnerRadius;
            app["ringOuterRadiusM"] = b.ringOuterRadius;
            bObj["appearance"] = app;

            bodiesArr.push_back(bObj);
        }
        root["bodies"] = bodiesArr;

        return JsonValue(root).serialize();
    }

    // Loads a scenario from a JSON string into Universe
    static bool loadScenarioFromJson(Universe& universe, const std::string& jsonStr, std::string* errOut = nullptr) {
        JsonValue root = JsonValue::parse(jsonStr, errOut);
        if (!root.isObject()) {
            if (errOut && errOut->empty()) *errOut = "Root element is not a JSON object";
            return false;
        }

        universe.bodies.clear();
        universe.nextBodyId = 1;
        universe.selectedBodyId = INVALID_BODY_ID;
        universe.primaryBodyId = INVALID_BODY_ID;
        universe.clock.reset();
        universe.energyDiagnostics.reset();

        universe.clock.isPaused = root["initialPause"].asBool(false);
        universe.clock.fixedTimestep = root["timeStepSeconds"].asDouble(10.0);
        if (root.contains("timeScale")) {
            universe.clock.setTimeScale(root["timeScale"].asDouble(1.0));
        }

        std::string integ = root["integrator"].asString("velocity-verlet");
        if (integ == "semi-implicit-euler") universe.integratorType = IntegratorType::SemiImplicitEuler;
        else if (integ == "runge-kutta-4") universe.integratorType = IntegratorType::RungeKutta4;
        else universe.integratorType = IntegratorType::VelocityVerlet;

        const auto& bodiesArr = root["bodies"].asArray();
        for (const auto& bVal : bodiesArr) {
            CelestialBody b;
            b.name = bVal["name"].asString("Imported Body");

            std::string t = bVal["type"].asString("Planet");
            if (t == "Star") b.type = BodyType::Star;
            else if (t == "Moon") b.type = BodyType::Moon;
            else if (t == "Asteroid") b.type = BodyType::Asteroid;
            else if (t == "Comet") b.type = BodyType::Comet;
            else if (t == "Black Hole") b.type = BodyType::BlackHole;
            else if (t == "Spacecraft / Probe") b.type = BodyType::Spacecraft;
            else b.type = BodyType::Planet;

            // Physics block
            if (bVal.contains("physics")) {
                const auto& phys = bVal["physics"];
                b.mass = phys["massKg"].asDouble(Constants::EarthMass);
                b.radius = phys["meanRadiusM"].asDouble(Constants::EarthRadius);
                double rotPeriod = phys["rotationPeriodSeconds"].asDouble(Constants::DayInSeconds);
                b.rotationSpeed = (rotPeriod > 0.0) ? ((2.0 * std::numbers::pi) / rotPeriod) : 0.0;
                b.axialTilt = phys["axialTiltDegrees"].asDouble(0.0);
                std::string mode = phys["mode"].asString("dynamic");
                b.isPinned = (mode == "pinned-source");
            } else {
                b.mass = bVal["massKg"].asDouble(Constants::EarthMass);
                b.radius = bVal["radiusM"].asDouble(Constants::EarthRadius);
                std::string mode = bVal["mode"].asString("dynamic");
                b.isPinned = (mode == "pinned-source");
            }

            // Initial State block
            if (bVal.contains("initialState")) {
                const auto& st = bVal["initialState"];
                if (st.contains("positionM") && st["positionM"].isArray() && st["positionM"].size() >= 3) {
                    b.position.x = st["positionM"][0].asDouble(0.0);
                    b.position.y = st["positionM"][1].asDouble(0.0);
                    b.position.z = st["positionM"][2].asDouble(0.0);
                }
                if (st.contains("velocityMS") && st["velocityMS"].isArray() && st["velocityMS"].size() >= 3) {
                    b.velocity.x = st["velocityMS"][0].asDouble(0.0);
                    b.velocity.y = st["velocityMS"][1].asDouble(0.0);
                    b.velocity.z = st["velocityMS"][2].asDouble(0.0);
                }
            } else {
                if (bVal.contains("positionM") && bVal["positionM"].isArray() && bVal["positionM"].size() >= 3) {
                    b.position.x = bVal["positionM"][0].asDouble(0.0);
                    b.position.y = bVal["positionM"][1].asDouble(0.0);
                    b.position.z = bVal["positionM"][2].asDouble(0.0);
                }
                if (bVal.contains("velocityMS") && bVal["velocityMS"].isArray() && bVal["velocityMS"].size() >= 3) {
                    b.velocity.x = bVal["velocityMS"][0].asDouble(0.0);
                    b.velocity.y = bVal["velocityMS"][1].asDouble(0.0);
                    b.velocity.z = bVal["velocityMS"][2].asDouble(0.0);
                }
            }

            // Appearance block
            if (bVal.contains("appearance")) {
                const auto& app = bVal["appearance"];
                if (app.contains("colorRGB") && app["colorRGB"].isArray() && app["colorRGB"].size() >= 3) {
                    b.color.r = static_cast<float>(app["colorRGB"][0].asDouble(0.5));
                    b.color.g = static_cast<float>(app["colorRGB"][1].asDouble(0.5));
                    b.color.b = static_cast<float>(app["colorRGB"][2].asDouble(0.5));
                }
                b.roughness = static_cast<float>(app["roughness"].asDouble(0.6));
                b.metallic = static_cast<float>(app["metallic"].asDouble(0.0));
                b.emission = static_cast<float>(app["emission"].asDouble(0.0));
                b.hasAtmosphere = app["atmosphereEnabled"].asBool(false);
                b.atmosphereDensity = static_cast<float>(app["atmosphereDensity"].asDouble(1.0));
                b.hasRings = app["ringsEnabled"].asBool(false);
                b.ringInnerRadius = app["ringInnerRadiusM"].asDouble(0.0);
                b.ringOuterRadius = app["ringOuterRadiusM"].asDouble(0.0);
            }

            universe.addBody(b);
        }

        universe.updatePrimaryBody();
        if (!universe.bodies.empty()) {
            universe.selectedBodyId = universe.bodies[0].id;
        }

        return true;
    }

    // Loads a scenario file via VFS
    static bool loadScenarioFromVFS(Universe& universe, const std::string& virtualUri, std::string* errOut = nullptr) {
        auto content = VFS::get().readTextFile(virtualUri);
        if (!content) {
            if (errOut) *errOut = "Failed to resolve or read VFS path: " + virtualUri;
            return false;
        }
        return loadScenarioFromJson(universe, *content, errOut);
    }

    // Saves a scenario file via VFS
    static bool saveScenarioToVFS(const Universe& universe, const std::string& virtualUri, const std::string& id, const std::string& name) {
        std::string jsonStr = serializeScenario(universe, id, name);
        return VFS::get().writeTextFile(virtualUri, jsonStr);
    }
};

} // namespace Orbital
