#pragma once

#include "ScenarioScript.h"
#include "../simulation/Universe.h"
#include "../camera/CameraController.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>

namespace Orbital {

struct ActiveMessage {
    std::string title;
    std::string text;
    bool visible = false;
};

class ScriptRunner {
public:
    std::vector<ScenarioScript> availableScripts;
    int currentScriptIndex = -1;
    size_t currentStepIndex = 0;
    bool isRunning = false;

    // Wait states
    bool isWaitingForUser = false;
    double realWaitTimer = 0.0;
    double simWaitTarget = 0.0;
    bool isWaitingSimTime = false;

    // Active Dialogue Card
    ActiveMessage activeMessage;

    // Experiment Notes / Telemetry Log
    std::vector<std::string> experimentReport;

    // Camera animation state
    bool isCameraAnimating = false;
    double cameraAnimElapsed = 0.0;
    double cameraAnimDuration = 1.0;
    double camStartDist = 1.0;
    double camTargetDist = 1.0;
    double camStartYaw = 0.0;
    double camTargetYaw = 0.0;
    double camStartPitch = 0.0;
    double camTargetPitch = 0.0;

    ScriptRunner() {
        availableScripts.push_back(ScenarioScript::createMassLabExperiment());
        availableScripts.push_back(ScenarioScript::createOrbitalMechanicsExperiment());
        availableScripts.push_back(ScenarioScript::createRelativityExperiment());
    }

    void startScript(int index, Universe& universe, CameraController& camera) {
        if (index < 0 || index >= static_cast<int>(availableScripts.size())) return;
        currentScriptIndex = index;
        currentStepIndex = 0;
        isRunning = true;
        isWaitingForUser = false;
        realWaitTimer = 0.0;
        isWaitingSimTime = false;
        activeMessage.visible = false;
        isCameraAnimating = false;
        experimentReport.clear();

        experimentReport.push_back("=== EXPERIMENT REPORT: " + availableScripts[index].title + " ===");
        experimentReport.push_back("Description: " + availableScripts[index].description);

        executeNextStep(universe, camera);
    }

    void stop() {
        isRunning = false;
        isWaitingForUser = false;
        activeMessage.visible = false;
        isCameraAnimating = false;
    }

    void userContinue(Universe& universe, CameraController& camera) {
        if (isWaitingForUser) {
            isWaitingForUser = false;
            activeMessage.visible = false;
            currentStepIndex++;
            executeNextStep(universe, camera);
        }
    }

    void update(Universe& universe, CameraController& camera, double realDeltaTime) {
        if (!isRunning) return;

        // 1. Update camera animation
        if (isCameraAnimating) {
            cameraAnimElapsed += realDeltaTime;
            float t = static_cast<float>(std::clamp(cameraAnimElapsed / cameraAnimDuration, 0.0, 1.0));
            // Smoothstep easing: 3*t^2 - 2*t^3
            float ease = t * t * (3.0f - 2.0f * t);

            camera.orbitDistance = camStartDist + (camTargetDist - camStartDist) * ease;
            camera.orbitYaw = camStartYaw + (camTargetYaw - camStartYaw) * ease;
            camera.orbitPitch = camStartPitch + (camTargetPitch - camStartPitch) * ease;

            if (t >= 1.0f) {
                isCameraAnimating = false;
            }
        }

        // 2. Real-time wait condition
        if (realWaitTimer > 0.0) {
            realWaitTimer -= realDeltaTime;
            if (realWaitTimer <= 0.0) {
                realWaitTimer = 0.0;
                currentStepIndex++;
                executeNextStep(universe, camera);
            }
            return;
        }

        // 3. Sim-time wait condition
        if (isWaitingSimTime) {
            if (universe.clock.simulatedTime >= simWaitTarget) {
                isWaitingSimTime = false;
                currentStepIndex++;
                executeNextStep(universe, camera);
            }
            return;
        }
    }

