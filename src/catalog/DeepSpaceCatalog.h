#pragma once

#include "../core/Types.h"
#include "../core/Json.h"
#include <string>
#include <vector>
#include <cmath>
#include <numbers>
#include <glm/glm.hpp>

namespace Orbital {

enum class DSOType {
    Galaxy,
    Nebula,
    StarCluster,
    Quasar
};

inline const char* GetDSOTypeName(DSOType type) {
    switch (type) {
        case DSOType::Galaxy:      return "Galaxy";
        case DSOType::Nebula:      return "Nebula";
        case DSOType::StarCluster: return "Star Cluster";
        case DSOType::Quasar:      return "Quasar";
    }
    return "Unknown";
}

struct DeepSpaceObject {
    std::string id;
    std::string name;
    DSOType type = DSOType::Galaxy;
    std::string morphology = "Spiral";
    
    // Distance and Dimensions in SI Units (Meters)
    double distanceM = 2.4e22; // ~2.5 million light years
    double diameterM = 9.5e20; // ~100,000 light years
    
    // Equatorial Coordinates (J2000 epoch)
    double rightAscensionHours = 0.0; // 0 to 24 hrs
    double declinationDegrees = 0.0;  // -90 to +90 deg
    
    float apparentMagnitude = 3.44f;
    glm::vec3 baseColor{0.9f, 0.85f, 0.75f};
    
    // Visual parameters
    float coreRadiusFraction = 0.15f;
    float spiralArmTightness = 0.25f;
    int spiralArmCount = 2;
    float tiltAngleDegrees = 45.0f;
    
    std::string provenance = "SIMBAD Astronomical Database / NASA";

    // Converts RA, Dec, and Distance into Cartesian 3D Coordinates (X, Y, Z in meters)
    [[nodiscard]] glm::dvec3 getCartesianPosition() const {
        double raRad = (rightAscensionHours / 24.0) * 2.0 * std::numbers::pi;
        double decRad = (declinationDegrees / 180.0) * std::numbers::pi;

        double cosDec = std::cos(decRad);
        double x = distanceM * cosDec * std::cos(raRad);
        double y = distanceM * std::sin(decRad);
        double z = distanceM * cosDec * std::sin(raRad);

        return glm::dvec3(x, y, z);
    }

    [[nodiscard]] JsonValue toJson() const {
        JsonValue::Object obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["type"] = std::string(GetDSOTypeName(type));
        obj["morphology"] = morphology;
        obj["distanceM"] = distanceM;
        obj["diameterM"] = diameterM;
        obj["rightAscensionHours"] = rightAscensionHours;
        obj["declinationDegrees"] = declinationDegrees;
        obj["apparentMagnitude"] = static_cast<double>(apparentMagnitude);
        
        JsonValue::Array col;
        col.push_back(static_cast<double>(baseColor.r));
        col.push_back(static_cast<double>(baseColor.g));
        col.push_back(static_cast<double>(baseColor.b));
        obj["baseColor"] = col;
        
        obj["coreRadiusFraction"] = static_cast<double>(coreRadiusFraction);
        obj["spiralArmTightness"] = static_cast<double>(spiralArmTightness);
        obj["spiralArmCount"] = spiralArmCount;
        obj["tiltAngleDegrees"] = static_cast<double>(tiltAngleDegrees);
        obj["provenance"] = provenance;
        return JsonValue(obj);
    }

    static DeepSpaceObject fromJson(const JsonValue& j) {
        DeepSpaceObject dso;
        dso.id = j["id"].asString();
        dso.name = j["name"].asString(dso.id);
        std::string t = j["type"].asString("Galaxy");
        if (t == "Nebula") dso.type = DSOType::Nebula;
        else if (t == "Star Cluster") dso.type = DSOType::StarCluster;
        else if (t == "Quasar") dso.type = DSOType::Quasar;
        else dso.type = DSOType::Galaxy;

        dso.morphology = j["morphology"].asString("Spiral");
        dso.distanceM = j["distanceM"].asDouble(2.4e22);
        dso.diameterM = j["diameterM"].asDouble(9.5e20);
        dso.rightAscensionHours = j["rightAscensionHours"].asDouble(0.0);
        dso.declinationDegrees = j["declinationDegrees"].asDouble(0.0);
        dso.apparentMagnitude = static_cast<float>(j["apparentMagnitude"].asDouble(4.0));

        if (j.contains("baseColor") && j["baseColor"].isArray() && j["baseColor"].size() >= 3) {
            dso.baseColor.r = static_cast<float>(j["baseColor"][0].asDouble(1.0));
            dso.baseColor.g = static_cast<float>(j["baseColor"][1].asDouble(1.0));
            dso.baseColor.b = static_cast<float>(j["baseColor"][2].asDouble(1.0));
        }

        dso.coreRadiusFraction = static_cast<float>(j["coreRadiusFraction"].asDouble(0.15));
        dso.spiralArmTightness = static_cast<float>(j["spiralArmTightness"].asDouble(0.25));
        dso.spiralArmCount = j["spiralArmCount"].asInt(2);
        dso.tiltAngleDegrees = static_cast<float>(j["tiltAngleDegrees"].asDouble(45.0));
        dso.provenance = j["provenance"].asString("Curated Astronomical Catalog");
        return dso;
    }
};

class DeepSpaceCatalog {
public:
    std::vector<DeepSpaceObject> objects;

    DeepSpaceCatalog() {
        populateDefaultCatalog();
    }

