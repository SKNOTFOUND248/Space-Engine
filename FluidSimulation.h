#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numbers>

// Particle structure containing physical properties
struct FluidParticle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 force{0.0f};
    float density{1000.0f};
    float pressure{0.0f};
    glm::vec3 color{0.2f, 0.6f, 0.95f}; // Dynamic water tint
};

// Preset scenarios
enum class FluidPreset {
    DAM_BREAK,
    CENTER_DROP,
    DUAL_SPLASH
};

class FluidSimulation {
public:
    // Simulation parameters (Smoothed Particle Hydrodynamics)
    float particleMass = 0.02f;
    float smoothingRadius = 0.16f;        // h
    float restDensity = 1000.0f;          // rho_0
    float gasStiffness = 350.0f;          // k in p = k*(rho - rho_0)
    float viscosity = 0.4f;               // mu
    float boundaryDamping = -0.5f;        // restitution against walls
    float particleRadius = 0.05f;

    // Environmental forces
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    int gravityMode = 0; // 0 = Down, 1 = Inverted, 2 = Zero-G, 3 = Lateral Swirl

    // Bounding Box container dimensions [min, max]
    glm::vec3 boxMin{-1.2f, 0.0f, -1.2f};
    glm::vec3 boxMax{ 1.2f, 2.2f,  1.2f};

    std::vector<FluidParticle> particles;

    // Spatial Grid for O(N) neighbor searching
    struct GridCell {
        std::vector<int> particleIndices;
    };
    std::vector<GridCell> grid;
    int gridDimX = 0, gridDimY = 0, gridDimZ = 0;
    float cellSize = 0.16f;

    FluidSimulation() {
        initGrid();
        resetToPreset(FluidPreset::DAM_BREAK);
    }

    void initGrid() {
        cellSize = smoothingRadius;
        gridDimX = static_cast<int>(std::ceil((boxMax.x - boxMin.x) / cellSize)) + 1;
        gridDimY = static_cast<int>(std::ceil((boxMax.y - boxMin.y) / cellSize)) + 1;
        gridDimZ = static_cast<int>(std::ceil((boxMax.z - boxMin.z) / cellSize)) + 1;
        grid.resize(gridDimX * gridDimY * gridDimZ);
    }

    void resetToPreset(FluidPreset preset) {
        particles.clear();
        float spacing = particleRadius * 1.55f;

        if (preset == FluidPreset::DAM_BREAK) {
            // Block of water on left side of container
            for (float y = boxMin.y + 0.1f; y <= boxMin.y + 1.8f; y += spacing) {
                for (float z = boxMin.z + 0.15f; z <= boxMax.z - 0.15f; z += spacing) {
                    for (float x = boxMin.x + 0.15f; x <= boxMin.x + 0.95f; x += spacing) {
                        FluidParticle p;
                        p.position = glm::vec3(x, y, z);
                        p.velocity = glm::vec3(0.0f);
                        particles.push_back(p);
                    }
                }
            }
        }
        else if (preset == FluidPreset::CENTER_DROP) {
            // Pool at bottom
            for (float y = boxMin.y + 0.08f; y <= boxMin.y + 0.35f; y += spacing) {
                for (float z = boxMin.z + 0.2f; z <= boxMax.z - 0.2f; z += spacing) {
                    for (float x = boxMin.x + 0.2f; x <= boxMax.x - 0.2f; x += spacing) {
                        FluidParticle p;
                        p.position = glm::vec3(x, y, z);
                        particles.push_back(p);
                    }
                }
            }
            // Suspended droplet block above center
            for (float y = boxMin.y + 1.1f; y <= boxMin.y + 1.9f; y += spacing) {
                for (float z = -0.4f; z <= 0.4f; z += spacing) {
                    for (float x = -0.4f; x <= 0.4f; x += spacing) {
                        FluidParticle p;
                        p.position = glm::vec3(x, y, z);
                        p.velocity = glm::vec3(0.0f, -1.5f, 0.0f);
                        particles.push_back(p);
                    }
                }
            }
        }
        else if (preset == FluidPreset::DUAL_SPLASH) {
            // Left column moving right
            for (float y = boxMin.y + 0.1f; y <= boxMin.y + 1.2f; y += spacing) {
                for (float z = -0.5f; z <= 0.5f; z += spacing) {
                    for (float x = boxMin.x + 0.15f; x <= boxMin.x + 0.65f; x += spacing) {
                        FluidParticle p;
                        p.position = glm::vec3(x, y, z);
                        p.velocity = glm::vec3(2.5f, 0.0f, 0.0f);
                        particles.push_back(p);
                    }
                }
            }
            // Right column moving left
            for (float y = boxMin.y + 0.1f; y <= boxMin.y + 1.2f; y += spacing) {
                for (float z = -0.5f; z <= 0.5f; z += spacing) {
                    for (float x = boxMax.x - 0.65f; x <= boxMax.x - 0.15f; x += spacing) {
                        FluidParticle p;
                        p.position = glm::vec3(x, y, z);
                        p.velocity = glm::vec3(-2.5f, 0.0f, 0.0f);
                        particles.push_back(p);
                    }
                }
            }
        }
    }

