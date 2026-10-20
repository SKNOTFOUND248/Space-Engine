# ANTIGRAVITY.md — Architectural Blueprint & AI Pair-Programming Notes

This document describes the engineering methodology, AI pair-programming integration, and architectural foundations of **ORBITAL**, developed using the **Google DeepMind Antigravity** advanced agentic coding environment.

---

## 1. Project Background & Provenance

ORBITAL was developed based on the **SpaceEngine Manual Research & Independent Implementation Blueprint** (`ORBITAL_SpaceEngine_Research_and_Implementation_Blueprint.txt`).

The objective is to implement an original, testable, scientifically grounded space simulation engine in native modern C++20 and OpenGL that compiles and runs cleanly in Microsoft Visual Studio on Windows, adhering to:
- Strict SI physical units ($m, kg, s, rad$).
- Dual scale modes: Laboratory scale (enhanced visual sizes for inspection) and True Physical scale ($1:1$ astronomical proportions).
- Camera-relative floating-origin rendering to prevent IEEE 754 32-bit float vertex jitter at interplanetary coordinates ($10^{11}\text{ m}$).
- Deterministic numerical integration and live energy conservation tracking.

---

## 2. Antigravity Skill & Tool Integration

Development on this repository follows the `visual-studio-cpp-project` skill:
- **Project Structure**: Preserves the native `CPPsumulations1.vcxproj` and `CPPsumulations1.slnx` MSBuild files.
- **Dependency Ingestion**: Automatically driven by vcpkg in manifest mode (`vcpkg.json`), managing `glfw3`, `glad`, `glm`, and `imgui` without manual link configurations.
- **Continuous Verification**: Every milestone is verified via MSBuild CLI compilation (`/p:Configuration=Release /p:Platform=x64`) and automated headless test runs (`CPPsumulations1.exe --test-only`).

---

## 3. Modular Code Layout

```
CPPsumulations1/
├── src/
│   ├── core/
│   │   ├── Types.h            # SI astronomical constants, BodyType, IntegratorType, BodyID
│   │   ├── Time.h             # SimulationClock with fixed timesteps & bounded substeps
│   │   ├── Json.h             # Zero-dependency C++20 recursive-descent JSON parser/serializer
│   │   └── VFS.h              # Virtual File System with path traversal sandboxing & add-on discovery
│   ├── physics/
│   │   ├── CelestialBody.h    # State representation, orbital history trails, metric calculations
│   │   ├── GravitySolver.h    # O(N^2) direct Newtonian pairwise gravity with softening
│   │   ├── Integrator.h       # Symplectic Velocity Verlet, Semi-Implicit Euler, RK4
│   │   ├── CollisionSystem.h  # Inelastic momentum/mass-conserving coalescence & elastic bounce
│   │   ├── OrbitAnalyzer.h    # Keplerian elements (a, e, i, rp, ra, T) & conic prediction paths
│   │   └── EnergyDiagnostics.h# Kinetic, potential, total energy, and relative drift (dE/E0)
│   ├── relativity/
│   │   └── RelativitySolver.h # Schwarzschild metric, photon sphere, ISCO, gravitational time dilation
│   ├── catalog/
│   │   ├── DeepSpaceCatalog.h # Galaxies (M31, M33), Nebulae (M42, M1), Star Clusters (M45)
│   │   └── CatalogLoader.h    # Section 36 scenario serializer and deserializer
│   ├── scripting/
│   │   ├── ScenarioScript.h   # Declarative command definitions and guided educational tutorials
│   │   └── ScriptRunner.h     # Non-blocking timeline executor with smooth camera easing
│   ├── graphics/
│   │   ├── SphereMesh.h       # Procedural UV sphere geometry generator
│   │   ├── RingMesh.h         # Planetary rings & relativistic accretion disk geometry
│   │   ├── OrbitRenderer.h    # GPU rendering of conic ellipses and orbital history trails
│   │   ├── Starfield.h        # 3D celestial starfield with spectral star temperatures
│   │   ├── DeepSpaceRenderer.h# Point-particle logarithmic spiral galaxies & emission nebulae
│   │   └── ProceduralPlanet.h # 5-octave GPU Simplex fBm noise, 6 biomes, Rayleigh limb scattering
│   ├── camera/
│   │   └── CameraController.h # Floating-origin view transform, exponential zoom, orbit & free-fly
│   ├── ui/
│   │   └── UIManager.h        # Dear ImGui scientific interface, Inspector, Explorer, Experiments
│   └── tests/
│       └── PhysicsTests.h     # 18-point automated validation test suite
├── data/
│   ├── catalogs/              # Deep space catalogs & planetary definitions
│   └── scenarios/             # Pre-built scenario JSON files
├── addons/                    # User-created add-on packages with manifest.json
├── vcpkg.json                 # Pinned dependencies
├── CPPsumulations1.vcxproj    # MSBuild project definition
└── CPPsumulations1.cpp        # Application entry point and graphics loop
```

---

## 4. Guidelines for Future AI Pair Programmers

When working on ORBITAL with Antigravity or any agentic coding assistant:
1. **Never break MSBuild**: Always verify both `Debug|x64` and `Release|x64` compile cleanly after modifying code.
2. **Never break the test suite**: Run `CPPsumulations1.exe --test-only` before finishing any task. All 18 tests must pass.
3. **Preserve Physical SI Integrity**: Keep physics in SI units (`double`). Never modify physics equations for visual aesthetic convenience; adjust display scale in the renderer instead.
4. **Register all new headers in `.vcxproj` and `.vcxproj.filters`**: MSBuild requires new C++ headers to be listed in `CPPsumulations1.vcxproj` under `<ClInclude Include="..." />`.
