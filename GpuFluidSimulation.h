#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Shader.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <numbers>

// Aligned 48-byte particle structure matching std430 layout on GPU
struct alignas(16) GPUParticle {
    glm::vec4 position_density{0.0f, 0.0f, 0.0f, 1000.0f}; // xyz: pos, w: density
    glm::vec4 velocity_pressure{0.0f, 0.0f, 0.0f, 0.0f};   // xyz: vel, w: pressure
    glm::vec4 force{0.0f, 0.0f, 0.0f, 0.0f};               // xyz: force, w: padding
};

enum class GpuFluidPreset {
    DAM_BREAK_MASSIVE,
    HIGH_DROP_SPLASH,
    DUAL_HIGH_SPEED_COLLISION
};

class GpuFluidSimulation {
public:
    // SPH Physics parameters
    float particleMass = 0.02f;
    float smoothingRadius = 0.15f;
    float restDensity = 1000.0f;
    float gasStiffness = 400.0f;
    float viscosity = 0.35f;
    float boundaryDamping = -0.45f;
    float particleRadius = 0.045f;

    // Environmental Gravity
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    int gravityMode = 0; // 0 = Standard, 1 = Inverted, 2 = Zero-G, 3 = Lateral Swirl

    // 3D Glass tank boundaries
    glm::vec3 boxMin{-1.3f, 0.0f, -1.3f};
    glm::vec3 boxMax{ 1.3f, 2.4f,  1.3f};

    // GPU Storage & Shaders
    unsigned int particleSSBO = 0;
    unsigned int dummyVAO = 0;
    unsigned int numParticles = 0;

    std::unique_ptr<Shader> densityPressureShader;
    std::unique_ptr<Shader> forceIntegrationShader;
    std::unique_ptr<Shader> splashImpulseShader;

    GpuFluidSimulation() = default;

    ~GpuFluidSimulation() {
        if (particleSSBO != 0) glDeleteBuffers(1, &particleSSBO);
        if (dummyVAO != 0) glDeleteVertexArrays(1, &dummyVAO);
    }

    GpuFluidSimulation(const GpuFluidSimulation&) = delete;
    GpuFluidSimulation& operator=(const GpuFluidSimulation&) = delete;

    void init() {
        // Compile GPU Compute Shaders executing on RTX 3050 CUDA cores
        initComputeShaders();

        // Create empty VAO for vertex pulling in OpenGL 4.3+
        glGenVertexArrays(1, &dummyVAO);

        // Create SSBO on RTX 3050 VRAM
        glGenBuffers(1, &particleSSBO);

        // Spawn initial scenario
        resetToPreset(GpuFluidPreset::DAM_BREAK_MASSIVE);
    }

    void resetToPreset(GpuFluidPreset preset) {
        std::vector<GPUParticle> initialParticles;
        float spacing = particleRadius * 1.55f;

        if (preset == GpuFluidPreset::DAM_BREAK_MASSIVE) {
            // Massive fluid column on left side
            for (float y = boxMin.y + 0.1f; y <= boxMin.y + 2.1f; y += spacing) {
                for (float z = boxMin.z + 0.15f; z <= boxMax.z - 0.15f; z += spacing) {
                    for (float x = boxMin.x + 0.15f; x <= boxMin.x + 1.15f; x += spacing) {
                        GPUParticle p;
                        p.position_density = glm::vec4(x, y, z, restDensity);
                        p.velocity_pressure = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                        initialParticles.push_back(p);
                    }
                }
            }
        }
        else if (preset == GpuFluidPreset::HIGH_DROP_SPLASH) {
            // Pool on bottom
            for (float y = boxMin.y + 0.08f; y <= boxMin.y + 0.45f; y += spacing) {
                for (float z = boxMin.z + 0.2f; z <= boxMax.z - 0.2f; z += spacing) {
                    for (float x = boxMin.x + 0.2f; x <= boxMax.x - 0.2f; x += spacing) {
                        GPUParticle p;
                        p.position_density = glm::vec4(x, y, z, restDensity);
                        initialParticles.push_back(p);
                    }
                }
            }
            // Dense suspended droplet mass dropping from top
            for (float y = boxMin.y + 1.2f; y <= boxMin.y + 2.2f; y += spacing) {
                for (float z = -0.6f; z <= 0.6f; z += spacing) {
                    for (float x = -0.6f; x <= 0.6f; x += spacing) {
                        GPUParticle p;
                        p.position_density = glm::vec4(x, y, z, restDensity);
                        p.velocity_pressure = glm::vec4(0.0f, -2.5f, 0.0f, 0.0f);
                        initialParticles.push_back(p);
                    }
                }
            }
        }
        else if (preset == GpuFluidPreset::DUAL_HIGH_SPEED_COLLISION) {
            // Left block jet
            for (float y = boxMin.y + 0.1f; y <= boxMin.y + 1.5f; y += spacing) {
                for (float z = -0.6f; z <= 0.6f; z += spacing) {
                    for (float x = boxMin.x + 0.15f; x <= boxMin.x + 0.85f; x += spacing) {
                        GPUParticle p;
                        p.position_density = glm::vec4(x, y, z, restDensity);
                        p.velocity_pressure = glm::vec4(3.2f, 0.0f, 0.0f, 0.0f);
                        initialParticles.push_back(p);
                    }
                }
            }
            // Right block jet
            for (float y = boxMin.y + 0.1f; y <= boxMin.y + 1.5f; y += spacing) {
                for (float z = -0.6f; z <= 0.6f; z += spacing) {
                    for (float x = boxMax.x - 0.85f; x <= boxMax.x - 0.15f; x += spacing) {
                        GPUParticle p;
                        p.position_density = glm::vec4(x, y, z, restDensity);
                        p.velocity_pressure = glm::vec4(-3.2f, 0.0f, 0.0f, 0.0f);
                        initialParticles.push_back(p);
                    }
                }
            }
        }

        numParticles = static_cast<unsigned int>(initialParticles.size());

        // Upload initial positions once to RTX 3050 VRAM SSBO (0 bytes transferred during runtime)
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, numParticles * sizeof(GPUParticle), initialParticles.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        std::cout << "[RTX 3050 VRAM] Allocated SSBO for " << numParticles 
                  << " particles (" << (numParticles * sizeof(GPUParticle) / 1024.0f) << " KB / 4096 MB VRAM).\n";
    }

