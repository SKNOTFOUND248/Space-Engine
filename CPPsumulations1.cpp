#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "src/core/Types.h"
#include "src/core/Time.h"
#include "src/core/Json.h"
#include "src/core/VFS.h"
#include "src/physics/CelestialBody.h"
#include "src/physics/GravitySolver.h"
#include "src/physics/Integrator.h"
#include "src/physics/CollisionSystem.h"
#include "src/physics/OrbitAnalyzer.h"
#include "src/physics/EnergyDiagnostics.h"
#include "src/relativity/RelativitySolver.h"
#include "src/simulation/Universe.h"
#include "src/camera/CameraController.h"
#include "src/catalog/DeepSpaceCatalog.h"
#include "src/catalog/CatalogLoader.h"
#include "src/scripting/ScenarioScript.h"
#include "src/scripting/ScriptRunner.h"
#include "src/graphics/SphereMesh.h"
#include "src/graphics/RingMesh.h"
#include "src/graphics/OrbitRenderer.h"
#include "src/graphics/Starfield.h"
#include "src/graphics/DeepSpaceRenderer.h"
#include "src/graphics/ProceduralPlanet.h"
#include "src/ui/UIManager.h"
#include "src/tests/PhysicsTests.h"
#include "Shader.h"

#include <iostream>
#include <vector>
#include <chrono>

using namespace Orbital;

// Window resolution
constexpr unsigned int SCR_WIDTH = 1440;
constexpr unsigned int SCR_HEIGHT = 860;

// Subsystems
Universe universe;
CameraController camera;
UIManager ui;
DeepSpaceCatalog deepSpaceCatalog;
DeepSpaceRenderer deepSpaceRenderer;
ScriptRunner scriptRunner;

// Mouse navigation state
bool isMouseDragging = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

// Ring & Accretion Disk Shader
const char* ringVertexShader = R"(#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 FragPosView;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoord = aTexCoord;
    vec4 viewPos = view * model * vec4(aPos, 1.0);
    FragPosView = viewPos.xyz;
    gl_Position = projection * viewPos;
}
)";

const char* ringFragmentShader = R"(#version 430 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPosView;

uniform vec4 ringColor;
uniform bool isAccretionDisk;

void main()
{
    // Radial band texture
    float u = TexCoord.x; // 0 = inner, 1 = outer
    float alpha = sin(u * 3.14159); // Soft fading at inner and outer boundaries

    if (isAccretionDisk) {
        // High-energy relativistic gradient (white-hot inner to golden-red outer)
        vec3 innerHot = vec3(1.0, 0.95, 0.85);
        vec3 midOrange = vec3(1.0, 0.6, 0.15);
        vec3 outerRed = vec3(0.7, 0.15, 0.05);

        vec3 diskColor = mix(innerHot, midOrange, u);
        diskColor = mix(diskColor, outerRed, u * u);

        // Doppler beaming approximation: one side appears slightly brighter
        float doppler = 0.8 + 0.4 * sin(TexCoord.y * 6.28318);
        FragColor = vec4(diskColor * doppler, alpha * 0.95);
    } else {
        // Planetary rings (Saturn-style dust bands)
        float bands = 0.6 + 0.4 * sin(u * 45.0) * cos(u * 15.0);
        FragColor = vec4(ringColor.rgb * bands, ringColor.a * alpha);
    }
}
)";

// Orbit Line Shader
const char* lineVertexShader = R"(#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char* lineFragmentShader = R"(#version 430 core
out vec4 FragColor;
uniform vec4 lineColor;

void main()
{
    FragColor = lineColor;
}
)";

// Starfield Shader
const char* starVertexShader = R"(#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 StarColor;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    StarColor = aColor;
    // Remove translation from view matrix so stars remain infinitely far
    mat4 viewNoTrans = mat4(mat3(view));
    gl_Position = projection * viewNoTrans * vec4(aPos, 1.0);
    gl_PointSize = 2.0;
}
)";

const char* starFragmentShader = R"(#version 430 core
out vec4 FragColor;
in vec3 StarColor;

