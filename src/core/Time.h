#pragma once

#include <cstdint>
#include <algorithm>

namespace Orbital {

class SimulationClock {
public:
    double timeScale = 1.0;          // Time acceleration multiplier (1x, 60x, 3600x, 86400x)
    double fixedTimestep = 60.0;     // Internal simulation timestep in seconds
    double simulatedTime = 0.0;      // Total elapsed simulated time in seconds
    double realTimeElapsed = 0.0;    // Total elapsed real-world time in seconds
    
    bool isPaused = false;
    bool stepOnce = false;

    int maxSubstepsPerFrame = 64;    // Protection against simulation stalls / spiral of death
    int lastExecutedSubsteps = 0;
    double accumulator = 0.0;

    SimulationClock() = default;

    void reset() {
        simulatedTime = 0.0;
        realTimeElapsed = 0.0;
        accumulator = 0.0;
        stepOnce = false;
    }

    void togglePause() {
        isPaused = !isPaused;
    }

    void setTimeScale(double scale) {
        timeScale = std::max(scale, 0.0);
    }

    void requestStep() {
        stepOnce = true;
    }

    // Returns the number of fixed substeps to execute this frame
    int update(double frameDeltaTime) {
        realTimeElapsed += frameDeltaTime;

        if (stepOnce) {
            stepOnce = false;
            lastExecutedSubsteps = 1;
            simulatedTime += fixedTimestep;
            return 1;
        }

        if (isPaused) {
            lastExecutedSubsteps = 0;
            return 0;
        }

        // Clamp frame delta time to prevent spiral of death when window is dragged
        double dtClamped = std::min(frameDeltaTime, 0.1);
        accumulator += dtClamped * timeScale;

        int substeps = 0;
        while (accumulator >= fixedTimestep && substeps < maxSubstepsPerFrame) {
            accumulator -= fixedTimestep;
            simulatedTime += fixedTimestep;
            substeps++;
        }

        // If workload is exceeding maxSubsteps, drop excess accumulated time to preserve responsiveness
        if (accumulator > fixedTimestep * maxSubstepsPerFrame) {
            accumulator = 0.0;
        }

        lastExecutedSubsteps = substeps;
        return substeps;
    }
};

} // namespace Orbital