    void cycleGravity() {
        gravityMode = (gravityMode + 1) % 4;
        switch (gravityMode) {
            case 0: gravity = glm::vec3(0.0f, -9.81f, 0.0f); break; // Standard downward
            case 1: gravity = glm::vec3(0.0f,  6.5f,  0.0f); break; // Inverted anti-gravity
            case 2: gravity = glm::vec3(0.0f,  0.0f,  0.0f); break; // Zero-G float
            case 3: gravity = glm::vec3(8.0f, -3.0f,  0.0f); break; // Lateral vortex swirl
        }
    }

    void injectSplashImpulse(float strength = 6.0f) {
        if (!splashImpulseShader || numParticles == 0) return;

        splashImpulseShader->use();
        splashImpulseShader->setUint("numParticles", numParticles);
        splashImpulseShader->setFloat("strength", strength);
        splashImpulseShader->setVec3("boxMin", boxMin);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);

        unsigned int workgroups = (numParticles + 255) / 256;
        glDispatchCompute(workgroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void step(float dt) {
        if (numParticles == 0) return;

        dt = std::min(dt, 0.016f);
        unsigned int workgroups = (numParticles + 255) / 256;

        float h = smoothingRadius;
        float poly6Const = static_cast<float>(315.0 / (64.0 * std::numbers::pi * std::pow(static_cast<double>(h), 9)));
        float spikyGradConst = static_cast<float>(-45.0 / (std::numbers::pi * std::pow(static_cast<double>(h), 6)));
        float viscLapConst = static_cast<float>(45.0 / (std::numbers::pi * std::pow(static_cast<double>(h), 6)));

        // Pass 1: Compute Density & Pressure on RTX 3050 CUDA cores
        densityPressureShader->use();
        densityPressureShader->setUint("numParticles", numParticles);
        densityPressureShader->setFloat("particleMass", particleMass);
        densityPressureShader->setFloat("smoothingRadius", smoothingRadius);
        densityPressureShader->setFloat("restDensity", restDensity);
        densityPressureShader->setFloat("gasStiffness", gasStiffness);
        densityPressureShader->setFloat("poly6Const", poly6Const);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
        glDispatchCompute(workgroups, 1, 1);

        // Memory barrier to synchronize writes before force calculation
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Pass 2: Compute Forces (Pressure, Viscosity, Gravity) and Euler Integration
        forceIntegrationShader->use();
        forceIntegrationShader->setUint("numParticles", numParticles);
        forceIntegrationShader->setFloat("particleMass", particleMass);
        forceIntegrationShader->setFloat("smoothingRadius", smoothingRadius);
        forceIntegrationShader->setFloat("viscosity", viscosity);
        forceIntegrationShader->setFloat("spikyGradConst", spikyGradConst);
        forceIntegrationShader->setFloat("viscLapConst", viscLapConst);
        forceIntegrationShader->setVec3("gravity", gravity);
        forceIntegrationShader->setVec3("boxMin", boxMin);
        forceIntegrationShader->setVec3("boxMax", boxMax);
        forceIntegrationShader->setFloat("boundaryDamping", boundaryDamping);
        forceIntegrationShader->setFloat("particleRadius", particleRadius);
        forceIntegrationShader->setFloat("dt", dt);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
        glDispatchCompute(workgroups, 1, 1);

        // Barrier for both compute reads and subsequent vertex pulling
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    }

private:
    void initComputeShaders() {
        // Compute Shader 1: Density & Pressure
        const char* densityCompSrc = R"(#version 430 core
layout(local_size_x = 256) in;

struct GPUParticle {
    vec4 position_density;
    vec4 velocity_pressure;
    vec4 force;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    GPUParticle particles[];
};

uniform uint numParticles;
uniform float particleMass;
uniform float smoothingRadius;
uniform float restDensity;
uniform float gasStiffness;
uniform float poly6Const;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    vec3 pos_i = particles[i].position_density.xyz;
    float h = smoothingRadius;
    float h2 = h * h;
    float density = 0.0;

    for (uint j = 0u; j < numParticles; ++j) {
        vec3 rVec = pos_i - particles[j].position_density.xyz;
        float r2 = dot(rVec, rVec);
        if (r2 < h2) {
            float term = h2 - r2;
            density += particleMass * poly6Const * term * term * term;
        }
    }

    density = max(density, restDensity);
    float pressure = gasStiffness * (density - restDensity);

    particles[i].position_density.w = density;
    particles[i].velocity_pressure.w = pressure;
}
)";

