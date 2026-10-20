#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "../simulation/Universe.h"
#include "../camera/CameraController.h"
#include "../catalog/DeepSpaceCatalog.h"
#include "../scripting/ScriptRunner.h"
#include "../core/VFS.h"
#include "../graphics/ProceduralPlanet.h"

#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>

namespace Orbital {

class UIManager {
public:
    bool showObjectExplorer = true;
    bool showInspector = true;
    bool showDiagnostics = true;
    bool showCreateBodyModal = false;
    bool showScriptRunner = false;
    bool showAddonsModal = false;
    bool showDeepSpaceExplorer = false;

    // Creation dialog state
    char newBodyName[64] = "New Planet";
    int newBodyType = 1; // Planet
    double newBodyMass = Constants::EarthMass;
    double newBodyRadius = Constants::EarthRadius;
    float newBodyColor[3] = {0.2f, 0.7f, 0.9f};
    double newBodyDistAU = 1.2;
    bool newBodyCircularOrbit = true;

    // Script Runner reference
    ScriptRunner* scriptRunner = nullptr;
    DeepSpaceCatalog* deepSpaceCatalog = nullptr;

    void init(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Dark modern scientific laboratory theme
        setScientificDarkTheme();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 430 core");
    }

    void shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void beginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void render(Universe& universe, CameraController& camera, double fps, double physicsTimeMs) {
        renderTopToolbar(universe, camera);
        if (showObjectExplorer) renderObjectExplorer(universe, camera);
        if (showInspector) renderInspector(universe, camera);
        if (showDiagnostics) renderDiagnostics(universe, fps, physicsTimeMs);
        if (showCreateBodyModal) renderCreateBodyModal(universe);
        if (showScriptRunner && scriptRunner) renderScriptRunnerPanel(universe, camera);
        if (showAddonsModal) renderAddonsModal();
        if (showDeepSpaceExplorer && deepSpaceCatalog) renderDeepSpaceExplorer(camera);

        // Always render floating dialogue banner if script has active prompt
        if (scriptRunner && scriptRunner->isRunning && scriptRunner->activeMessage.visible) {
            renderFloatingScriptDialog(universe, camera);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

private:
    void setScientificDarkTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.94f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.55f, 0.62f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.06f, 0.08f, 0.12f, 0.88f); // Sleek translucent dark
        colors[ImGuiCol_ChildBg]               = ImVec4(0.08f, 0.10f, 0.15f, 0.70f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.10f, 0.15f, 0.95f);
        colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.28f, 0.40f, 0.60f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.12f, 0.16f, 0.24f, 0.70f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.18f, 0.25f, 0.38f, 0.80f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.22f, 0.32f, 0.48f, 0.90f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.11f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.12f, 0.18f, 0.28f, 1.00f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.07f, 0.09f, 0.14f, 1.00f);
        colors[ImGuiCol_Button]                = ImVec4(0.16f, 0.24f, 0.38f, 0.80f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.24f, 0.36f, 0.55f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.28f, 0.42f, 0.65f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.28f, 0.44f, 0.75f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.36f, 0.56f, 0.90f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.30f, 0.45f, 0.70f, 1.00f);
        colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.16f, 0.24f, 0.80f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.24f, 0.36f, 0.56f, 1.00f);
        colors[ImGuiCol_TabActive]             = ImVec4(0.18f, 0.28f, 0.44f, 1.00f);
        colors[ImGuiCol_PlotLines]             = ImVec4(0.35f, 0.75f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.45f, 0.35f, 1.00f);
    }

    void renderTopToolbar(Universe& universe, CameraController& camera) {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y));
        ImGui::SetNextWindowSize(ImVec2(vp->Size.x, 48.0f));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                                 ImGuiWindowFlags_NoMove | 
                                 ImGuiWindowFlags_NoScrollWithMouse | 
                                 ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("TopToolbar", nullptr, flags)) {
            // Play / Pause
            if (ImGui::Button(universe.clock.isPaused ? " Play " : " Pause ")) {
                universe.clock.togglePause();
            }
            ImGui::SameLine();

            // Single Step
            if (ImGui::Button(" Step (1 dt) ")) {
                universe.clock.requestStep();
            }
            ImGui::SameLine();

            // Reset
            if (ImGui::Button(" Reset ")) {
                universe.loadScenario(ScenarioPreset::SolarSystem);
            }
            ImGui::SameLine();

            // Time warp selection
            ImGui::SetNextItemWidth(140.0f);
            const char* timeScales[] = { "1x Realtime", "60x (1 min/s)", "3600x (1 hr/s)", "86400x (1 day/s)", "604800x (1 wk/s)", "2592000x (1 mo/s)" };
            static int currentTimeScaleIdx = 3; // 86400x
            if (ImGui::Combo("##TimeWarp", &currentTimeScaleIdx, timeScales, IM_ARRAYSIZE(timeScales))) {
                double scales[] = { 1.0, 60.0, 3600.0, 86400.0, 604800.0, 2592000.0 };
                universe.clock.timeScale = scales[currentTimeScaleIdx];
            }
            ImGui::SameLine();

            // Scenario Preset selector
            ImGui::SetNextItemWidth(190.0f);
            const char* presets[] = {
                "Solar System",
                "Two-Body Kepler Lab",
                "Three-Body Chaos",
                "Binary Star System",
                "Planetary Collision",
                "Black Hole Lab",
                "Mass vs Gravity Lab",
                "Relativistic Precession",
                "Empty Universe"
            };
            static int currentPresetIdx = 0;
            if (ImGui::Combo("##Preset", &currentPresetIdx, presets, IM_ARRAYSIZE(presets))) {
                universe.loadScenario(static_cast<ScenarioPreset>(currentPresetIdx));
            }
            ImGui::SameLine();

            // Scale Mode Toggle
            if (ImGui::Button(camera.scaleMode == ScaleMode::LaboratoryScale ? " Lab Scale " : " 1:1 Physical ")) {
                camera.scaleMode = (camera.scaleMode == ScaleMode::LaboratoryScale) ? ScaleMode::TruePhysical : ScaleMode::LaboratoryScale;
            }
            ImGui::SameLine();

            // Create New Body Button
            if (ImGui::Button(" + Add Body ")) {
                showCreateBodyModal = true;
            }
            ImGui::SameLine();

            // Guided Experiments / Script Runner
            if (ImGui::Button(showScriptRunner ? " [X] Experiments " : " Guided Experiments ")) {
                showScriptRunner = !showScriptRunner;
            }
            ImGui::SameLine();

            // Deep-Space Catalog Button
            if (ImGui::Button(showDeepSpaceExplorer ? " [X] DSO Catalog " : " DSO Catalog ")) {
                showDeepSpaceExplorer = !showDeepSpaceExplorer;
            }
            ImGui::SameLine();

            // Add-ons Manager Button
            if (ImGui::Button(" Add-ons ")) {
                showAddonsModal = true;
            }
        }
        ImGui::End();
    }

    void renderObjectExplorer(Universe& universe, CameraController& camera) {
        ImGui::SetNextWindowPos(ImVec2(10.0f, 55.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280.0f, 380.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Object Explorer", &showObjectExplorer)) {
            ImGui::Text("Active Celestial Bodies: %zu", universe.bodies.size());
            ImGui::Separator();

            for (const auto& b : universe.bodies) {
                if (b.isDestroyed) continue;

                bool isSelected = (universe.selectedBodyId == b.id);
                std::string label = b.name + " (" + GetBodyTypeName(b.type) + ")";

                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    universe.selectedBodyId = b.id;
                    camera.focusOnBody(b);
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Mass: %.2e kg | Radius: %.2e m\nBiome: %s", 
                                      b.mass, b.radius, GetBiomeTypeName(static_cast<PlanetBiomeType>(b.biomeType)));
                }
            }

            ImGui::Separator();
            if (universe.selectedBodyId != INVALID_BODY_ID) {
                if (ImGui::Button("Focus Camera")) {
                    if (const auto* b = universe.getBody(universe.selectedBodyId)) {
                        camera.focusOnBody(*b);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete Selected")) {
                    universe.removeBody(universe.selectedBodyId);
                }
            }
        }
        ImGui::End();
    }

    void renderInspector(Universe& universe, CameraController& camera) {
        ImGui::SetNextWindowPos(ImVec2(920.0f, 55.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(370.0f, 520.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Scientific Inspector & Mass Lab", &showInspector)) {
            CelestialBody* body = universe.getBody(universe.selectedBodyId);
            if (!body) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a celestial body to inspect.");
                ImGui::End();
                return;
            }

            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s", body->name.c_str());
            ImGui::TextDisabled("Classification: %s", GetBodyTypeName(body->type));
            ImGui::Separator();

            if (ImGui::BeginTabBar("InspectorTabs")) {
                if (ImGui::BeginTabItem("Physics")) {
                    ImGui::Text("Mass: %.3e kg", body->mass);
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%.2f M_Earth)", body->mass / Constants::EarthMass);

                    ImGui::Text("Radius: %.3e m", body->radius);
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%.2f R_Earth)", body->radius / Constants::EarthRadius);

                    ImGui::Text("Mean Density: %.1f kg/m^3", body->getMeanDensity());
                    ImGui::Text("Surface Gravity: %.2f m/s^2 (%.2f g)", body->getSurfaceGravity(), body->getSurfaceGravity() / 9.80665);
                    ImGui::Text("Escape Velocity: %.2f km/s", body->getEscapeVelocity() / 1000.0);
                    ImGui::Text("Gravitational Parameter (GM): %.3e m^3/s^2", body->getGravitationalParameter());

                    ImGui::Separator();
                    ImGui::TextWrapped("Direct Mass Experimentation:");
                    static float massMultiplier = 1.0f;
                    if (ImGui::SliderFloat("Mass Multiplier", &massMultiplier, 0.1f, 10.0f, "%.2fx")) {
                        body->mass = Constants::EarthMass * static_cast<double>(massMultiplier);
                    }
                    ImGui::Checkbox("Pin Body (Fixed Anchor)", &body->isPinned);

                    if (body->type == BodyType::BlackHole) {
                        ImGui::Separator();
                        double rs = body->getSchwarzschildRadius();
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "General Relativity Analysis");
                        ImGui::Text("Schwarzschild Radius (r_s): %.2f km", rs / 1000.0);
                        ImGui::Text("Photon Sphere (1.5 r_s): %.2f km", body->getPhotonSphereRadius() / 1000.0);
                        ImGui::Text("ISCO Radius (3.0 r_s): %.2f km", body->getISCORadius() / 1000.0);
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Surface & Biomes")) {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.95f, 1.0f), "Procedural Surface Generation");
                    
                    const char* biomes[] = {
                        "Terrestrial (Earth-like)",
                        "Desert (Mars-like)",
                        "Ocean World",
                        "Ice World (Europa-like)",
                        "Volcanic (Io-like)",
                        "Gas Giant (Jupiter-like)"
                    };
                    ImGui::Combo("Biome Type", &body->biomeType, biomes, IM_ARRAYSIZE(biomes));

                    ImGui::SliderFloat("Procedural Seed", &body->seed, 1.0f, 1000.0f, "%.0f");
                    ImGui::SliderFloat("Sea Level", &body->seaLevel, 0.0f, 1.0f, "%.2f");
                    ImGui::SliderFloat("Cloud Coverage", &body->cloudCoverage, 0.0f, 1.0f, "%.2f");
                    
                    ImGui::Checkbox("Has Atmosphere", &body->hasAtmosphere);
                    if (body->hasAtmosphere) {
                        ImGui::ColorEdit3("Atmosphere Color", (float*)&body->atmosphereColor);
                        ImGui::SliderFloat("Atmosphere Density", &body->atmosphereDensity, 0.1f, 3.0f, "%.2f");
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Orbit")) {
                    const CelestialBody* primary = universe.getPrimaryBody();
                    if (primary && primary->id != body->id) {
                        auto elem = OrbitAnalyzer::calculate(*body, *primary);
                        ImGui::Text("Primary Body: %s", primary->name.c_str());
                        ImGui::Text("Orbit Type: %s", elem.isBound ? (elem.eccentricity < 0.01 ? "Circular" : "Elliptical") : "Hyperbolic / Unbound");
                        ImGui::Text("Semi-Major Axis (a): %.3e m (%.3f AU)", elem.semiMajorAxis, elem.semiMajorAxis / Constants::AU);
                        ImGui::Text("Eccentricity (e): %.4f", elem.eccentricity);
                        ImGui::Text("Inclination (i): %.2f deg", elem.inclination);
                        ImGui::Text("Periapsis: %.3e m", elem.periapsis);
                        ImGui::Text("Apoapsis: %.3e m", elem.apoapsis);
                        if (elem.isBound) {
                            ImGui::Text("Period (T): %.2f days (%.2f yrs)", elem.period / Constants::DayInSeconds, elem.period / Constants::YearInSeconds);
                        }
                        ImGui::Text("Orbital Speed: %.2f km/s", elem.currentSpeed / 1000.0);
                    } else {
                        ImGui::Text("Central reference body (No parent orbit).");
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void renderDiagnostics(Universe& universe, double fps, double physicsTimeMs) {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(10.0f, vp->Size.y - 200.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500.0f, 190.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Scientific Diagnostics & Telemetry", &showDiagnostics)) {
            ImGui::Text("Telemetry: FPS: %.0f | Physics: %.2f ms | Bodies: %zu", fps, physicsTimeMs, universe.bodies.size());
            ImGui::Text("Simulated Time: %.2f days (%.2f yrs)", 
                        universe.clock.simulatedTime / Constants::DayInSeconds, 
                        universe.clock.simulatedTime / Constants::YearInSeconds);
            ImGui::Separator();

            const auto& diag = universe.energyDiagnostics.current;
            ImGui::Text("Kinetic Energy (K):  %.3e J", diag.kineticEnergy);
            ImGui::Text("Potential Energy (U): %.3e J", diag.potentialEnergy);
            ImGui::Text("Total Energy (E):     %.3e J", diag.totalEnergy);
            ImGui::TextColored(std::abs(diag.energyDrift) < 1e-4 ? ImVec4(0.3f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                               "Energy Drift (dE/E0): %.4e (%.4f %%)", diag.energyDrift, diag.energyDrift * 100.0);

            if (!universe.energyDiagnostics.driftHistory.empty()) {
                std::vector<float> data(universe.energyDiagnostics.driftHistory.begin(), universe.energyDiagnostics.driftHistory.end());
                ImGui::PlotLines("Drift (%)", data.data(), static_cast<int>(data.size()), 0, nullptr, -0.05f, 0.05f, ImVec2(0, 36));
            }

            ImGui::Separator();
            if (ImGui::Button("Export Telemetry to CSV")) {
                exportTelemetryCsv(universe);
            }
        }
        ImGui::End();
    }

    void renderScriptRunnerPanel(Universe& universe, CameraController& camera) {
        ImGui::SetNextWindowPos(ImVec2(300.0f, 55.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(420.0f, 400.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Guided Tours & Scientific Experiments", &showScriptRunner)) {
            if (!scriptRunner) {
                ImGui::Text("Script runner subsystem not initialized.");
                ImGui::End();
                return;
            }

            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Automated Interactive Experiments");
            ImGui::TextDisabled("Select an experiment to begin guided scientific verification:");
            ImGui::Separator();

            for (size_t i = 0; i < scriptRunner->availableScripts.size(); ++i) {
                const auto& s = scriptRunner->availableScripts[i];
                bool isThisRunning = (scriptRunner->isRunning && scriptRunner->currentScriptIndex == static_cast<int>(i));

                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Button(isThisRunning ? " Running... " : " Start ")) {
                    scriptRunner->startScript(static_cast<int>(i), universe, camera);
                }
                ImGui::SameLine();
                ImGui::Text("%s", s.title.c_str());
                ImGui::TextDisabled("%s", s.description.c_str());
                ImGui::PopID();
                ImGui::Separator();
            }

            if (scriptRunner->isRunning) {
                const auto* active = scriptRunner->getActiveScript();
                if (active) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "Active: %s", active->title.c_str());
                    ImGui::Text("Step %zu of %zu", scriptRunner->currentStepIndex + 1, active->steps.size());
                    
                    if (scriptRunner->isWaitingForUser) {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Status: Waiting for user confirmation");
                        if (ImGui::Button(" Continue / Next Step ")) {
                            scriptRunner->userContinue(universe, camera);
                        }
                    } else if (scriptRunner->realWaitTimer > 0.0) {
                        ImGui::Text("Observing trajectory... (%.1f s)", scriptRunner->realWaitTimer);
                    }

                    if (ImGui::Button("Abort Experiment")) {
                        scriptRunner->stop();
                    }
                }
            }

            if (!scriptRunner->experimentReport.empty()) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Experiment Observation Notes:");
                for (const auto& note : scriptRunner->experimentReport) {
                    ImGui::TextWrapped("%s", note.c_str());
                }
            }
        }
        ImGui::End();
    }

    void renderFloatingScriptDialog(Universe& universe, CameraController& camera) {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        float width = 560.0f;
        float height = 150.0f;
        ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + (vp->Size.x - width) * 0.5f, vp->Pos.y + vp->Size.y - height - 30.0f));
        ImGui::SetNextWindowSize(ImVec2(width, height));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("FloatingScriptDialog", nullptr, flags)) {
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "%s", scriptRunner->activeMessage.title.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("%s", scriptRunner->activeMessage.text.c_str());

            if (scriptRunner->isWaitingForUser) {
                ImGui::Spacing();
                if (ImGui::Button(" Continue / Next Step ", ImVec2(180, 28))) {
                    scriptRunner->userContinue(universe, camera);
                }
            }
        }
        ImGui::End();
    }

    void renderDeepSpaceExplorer(CameraController& camera) {
        ImGui::SetNextWindowPos(ImVec2(300.0f, 120.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(380.0f, 360.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Deep-Space Catalog (DSO)", &showDeepSpaceExplorer)) {
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Curated Deep-Space Objects");
            ImGui::TextDisabled("Extragalactic structures, nebulae, and stellar nurseries:");
            ImGui::Separator();

            for (const auto& dso : deepSpaceCatalog->objects) {
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.6f, 1.0f), "%s", dso.name.c_str());
                ImGui::Text("Type: %s | Morphology: %s", GetDSOTypeName(dso.type), dso.morphology.c_str());
                
                double distLY = dso.distanceM / 9.4607e15;
                if (distLY > 1.0e5) {
                    ImGui::Text("Distance: %.2f Mly (Mega light-years)", distLY / 1.0e6);
                } else {
                    ImGui::Text("Distance: %.0f ly", distLY);
                }
                ImGui::Text("Apparent Mag: %.2f | RA: %.2fh | Dec: %.1f deg", 
                            dso.apparentMagnitude, dso.rightAscensionHours, dso.declinationDegrees);
                ImGui::TextDisabled("Source: %s", dso.provenance.c_str());
                
                if (ImGui::Button(("Look Towards##" + dso.id).c_str())) {
                    glm::dvec3 pos = dso.getCartesianPosition();
                    glm::dvec3 dir = glm::normalize(pos);
                    camera.orbitYaw = glm::degrees(std::atan2(dir.z, dir.x));
                    camera.orbitPitch = glm::degrees(std::asin(dir.y));
                }
                ImGui::Separator();
            }
        }
        ImGui::End();
    }

    void renderAddonsModal() {
        ImGui::SetNextWindowPos(ImVec2(350.0f, 150.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(480.0f, 320.0f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Add-on Packages & Virtual File System", &showAddonsModal)) {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Discovered Add-on Packages");
            ImGui::TextDisabled("Package manifests adhering to Section 25 & 36:");
            ImGui::Separator();

            const auto& addons = VFS::get().getAddons();
            if (addons.empty()) {
                ImGui::Text("No external add-ons discovered in 'addons/' directory.");
                ImGui::TextDisabled("Place add-on directories with 'manifest.json' inside ./addons/ to extend catalogs, scenarios, and textures.");
            } else {
                for (const auto& a : addons) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "%s", a.displayName.c_str());
                    ImGui::Text("ID: %s (v%s) | License: %s", a.id.c_str(), a.version.c_str(), a.license.c_str());
                    ImGui::TextDisabled("Path: %s", a.addonDirectory.string().c_str());
                    ImGui::Separator();
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("Close")) {
                showAddonsModal = false;
            }
        }
        ImGui::End();
    }

    void exportTelemetryCsv(const Universe& universe) {
        std::ofstream csv("orbital_telemetry.csv", std::ios::out | std::ios::trunc);
        if (!csv.is_open()) return;

        csv << "TimeSeconds,TotalEnergyJoules,KineticEnergyJoules,PotentialEnergyJoules,EnergyDrift\n";
        const auto& diag = universe.energyDiagnostics.current;
        csv << universe.clock.simulatedTime << ","
            << diag.totalEnergy << ","
            << diag.kineticEnergy << ","
            << diag.potentialEnergy << ","
            << diag.energyDrift << "\n";
        
        csv.close();
        std::cout << "[TELEMETRY] Successfully exported simulation state to orbital_telemetry.csv\n";
    }

    void renderCreateBodyModal(Universe& universe) {
        ImGui::OpenPopup("Add Celestial Body");
        if (ImGui::BeginPopupModal("Add Celestial Body", &showCreateBodyModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Name", newBodyName, IM_ARRAYSIZE(newBodyName));

            const char* typeItems[] = { "Star", "Planet", "Moon", "Asteroid", "Comet", "Black Hole", "Spacecraft" };
            ImGui::Combo("Body Type", &newBodyType, typeItems, IM_ARRAYSIZE(typeItems));

            ImGui::InputDouble("Mass (kg)", &newBodyMass, 1e22, 1e24, "%.2e");
            ImGui::InputDouble("Radius (m)", &newBodyRadius, 1e5, 1e6, "%.2e");
            ImGui::InputDouble("Distance from Sun (AU)", &newBodyDistAU, 0.1, 1.0, "%.3f");
            ImGui::ColorEdit3("Albedo Color", newBodyColor);
            ImGui::Checkbox("Initialize in Circular Keplerian Orbit", &newBodyCircularOrbit);

            ImGui::Separator();
            if (ImGui::Button("Spawn Body", ImVec2(120, 0))) {
                CelestialBody b;
                b.name = newBodyName;
                b.type = static_cast<BodyType>(newBodyType);
                b.mass = newBodyMass;
                b.radius = newBodyRadius;
                b.color = glm::vec3(newBodyColor[0], newBodyColor[1], newBodyColor[2]);

                double r = newBodyDistAU * Constants::AU;
                b.position = glm::dvec3(r, 0.0, 0.0);

                const auto* primary = universe.getPrimaryBody();
                if (newBodyCircularOrbit && primary) {
                    double v = std::sqrt((Constants::G * primary->mass) / r);
                    b.velocity = glm::dvec3(0.0, 0.0, v);
                }

                universe.addBody(b);
                showCreateBodyModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                showCreateBodyModal = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
};

} // namespace Orbital