    void cycleGravity() {
        gravityMode = (gravityMode + 1) % 4;
        switch (gravityMode) {
            case 0: gravity = glm::vec3(0.0f, -9.81f, 0.0f); break; // Standard
            case 1: gravity = glm::vec3(0.0f,  6.0f,  0.0f); break; // Inverted / Anti-gravity
            case 2: gravity = glm::vec3(0.0f,  0.0f,  0.0f); break; // Zero-G
            case 3: gravity = glm::vec3(7.0f, -3.0f,  0.0f); break; // Lateral gravity
        }
    }

    void addSplashImpulse(float strength = 4.0f) {
        for (auto& p : particles) {
            float dist = glm::length(glm::vec2(p.position.x, p.position.z));
            if (dist < 0.6f && p.position.y < boxMin.y + 0.6f) {
                p.velocity.y += strength * (1.0f - dist / 0.6f);
                p.velocity.x += (p.position.x > 0.0f ? 1.0f : -1.0f) * 1.5f;
                p.velocity.z += (p.position.z > 0.0f ? 1.0f : -1.0f) * 1.5f;
            }
        }
    }

    void step(float dt) {
        // Clamp maximum timestep for numerical stability
        dt = std::min(dt, 0.016f);

        updateSpatialGrid();
        computeDensityAndPressure();
        computeForces();
        integrate(dt);
    }

private:
    [[nodiscard]] int getGridIndex(int x, int y, int z) const {
        return x + gridDimX * (y + gridDimY * z);
    }