        // Compute Shader 2: Forces & Integration
        const char* forceCompSrc = R"(#version 430 core
layout(local_size_x = 256) in;

struct GPUParticle {
    vec4 position_density;
    vec4 velocity_pressure;
    vec4 force;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    GPUParticle particles[];
};

uniform uint numParticles;
uniform float particleMass;
uniform float smoothingRadius;
uniform float viscosity;
uniform float spikyGradConst;
uniform float viscLapConst;
uniform vec3 gravity;
uniform vec3 boxMin;
uniform vec3 boxMax;
uniform float boundaryDamping;
uniform float particleRadius;
uniform float dt;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    vec3 pos_i = particles[i].position_density.xyz;
    float rho_i = particles[i].position_density.w;
    float p_i = particles[i].velocity_pressure.w;
    vec3 vel_i = particles[i].velocity_pressure.xyz;

    float h = smoothingRadius;
    vec3 fPressure = vec3(0.0);
    vec3 fViscosity = vec3(0.0);

    for (uint j = 0u; j < numParticles; ++j) {
        if (i == j) continue;

        vec3 pos_j = particles[j].position_density.xyz;
        vec3 rVec = pos_i - pos_j;
        float r = length(rVec);

        if (r > 1e-5 && r < h) {
            vec3 rDir = rVec / r;
            float h_r = h - r;

            float rho_j = particles[j].position_density.w;
            float p_j = particles[j].velocity_pressure.w;
            vec3 vel_j = particles[j].velocity_pressure.xyz;

            float pTerm = (p_i + p_j) / (2.0 * rho_j);
            fPressure += -particleMass * pTerm * (spikyGradConst * h_r * h_r) * rDir;

            fViscosity += viscosity * particleMass * (vel_j - vel_i) / rho_j * (viscLapConst * h_r);
        }
    }

    vec3 fGravity = gravity * rho_i;
    vec3 totalForce = fPressure + fViscosity + fGravity;

    // Numerical integration directly on GPU
    vec3 acceleration = totalForce / rho_i;
    vel_i += acceleration * dt;
    pos_i += vel_i * dt;

    // Glass container boundary collisions
    float pad = particleRadius * 0.9;

    if (pos_i.x < boxMin.x + pad) {
        pos_i.x = boxMin.x + pad;
        vel_i.x *= boundaryDamping;
    } else if (pos_i.x > boxMax.x - pad) {
        pos_i.x = boxMax.x - pad;
        vel_i.x *= boundaryDamping;
    }

    if (pos_i.y < boxMin.y + pad) {
        pos_i.y = boxMin.y + pad;
        vel_i.y *= boundaryDamping;
    } else if (pos_i.y > boxMax.y - pad) {
        pos_i.y = boxMax.y - pad;
        vel_i.y *= boundaryDamping;
    }

    if (pos_i.z < boxMin.z + pad) {
        pos_i.z = boxMin.z + pad;
        vel_i.z *= boundaryDamping;
    } else if (pos_i.z > boxMax.z - pad) {
        pos_i.z = boxMax.z - pad;
        vel_i.z *= boundaryDamping;
    }

    particles[i].position_density.xyz = pos_i;
    particles[i].velocity_pressure.xyz = vel_i;
    particles[i].force.xyz = totalForce;
}
)";

        // Compute Shader 3: Splash Wave Impulse on GPU
        const char* splashCompSrc = R"(#version 430 core
layout(local_size_x = 256) in;

struct GPUParticle {
    vec4 position_density;
    vec4 velocity_pressure;
    vec4 force;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    GPUParticle particles[];
};

uniform uint numParticles;
uniform float strength;
uniform vec3 boxMin;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= numParticles) return;

    vec3 pos = particles[i].position_density.xyz;
    float dist = length(pos.xz);
    if (dist < 0.7 && pos.y < boxMin.y + 0.8) {
        particles[i].velocity_pressure.y += strength * (1.0 - dist / 0.7);
        particles[i].velocity_pressure.x += (pos.x >= 0.0 ? 1.0 : -1.0) * 2.0;
        particles[i].velocity_pressure.z += (pos.z >= 0.0 ? 1.0 : -1.0) * 2.0;
    }
}
)";

        densityPressureShader = std::make_unique<Shader>(densityCompSrc);
        forceIntegrationShader = std::make_unique<Shader>(forceCompSrc);
        splashImpulseShader = std::make_unique<Shader>(splashCompSrc);
    }
};