void main()
{
    FragColor = vec4(StarColor, 1.0);
}
)";

// GLFW Callbacks
void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int /*mods*/) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            isMouseDragging = true;
        } else if (action == GLFW_RELEASE) {
            isMouseDragging = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* /*window*/, double xpos, double ypos) {
    if (ImGui::GetIO().WantCaptureMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        return;
    }

    if (isMouseDragging) {
        double xOffset = xpos - lastMouseX;
        double yOffset = lastMouseY - ypos;
        camera.processOrbitRotation(xOffset, yOffset);
    }

    lastMouseX = xpos;
    lastMouseY = ypos;
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    camera.processZoom(yoffset);
}

int main(int argc, char** argv)
{
    std::cout << "=================================================================\n";
    std::cout << "         ORBITAL - 3D High-Fidelity Space Simulation Engine      \n";
    std::cout << "=================================================================\n\n";

    // Run automated physics verification tests
    TestReport report = PhysicsTests::runAll();

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-only") {
            std::cout << "[CI/TEST] --test-only argument passed. Exiting with code " << report.failedTests << "\n";
            return report.failedTests;
        }
    }

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ORBITAL - Space Simulation Engine", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "[RENDERER] " << glGetString(GL_RENDERER) << "\n";
    std::cout << "[OPENGL]   " << glGetString(GL_VERSION) << "\n\n";

    // Enable OpenGL features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize Virtual File System
    VFS::get().initialize(".");

    // Connect subsystems to UI
    ui.scriptRunner = &scriptRunner;
    ui.deepSpaceCatalog = &deepSpaceCatalog;

    // Initialize UI Manager
    ui.init(window);

    // Initialize Deep Space Renderer (Galaxies and Nebulae)
    deepSpaceRenderer.init();

    // Shaders
    Shader bodyShader(ProceduralPlanetSurface::getVertexShader(), ProceduralPlanetSurface::getFragmentShader());
    Shader ringShader(ringVertexShader, ringFragmentShader);
    Shader lineShader(lineVertexShader, lineFragmentShader);
    Shader starShader(starVertexShader, starFragmentShader);

    // Geometry Meshes
    SphereMesh sphereMesh(48, 32);
    RingMesh ringMesh(64);
    OrbitRenderer orbitRenderer;
    Starfield starfield;

    // Timing tracking
    auto lastTime = std::chrono::high_resolution_clock::now();
    double frameCount = 0.0;
    double fps = 60.0;
    double fpsTimer = 0.0;
    double physicsTimeMs = 0.0;

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        // FPS calculation
        frameCount++;
        fpsTimer += dt;
        if (fpsTimer >= 1.0) {
            fps = frameCount / fpsTimer;
            frameCount = 0.0;
            fpsTimer = 0.0;
        }

        glfwPollEvents();

        // 0. Advance Guided Scenario Script / Interactive Experiment
        scriptRunner.update(universe, camera, dt);

        // 1. Advance Physics Simulation
        auto physStart = std::chrono::high_resolution_clock::now();
        universe.update(dt);
        auto physEnd = std::chrono::high_resolution_clock::now();
        physicsTimeMs = std::chrono::duration<double, std::milli>(physEnd - physStart).count();

        // Update Camera target position
        const CelestialBody* selectedBody = universe.getBody(universe.selectedBodyId);
        camera.update(selectedBody, dt);

        // Rendering setup
        glClearColor(0.015f, 0.02f, 0.035f, 1.0f); // Deep astronomical space
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int winWidth = 0, winHeight = 0;
        glfwGetFramebufferSize(window, &winWidth, &winHeight);
        float aspectRatio = (winHeight > 0) ? static_cast<float>(winWidth) / static_cast<float>(winHeight) : 1.0f;

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);

        // A. Render Celestial Starfield (Depth Test off or behind scene)
        glDisable(GL_DEPTH_TEST);
        starShader.use();
        starShader.setMat4("view", view);
        starShader.setMat4("projection", projection);
        starfield.draw();

        // A2. Render Deep Space Objects (Spiral Galaxies & Diffuse Nebulae)
        deepSpaceRenderer.render(camera, deepSpaceCatalog, projection);
        glEnable(GL_DEPTH_TEST);

        // B. Render Orbital Trajectory Trails & Conic Section Predictions
        lineShader.use();
        lineShader.setMat4("view", view);
        lineShader.setMat4("projection", projection);

        // Draw predicted Keplerian orbit for selected body
        const CelestialBody* primary = universe.getPrimaryBody();
        if (selectedBody && primary && selectedBody->id != primary->id) {
            auto conicPoints = OrbitAnalyzer::generateConicPoints(*selectedBody, *primary, 240);
            lineShader.setVec4("lineColor", glm::vec4(0.3f, 0.8f, 1.0f, 0.85f));
            orbitRenderer.drawConicPrediction(conicPoints, camera);
        }

        // Draw past orbital trails
        for (const auto& b : universe.bodies) {
            if (b.isDestroyed) continue;
            lineShader.setVec4("lineColor", glm::vec4(b.color * 0.7f, 0.45f));
            orbitRenderer.drawTrail(b, camera);
        }

        // C. Render Celestial Bodies (Stars, Planets, Moons, Black Holes)
        bodyShader.use();
        bodyShader.setMat4("view", view);
        bodyShader.setMat4("projection", projection);

        for (const auto& b : universe.bodies) {
            if (b.isDestroyed) continue;

            glm::vec3 renderPos = camera.toRenderSpace(b.position);
            float visualRadius = camera.getVisualRadius(b);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, renderPos);
            model = glm::rotate(model, glm::radians(static_cast<float>(b.axialTilt)), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::rotate(model, static_cast<float>(b.rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(visualRadius));

            bodyShader.setMat4("model", model);
            bodyShader.setVec3("baseColor", b.color);
            bodyShader.setFloat("emission", b.emission);
            bodyShader.setBool("isBlackHole", b.type == BodyType::BlackHole);
            bodyShader.setBool("isGasGiant", b.biomeType == 5);
            bodyShader.setInt("biomeType", b.biomeType);
            bodyShader.setFloat("seed", b.seed);
            bodyShader.setFloat("simTime", static_cast<float>(universe.clock.simulatedTime));
            bodyShader.setFloat("seaLevel", b.seaLevel);
            bodyShader.setFloat("cloudCoverage", b.cloudCoverage);
            bodyShader.setBool("hasAtmosphere", b.hasAtmosphere);
            bodyShader.setVec3("atmosphereColor", b.atmosphereColor);
            bodyShader.setFloat("atmosphereDensity", b.atmosphereDensity);

            sphereMesh.draw();

            // Render Rings or Accretion Disks
            if (b.hasRings) {
                glDisable(GL_CULL_FACE);
                ringShader.use();
                ringShader.setMat4("view", view);
                ringShader.setMat4("projection", projection);

                float innerScale = visualRadius * 1.3f;
                float outerScale = visualRadius * 2.8f;
                if (b.type == BodyType::BlackHole) {
                    innerScale = visualRadius * 1.5f;
                    outerScale = visualRadius * 5.0f;
                }

                glm::mat4 ringModel = glm::mat4(1.0f);
                ringModel = glm::translate(ringModel, renderPos);
                ringModel = glm::rotate(ringModel, glm::radians(static_cast<float>(b.axialTilt)), glm::vec3(0.0f, 0.0f, 1.0f));
                ringModel = glm::scale(ringModel, glm::vec3(outerScale * 0.5f));

                ringShader.setMat4("model", ringModel);
                ringShader.setVec4("ringColor", b.ringColor);
                ringShader.setBool("isAccretionDisk", b.type == BodyType::BlackHole);

                ringMesh.draw();

                bodyShader.use(); // restore
            }
        }

        // D. Render Dear ImGui Interface
        ui.beginFrame();
        ui.render(universe, camera, fps, physicsTimeMs);

        glfwSwapBuffers(window);
    }

    deepSpaceRenderer.shutdown();
    ui.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