    void updateSpatialGrid() {
        for (auto& cell : grid) {
            cell.particleIndices.clear();
        }

        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            const auto& pos = particles[i].position;
            int gx = std::clamp(static_cast<int>((pos.x - boxMin.x) / cellSize), 0, gridDimX - 1);
            int gy = std::clamp(static_cast<int>((pos.y - boxMin.y) / cellSize), 0, gridDimY - 1);
            int gz = std::clamp(static_cast<int>((pos.z - boxMin.z) / cellSize), 0, gridDimZ - 1);

            int idx = getGridIndex(gx, gy, gz);
            grid[idx].particleIndices.push_back(i);
        }
    }

    void computeDensityAndPressure() {
        const float h = smoothingRadius;
        const float h2 = h * h;
        // Poly6 Kernel constant: 315 / (64 * pi * h^9)
        const float poly6Const = static_cast<float>(315.0 / (64.0 * std::numbers::pi * std::pow(static_cast<double>(h), 9)));

        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            auto& pi = particles[i];
            float density = 0.0f;

            int gx = std::clamp(static_cast<int>((pi.position.x - boxMin.x) / cellSize), 0, gridDimX - 1);
            int gy = std::clamp(static_cast<int>((pi.position.y - boxMin.y) / cellSize), 0, gridDimY - 1);
            int gz = std::clamp(static_cast<int>((pi.position.z - boxMin.z) / cellSize), 0, gridDimZ - 1);

            for (int oz = std::max(0, gz - 1); oz <= std::min(gridDimZ - 1, gz + 1); ++oz) {
                for (int oy = std::max(0, gy - 1); oy <= std::min(gridDimY - 1, gy + 1); ++oy) {
                    for (int ox = std::max(0, gx - 1); ox <= std::min(gridDimX - 1, gx + 1); ++ox) {
                        const auto& cell = grid[getGridIndex(ox, oy, oz)];
                        for (int jIdx : cell.particleIndices) {
                            glm::vec3 rVec = pi.position - particles[jIdx].position;
                            float r2 = glm::dot(rVec, rVec);
                            if (r2 < h2) {
                                float term = h2 - r2;
                                density += particleMass * poly6Const * term * term * term;
                            }
                        }
                    }
                }
            }

            pi.density = std::max(density, restDensity);
            // Equation of State (Tait's formula / Ideal Gas)
            pi.pressure = gasStiffness * (pi.density - restDensity);
        }
    }

    void computeForces() {
        const float h = smoothingRadius;
        // Spiky gradient constant: -45 / (pi * h^6)
        const float spikyGradConst = static_cast<float>(-45.0 / (std::numbers::pi * std::pow(static_cast<double>(h), 6)));
        // Viscosity Laplacian constant: 45 / (pi * h^6)
        const float viscLapConst = static_cast<float>(45.0 / (std::numbers::pi * std::pow(static_cast<double>(h), 6)));

        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            auto& pi = particles[i];
            glm::vec3 fPressure{0.0f};
            glm::vec3 fViscosity{0.0f};

            int gx = std::clamp(static_cast<int>((pi.position.x - boxMin.x) / cellSize), 0, gridDimX - 1);
            int gy = std::clamp(static_cast<int>((pi.position.y - boxMin.y) / cellSize), 0, gridDimY - 1);
            int gz = std::clamp(static_cast<int>((pi.position.z - boxMin.z) / cellSize), 0, gridDimZ - 1);

            for (int oz = std::max(0, gz - 1); oz <= std::min(gridDimZ - 1, gz + 1); ++oz) {
                for (int oy = std::max(0, gy - 1); oy <= std::min(gridDimY - 1, gy + 1); ++oy) {
                    for (int ox = std::max(0, gx - 1); ox <= std::min(gridDimX - 1, gx + 1); ++ox) {
                        const auto& cell = grid[getGridIndex(ox, oy, oz)];
                        for (int jIdx : cell.particleIndices) {
                            if (i == jIdx) continue;

                            const auto& pj = particles[jIdx];
                            glm::vec3 rVec = pi.position - pj.position;
                            float r = glm::length(rVec);

                            if (r > 1e-5f && r < h) {
                                glm::vec3 rDir = rVec / r;
                                float h_r = h - r;

                                // Symmetric pressure force: -m * (pi + pj) / (2 * rho_j) * gradW
                                float pTerm = (pi.pressure + pj.pressure) / (2.0f * pj.density);
                                fPressure += -particleMass * pTerm * (spikyGradConst * h_r * h_r) * rDir;

                                // Viscosity force: mu * m * (vj - vi) / rho_j * laplacianW
                                fViscosity += viscosity * particleMass * (pj.velocity - pi.velocity) / pj.density * (viscLapConst * h_r);
                            }
                        }
                    }
                }
            }

            // Gravity force
            glm::vec3 fGravity = gravity * pi.density;

            pi.force = fPressure + fViscosity + fGravity;
        }
    }

    void integrate(float dt) {
        for (auto& p : particles) {
            // Leapfrog / Euler integration
            glm::vec3 acceleration = p.force / p.density;
            p.velocity += acceleration * dt;
            p.position += p.velocity * dt;

            // Boundary collision handling with padding
            float pad = particleRadius * 0.9f;

            // X-axis boundaries
            if (p.position.x < boxMin.x + pad) {
                p.position.x = boxMin.x + pad;
                p.velocity.x *= boundaryDamping;
            } else if (p.position.x > boxMax.x - pad) {
                p.position.x = boxMax.x - pad;
                p.velocity.x *= boundaryDamping;
            }

            // Y-axis boundaries (floor and ceiling)
            if (p.position.y < boxMin.y + pad) {
                p.position.y = boxMin.y + pad;
                p.velocity.y *= boundaryDamping;
            } else if (p.position.y > boxMax.y - pad) {
                p.position.y = boxMax.y - pad;
                p.velocity.y *= boundaryDamping;
            }

            // Z-axis boundaries
            if (p.position.z < boxMin.z + pad) {
                p.position.z = boxMin.z + pad;
                p.velocity.z *= boundaryDamping;
            } else if (p.position.z > boxMax.z - pad) {
                p.position.z = boxMax.z - pad;
                p.velocity.z *= boundaryDamping;
            }

            // Color coding based on velocity & pressure for stunning visuals
            float speed = glm::length(p.velocity);
            float speedFactor = std::clamp(speed / 4.0f, 0.0f, 1.0f);
            float pressureFactor = std::clamp(p.pressure / 800.0f, 0.0f, 1.0f);

            // Interpolate from deep ocean sapphire to glowing aquamarine to energetic foam white
            glm::vec3 deepBlue(0.08f, 0.35f, 0.85f);
            glm::vec3 brightCyan(0.15f, 0.85f, 0.95f);
            glm::vec3 splashFoam(0.9f, 0.98f, 1.0f);

            glm::vec3 baseTint = glm::mix(deepBlue, brightCyan, speedFactor);
            p.color = glm::mix(baseTint, splashFoam, pressureFactor * 0.7f);
        }
    }
};