    const ScenarioScript* getActiveScript() const {
        if (currentScriptIndex >= 0 && currentScriptIndex < static_cast<int>(availableScripts.size())) {
            return &availableScripts[currentScriptIndex];
        }
        return nullptr;
    }

private:
    void executeNextStep(Universe& universe, CameraController& camera) {
        const auto* script = getActiveScript();
        if (!script) {
            stop();
            return;
        }

        while (currentStepIndex < script->steps.size()) {
            const auto& step = script->steps[currentStepIndex];

            switch (step.type) {
                case ScriptCommandType::LoadScenario: {
                    if (step.stringArg1 == "TwoBodyLaboratory") universe.loadScenario(ScenarioPreset::TwoBodyLaboratory);
                    else if (step.stringArg1 == "MassVsGravityLab") universe.loadScenario(ScenarioPreset::MassVsGravityLab);
                    else if (step.stringArg1 == "SupermassiveBlackHole") universe.loadScenario(ScenarioPreset::SupermassiveBlackHole);
                    else if (step.stringArg1 == "PlanetaryCollision") universe.loadScenario(ScenarioPreset::PlanetaryCollision);
                    else if (step.stringArg1 == "SolarSystem") universe.loadScenario(ScenarioPreset::SolarSystem);
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::Pause: {
                    universe.clock.isPaused = true;
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::Resume: {
                    universe.clock.isPaused = false;
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::SetTimeScale: {
                    universe.clock.setTimeScale(step.doubleArg1);
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::CameraFocus: {
                    for (const auto& b : universe.bodies) {
                        if (!b.isDestroyed && b.name.find(step.stringArg1) != std::string::npos) {
                            camera.focusOnBody(b);
                            universe.selectedBodyId = b.id;
                            break;
                        }
                    }
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::CameraSetPose: {
                    isCameraAnimating = true;
                    cameraAnimElapsed = 0.0;
                    cameraAnimDuration = 1.5;
                    camStartDist = camera.orbitDistance;
                    camTargetDist = step.doubleArg1 * Constants::AU;
                    camStartYaw = camera.orbitYaw;
                    camTargetYaw = step.doubleArg2;
                    camStartPitch = camera.orbitPitch;
                    camTargetPitch = step.doubleArg3;
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::BodySetMass: {
                    for (auto& b : universe.bodies) {
                        if (!b.isDestroyed && b.name.find(step.stringArg1) != std::string::npos) {
                            b.mass = step.doubleArg1;
                            std::ostringstream ss;
                            ss << "[SCRIPT] Modified mass of '" << b.name << "' to " << b.mass << " kg";
                            std::cout << ss.str() << "\n";
                            break;
                        }
                    }
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::BodySetVelocity: {
                    for (auto& b : universe.bodies) {
                        if (!b.isDestroyed && b.name.find(step.stringArg1) != std::string::npos) {
                            // If doubleArg1 > 0, scale velocity by factor
                            if (step.doubleArg1 > 0.0) {
                                b.velocity *= step.doubleArg1;
                            } else {
                                b.velocity = step.vecArg;
                            }
                            break;
                        }
                    }
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::ShowMessage: {
                    activeMessage.title = step.stringArg1;
                    activeMessage.text = step.stringArg2;
                    activeMessage.visible = true;
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::ReportNote: {
                    experimentReport.push_back("- " + step.stringArg1);
                    currentStepIndex++;
                    break;
                }
                case ScriptCommandType::WaitRealSeconds: {
                    realWaitTimer = step.doubleArg1;
                    return; // Yield until timer expires
                }
                case ScriptCommandType::WaitSimSeconds: {
                    simWaitTarget = universe.clock.simulatedTime + step.doubleArg1;
                    isWaitingSimTime = true;
                    return; // Yield until simulation reaches target
                }
                case ScriptCommandType::WaitUserContinue: {
                    isWaitingForUser = true;
                    return; // Yield until user clicks Continue
                }
            }
        }

        // Script finished
        experimentReport.push_back("=== EXPERIMENT RUN COMPLETE ===");
        isRunning = false;
    }
};

} // namespace Orbital
