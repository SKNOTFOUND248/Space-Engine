# Contributing to ORBITAL

Thank you for your interest in contributing to **ORBITAL**! We welcome contributions from astrophysicists, numerical simulation developers, graphics engineers, and UI/UX designers.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Prerequisites](#development-prerequisites)
- [Development Workflow](#development-workflow)
- [Engineering Standards](#engineering-standards)
- [Physics & Scientific Honesty Rules](#physics--scientific-honesty-rules)
- [Running Automated Tests](#running-automated-tests)
- [Submitting a Pull Request](#submitting-a-pull-request)

---

## Code of Conduct

We are committed to providing a friendly, rigorous, and welcoming environment for all contributors. Please treat fellow contributors with respect and focus critiques on technical and physical merits.

---

## Getting Started

1. **Fork the repository** on GitHub: `https://github.com/SKNOTFOUND248/Space-Engine`
2. **Clone your fork**:
   ```powershell
   git clone https://github.com/<your-username>/Space-Engine.git
   cd Space-Engine
   ```
3. **Open the solution**:
   Open `CPPsumulations1.slnx` or `CPPsumulations1.vcxproj` in Visual Studio 2022 or Visual Studio 2026.

---

## Development Prerequisites

- **OS**: Windows 10/11 (x64)
- **Compiler**: Microsoft Visual C++ (MSVC `v143` or `v145`), `/std:c++20`
- **Build System**: MSBuild (`.vcxproj`)
- **Package Manager**: [vcpkg](https://vcpkg.io/) (manifest mode via `vcpkg.json`)
  - Dependencies installed automatically:
    - `glfw3`
    - `glad` (OpenGL 4.3+ Core Profile)
    - `glm`
    - `imgui` (`glfw-binding`, `opengl3-binding`)

---

## Development Workflow

1. Create a feature branch off `main`:
   ```powershell
   git checkout -b feature/your-feature-name
   ```
2. Make your changes adhering to project coding standards.
3. Build the project in both **Debug|x64** and **Release|x64** configurations.
4. Run the automated test suite to verify no regressions:
   ```powershell
   .\x64\Release\CPPsumulations1.exe --test-only
   ```
5. Commit with descriptive commit messages following the Conventional Commits specification (e.g. `feat(physics): add adaptive Runge-Kutta-Fehlberg integrator`, `fix(vfs): resolve case sensitivity on catalog loading`).

---

## Engineering Standards

- **Language Standard**: ISO C++20 (`/std:c++20`).
- **SI Units Internally**: All physical parameters in memory and serialization must be in SI units:
  - Distance: meters ($m$)
  - Mass: kilograms ($kg$)
  - Time: seconds ($s$)
  - Angle: radians ($rad$) internally (degrees only for UI display)
  - Velocity: $m/s$, Acceleration: $m/s^2$, Energy: Joules ($J$)
- **Precision**: 64-bit IEEE 754 floating point (`double`, `glm::dvec3`) for all physical positions, velocities, and gravitational calculations. 32-bit floats are reserved exclusively for GPU rendering vertex data.
- **Floating-Origin Rendering**: Never send astronomical coordinates ($10^{11}\text{ m}$) directly to OpenGL shaders. Always subtract the camera's position in double precision on the CPU, and translate camera-relative coordinates to the GPU.
- **Resource Lifetime (RAII)**: OpenGL handles (VAO, VBO, shaders, textures) must be RAII-managed and cleanly deleted upon shutdown.
- **Defensive Error Handling**: Deserializers and parsers must catch non-finite values (`NaN`, `Inf`) and invalid schemas without crashing the engine.

---

## Physics & Scientific Honesty Rules

ORBITAL strictly distinguishes analytical laws from approximations:
1. **Newtonian Gravity**: Must be computed symmetrically without ordered-pair double-counting.
2. **Conservation Laws**: Collision mergers must strictly conserve mass ($M = m_1 + m_2$) and linear momentum ($\mathbf{P} = \mathbf{p}_1 + \mathbf{p}_2$). Any lost kinetic energy must be explicitly documented and reported as an inelastic impact dissipation sink.
3. **Relativistic Effects**: Clearly distinguish exact Schwarzschild analytical solutions (such as $r_s = 2GM/c^2$, photon sphere at $1.5 r_s$, ISCO at $3.0 r_s$) from Post-Newtonian approximations (1PN perihelion advance) and screen-space gravitational deflection shaders.
4. **No Fake Telemetry**: Never hardcode decorative UI numbers or fake orbital trajectories. Every readout in the Inspector and Diagnostics must derive from actual simulated state.

---

## Running Automated Tests

Before submitting any code, verify all 18 automated tests pass:
```powershell
# Using MSBuild from CLI:
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" CPPsumulations1.vcxproj /p:Configuration=Release /p:Platform=x64 -m

# Run headless test suite:
.\x64\Release\CPPsumulations1.exe --test-only
```
If adding a new physical calculation or feature, implement a corresponding test case in `src/tests/PhysicsTests.h`.

---

## Submitting a Pull Request

1. Push your branch to your fork:
   ```powershell
   git push origin feature/your-feature-name
   ```
2. Open a Pull Request against the `main` branch of `SKNOTFOUND248/Space-Engine`.
3. Fill out the PR template detailing:
   - What the PR accomplishes.
   - Any physical or architectural trade-offs.
   - Test results confirming `18/18 Passed`.
