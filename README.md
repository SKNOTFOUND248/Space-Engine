# ORBITAL — High-Fidelity 3D Space Simulation Engine

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?style=for-the-badge&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.3_Core-5586A4?style=for-the-badge&logo=opengl)](https://www.opengl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen?style=for-the-badge&logo=visualstudio)](https://visualstudio.microsoft.com/)
[![Tests](https://img.shields.io/badge/Physics_Tests-18%2F18_Passed-success?style=for-the-badge)](https://github.com/SKNOTFOUND248/Space-Engine)

**ORBITAL** is an interactive, real-time 3D celestial mechanics and astrophysics laboratory engine built in **modern C++20 and OpenGL 4.3 Core**. Inspired by the exploration freedom of *SpaceEngine* and the rigor of computational orbital dynamics, ORBITAL combines Newtonian N-body gravitational dynamics, general relativistic metrics, symplectic numerical integration, procedural GPU planetary biomes, extragalactic deep-space rendering, and interactive scientific guided experiments.

---

## Key Highlights

- **Double-Precision SI Physics Engine**: All positions, velocities, masses, and forces are simulated in 64-bit IEEE 754 precision (`double`, `glm::dvec3`) using strict SI units ($m, kg, s, rad$).
- **Floating-Origin Rendering**: Seamlessly spans astronomical scales from planetary radii ($10^6\text{ m}$) to interstellar distances ($10^{13}\text{ m}$) by computing camera-relative offsets in double precision before GPU upload, completely eliminating floating-point jitter.
- **Symplectic & Higher-Order Integrators**: Includes Velocity Verlet (2nd-order symplectic, preserving phase-space volume and long-term energy conservation), Semi-Implicit Euler, and Runge-Kutta 4 (RK4).
- **Physical Collision Invariants**: Inelastic mergers strictly conserve total mass ($M = m_1 + m_2$) and linear momentum ($\mathbf{P} = \mathbf{p}_1 + \mathbf{p}_2$) with volume-conserving coalescence ($R = \sqrt[3]{R_1^3 + R_2^3}$) and real-time kinetic impact energy logging.
- **General Relativity & Black Holes**: Computes exact Schwarzschild metrics ($r_s = 2GM/c^2$, photon sphere at $1.5 r_s$, ISCO at $3.0 r_s$), gravitational time dilation ($\sqrt{1 - r_s/r}$), Doppler-beamed relativistic accretion disks, and 1PN Post-Newtonian periapsis advance.
- **Procedural GPU Planet Biomes**: 5-octave Simplex fBm noise running in GLSL shaders generating 6 biomes (Terrestrial, Desert, Ocean World, Ice World, Volcanic, Gas Giant) with ocean specular glint, rotating cloud layers, and Rayleigh/Mie limb scattering with sunset reddening.
- **Deep-Space Object (DSO) Catalog**: Real-time rendering of extragalactic spiral galaxies (logarithmic density waves $r = r_0 e^{k\theta}$) and diffuse emission nebulae with additive particle blending.
- **Guided Tours & Educational Experiments**: Interactive step-by-step experiment runner with dialogue cards, camera interpolation, and automated observation reports.
- **Virtual File System (VFS) & Add-on Packaging**: Sandboxed asset loading rejecting path traversal attacks (`..`), reading versioned JSON scenarios and modular add-on packages.

---

## Engine Architecture

```
                                  +---------------------------------------+
                                  |            SimulationClock            |
                                  | Fixed Timestep dt & Bounded Substeps  |
                                  +---------------------------------------+
                                                     |
                                                     v
+-----------------------+         +---------------------------------------+         +-----------------------+
|     GravitySolver     | ------> |              Integrators              | ------> |    CollisionSystem    |
| Mutual O(N^2) Gravity |         | Verlet (Symplectic) / Euler / RK4     |         | Inelastic Mass/Momentum|
+-----------------------+         +---------------------------------------+         +-----------------------+
                                                     |
                                                     v
+-----------------------+         +---------------------------------------+         +-----------------------+
|   RelativitySolver    | <====== |             Universe State            | ======> |   OrbitAnalyzer &     |
| Schwarzschild & 1PN   |         |      Bodies, Registry, Primaries      |         |   EnergyDiagnostics   |
+-----------------------+         +---------------------------------------+         +-----------------------+
                                                     |
                                                     v
                                  +---------------------------------------+
                                  |            CameraController           |
                                  | Floating-Origin Double-Precision Cam  |
                                  +---------------------------------------+
                                                     |
                                                     v
+-----------------------+         +---------------------------------------+         +-----------------------+
|  Procedural Planets   |         |           DeepSpaceRenderer           |         |       UIManager       |
| Simplex Biomes & Atmo |         | Spiral Galaxies & Emission Nebulae    |         | ImGui Lab & Telemetry |
+-----------------------+         +---------------------------------------+         +-----------------------+
```

---

## 9 Built-in Scenarios

1. **Complete Solar System**: Sun, Mercury, Venus, Earth, Moon, Mars, Jupiter, Saturn (with rings), Uranus, and Neptune in verified circular Keplerian orbits.
2. **Two-Body Kepler Lab**: Clean two-body system demonstrating bound elliptical orbits ($e < 1.0$) with real-time orbital elements ($a, e, i, r_p, r_a, T$).
3. **Three-Body Chaos**: Symmetrical equilateral 3-body system demonstrating gravitational chaos, three-body interaction, and extreme sensitivity to initial conditions.
4. **Binary Star System**: Two mutually orbiting massive stars with a circumbinary planet.
5. **Planetary Collision**: High-speed impact simulation demonstrating exact mass and momentum conservation and impact energy dissipation.
6. **Supermassive Black Hole**: Relativistic horizon ($r_s$) with glowing Doppler-brightened accretion disk, photon ring, and orbiting test probes near ISCO ($3.0 r_s$).
7. **Mass vs. Gravity Laboratory**: Interactive test planet with fixed gravimeter probe demonstrating Newtonian $a = GM/r^2$ drop-off.
8. **Relativistic Precession**: High-eccentricity close orbit demonstrating 1PN Post-Newtonian perihelion advance (Mercury-like relativistic advance).
9. **Custom Empty Universe**: Blank sandbox for building custom planetary systems from scratch.

---

## Controls

| Action | Control | Description |
| :--- | :--- | :--- |
| **Orbit Camera** | Left/Right Click + Drag | Rotates the view around the focused body |
| **Exponential Zoom**| Mouse Scroll Wheel | Zooms in/out smoothly across $10^6$ to $10^{13}$ meters |
| **Select / Focus** | Click in Object Explorer | Focuses camera on target celestial body |
| **Play / Pause** | "Play / Pause" Toolbar | Freezes or resumes numerical integration |
| **Step 1 Timestep**| "Step" Toolbar | Advances simulation by one fixed timestep ($\Delta t$) |
| **Time Warp** | Time Warp Dropdown | Accelerates time (1x up to 30 days per real second) |
| **Scale Mode** | "Lab Scale" / "1:1 Physical" | Toggles magnified planet view vs true $1:1$ physical view |
| **Guided Experiments**| "Guided Experiments" Toolbar| Opens automated educational scientific tutorials |
| **DSO Catalog** | "DSO Catalog" Toolbar | Opens deep-space catalog of galaxies, nebulae, clusters |
| **Add Body** | "+ Add Body" Toolbar | Opens dialog to spawn custom planets, stars, black holes |
| **Export Telemetry**| "Export Telemetry to CSV" | Exports telemetry and energy drift history to CSV |

---

## Automated Scientific Test Suite

ORBITAL features an automated test harness covering analytical physics, celestial mechanics, numerical conservation laws, and subsystem sandboxing:

```
======================================================
         ORBITAL SCIENTIFIC PHYSICS TEST SUITE        
======================================================
 [PASS] 1. Newtonian Gravitational Acceleration matches analytical formula
 [PASS] 2. Acceleration doubles when source mass doubles at fixed distance
 [PASS] 3. Acceleration falls by a factor of 4 when distance doubles
 [PASS] 4. Escape velocity matches analytical formula
 [PASS] 5. Schwarzschild radius scales linearly with mass
 [PASS] 6. Two-body Keplerian initial conditions produce bound orbit (e < 1.0)
 [PASS] 7. Velocity Verlet integration conserves energy within tolerance
 [PASS] 8. Inelastic collision merger strictly conserves mass and linear momentum
 [PASS] 9. Center of mass calculation matches analytical result
 [PASS] 10. Numerical non-finite / zero-distance handling produces no NaNs
 [PASS] 11. Photon sphere (1.5 rs) and ISCO (3.0 rs) metric exactness
 [PASS] 12. Gravitational time dilation asymptotically approaches 0 at horizon
 [PASS] 13. Kepler's Third Law orbital period verification (T = 2*pi*sqrt(a^3/mu))
 [PASS] 14. JSON Parser & Serializer round-trip parsing of nested data
 [PASS] 15. Virtual File System path traversal rejection & security checks
 [PASS] 16. Scenario catalog serialization & deserialization round-trip
 [PASS] 17. Deep-space equatorial coordinate to Cartesian 3D projection
 [PASS] 18. Scenario script command validation and step integrity
------------------------------------------------------
 Test Summary: 18/18 Passed (ALL PASSED)
======================================================
```

To run the test suite headlessly from terminal:
```powershell
.\x64\Release\CPPsumulations1.exe --test-only
```

---

## Quick Start & Building

### Prerequisites
- **Windows 10 / 11 (x64)**
- **Visual Studio 2022** or **Visual Studio 2026** with **Desktop development with C++**
- GPU supporting **OpenGL 4.3 Core Profile** or higher

### Build Steps
1. Clone the repository:
   ```powershell
   git clone https://github.com/SKNOTFOUND248/Space-Engine.git
   cd Space-Engine
   ```
2. Open `CPPsumulations1.slnx` or `CPPsumulations1.vcxproj` in Visual Studio.
3. Select **Release** | **x64**.
4. Press **Ctrl + Shift + B** to compile. All dependencies (`glfw3`, `glad`, `glm`, `imgui`) are automatically acquired via `vcpkg` manifest mode.
5. Press **F5** to launch!

For comprehensive build instructions and troubleshooting, see [INSTRUCTIONS.md](INSTRUCTIONS.md).

---

## Documentation Links

- [INSTRUCTIONS.md](INSTRUCTIONS.md) — Detailed user guide, keyboard/mouse controls, scenario authoring, and troubleshooting.
- [CONTRIBUTING.md](CONTRIBUTING.md) — Contribution guidelines, scientific honesty rules, and pull request workflow.
- [SECURITY.md](SECURITY.md) — Security policy, VFS sandboxing, and vulnerability disclosure.
- [ANTIGRAVITY.md](ANTIGRAVITY.md) — Engineering architecture, AI pair-programming methodology, and modular code layout.
- [PHYSICS_REFERENCE.md](PHYSICS_REFERENCE.md) — Detailed mathematical formulas for orbital mechanics, metrics, and integrators.

---

## License

This project is licensed under the [MIT License](LICENSE).
Third-party libraries (`GLFW`, `GLAD`, `GLM`, `Dear ImGui`) are subject to their respective open-source licenses.
All procedural shaders, simulation code, and scenarios are original implementations.