    void populateDefaultCatalog() {
        objects.clear();

        // 1 Light Year = 9.4607e15 meters
        constexpr double LY = 9.4607e15;

        // 1. Andromeda Galaxy (M31) - Nearest major spiral galaxy
        {
            DeepSpaceObject m31;
            m31.id = "dso.galaxy.m31";
            m31.name = "Andromeda Galaxy (M31 / NGC 224)";
            m31.type = DSOType::Galaxy;
            m31.morphology = "SA(s)b Barred Spiral";
            m31.distanceM = 2.537e6 * LY; // 2.537 million ly
            m31.diameterM = 220000.0 * LY;
            m31.rightAscensionHours = 0.712; // 00h 42m 44s
            m31.declinationDegrees = 41.27;   // +41 16'
            m31.apparentMagnitude = 3.44f;
            m31.baseColor = glm::vec3(0.95f, 0.88f, 0.75f);
            m31.spiralArmCount = 2;
            m31.spiralArmTightness = 0.22f;
            m31.tiltAngleDegrees = 77.0f; // Edge-on slanted
            m31.provenance = "NASA/IPAC Extragalactic Database (NED)";
            objects.push_back(m31);
        }

        // 2. Triangulum Galaxy (M33) - 3rd largest in Local Group
        {
            DeepSpaceObject m33;
            m33.id = "dso.galaxy.m33";
            m33.name = "Triangulum Galaxy (M33 / NGC 598)";
            m33.type = DSOType::Galaxy;
            m33.morphology = "SA(s)cd Flocculent Spiral";
            m33.distanceM = 2.73e6 * LY;
            m33.diameterM = 60000.0 * LY;
            m33.rightAscensionHours = 1.564; // 01h 33m 50s
            m33.declinationDegrees = 30.66;
            m33.apparentMagnitude = 5.72f;
            m33.baseColor = glm::vec3(0.75f, 0.85f, 0.98f); // Blue-tinted starburst
            m33.spiralArmCount = 3;
            m33.spiralArmTightness = 0.35f;
            m33.tiltAngleDegrees = 54.0f;
            m33.provenance = "NASA/ESA Hubble Heritage";
            objects.push_back(m33);
        }

        // 3. Orion Nebula (M42) - Active stellar nursery
        {
            DeepSpaceObject m42;
            m42.id = "dso.nebula.m42";
            m42.name = "Great Orion Nebula (M42 / NGC 1976)";
            m42.type = DSOType::Nebula;
            m42.morphology = "Diffuse Emission & Reflection H-II";
            m42.distanceM = 1344.0 * LY;
            m42.diameterM = 24.0 * LY;
            m42.rightAscensionHours = 5.59; // 05h 35m 17s
            m42.declinationDegrees = -5.39;
            m42.apparentMagnitude = 4.0f;
            m42.baseColor = glm::vec3(0.95f, 0.35f, 0.45f); // Glowing hydrogen-alpha pink/red
            m42.coreRadiusFraction = 0.25f;
            m42.provenance = "ESA/Hubble & ESO VLT";
            objects.push_back(m42);
        }

        // 4. Pleiades Star Cluster (M45) - The Seven Sisters
        {
            DeepSpaceObject m45;
            m45.id = "dso.cluster.m45";
            m45.name = "Pleiades Star Cluster (M45)";
            m45.type = DSOType::StarCluster;
            m45.morphology = "Open Star Cluster with Reflection Nebula";
            m45.distanceM = 444.0 * LY;
            m45.diameterM = 17.5 * LY;
            m45.rightAscensionHours = 3.79; // 03h 47m 24s
            m45.declinationDegrees = 24.11;
            m45.apparentMagnitude = 1.6f;
            m45.baseColor = glm::vec3(0.65f, 0.80f, 1.0f); // Brilliant hot B-type blue stars
            m45.provenance = "Hipparcos Astrometry & Gaia DR3";
            objects.push_back(m45);
        }

        // 5. Crab Nebula (M1) - Historic Supernova Remnant of 1054 CE
        {
            DeepSpaceObject m1;
            m1.id = "dso.nebula.m1";
            m1.name = "Crab Nebula (M1 / NGC 1952)";
            m1.type = DSOType::Nebula;
            m1.morphology = "Supernova Remnant (Pulsar Wind Nebula)";
            m1.distanceM = 6500.0 * LY;
            m1.diameterM = 11.0 * LY;
            m1.rightAscensionHours = 5.575; // 05h 34m 31s
            m1.declinationDegrees = 22.01;
            m1.apparentMagnitude = 8.4f;
            m1.baseColor = glm::vec3(0.40f, 0.85f, 0.90f); // Synchrotron teal filament glow
            m1.provenance = "Chandra X-Ray Observatory & Hubble";
            objects.push_back(m1);
        }
    }

    const DeepSpaceObject* findById(const std::string& id) const {
        for (const auto& obj : objects) {
            if (obj.id == id) return &obj;
        }
        return nullptr;
    }

    std::string serializeJson() const {
        JsonValue::Array arr;
        for (const auto& obj : objects) {
            arr.push_back(obj.toJson());
        }
        JsonValue root(arr);
        return root.serialize();
    }

    bool loadFromJsonString(const std::string& jsonStr, std::string* errOut = nullptr) {
        JsonValue root = JsonValue::parse(jsonStr, errOut);
        if (!root.isArray()) {
            if (errOut) *errOut = "Expected JSON array of deep-space objects";
            return false;
        }

        objects.clear();
        for (size_t i = 0; i < root.size(); ++i) {
            objects.push_back(DeepSpaceObject::fromJson(root[i]));
        }
        return true;
    }
};

} // namespace Orbital
