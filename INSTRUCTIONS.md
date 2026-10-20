# ORBITAL — User & Developer Instructions

Comprehensive guide for building, running, exploring, and extending **ORBITAL**.

---

## Table of Contents

1. [System Requirements](#1-system-requirements)
2. [Building the Engine](#2-building-the-engine)
   - [Option A: Visual Studio IDE (Recommended)](#option-a-visual-studio-ide-recommended)
   - [Option B: Command Line via MSBuild](#option-b-command-line-via-msbuild)
3. [Running the Application](#3-running-the-application)
   - [Interactive 3D Graphics Mode](#interactive-3d-graphics-mode)
   - [Headless Automated Test Suite](#headless-automated-test-suite)
4. [Controls & Navigation](#4-controls--navigation)
5. [User Interface Overview](#5-user-interface-overview)
6. [Running Guided Experiments & Tours](#6-running-guided-experiments--tours)
7. [Creating Custom Scenarios (JSON)](#7-creating-custom-scenarios-json)
8. [Authoring Add-on Packages](#8-authoring-add-on-packages)
9. [Troubleshooting](#9-troubleshooting)

---

## 1. System Requirements

- **Operating System**: Windows 10 (version 1903+) or Windows 11 (64-bit).
- **Graphics Card**: Dedicated or integrated GPU supporting **OpenGL 4.3 Core Profile** or higher.
- **Visual Studio**: Visual Studio 2022 (v17.x) or Visual Studio 2026 with the **Desktop development with C++** workload installed.
- **C++ Standard**: C++20 (`/std:c++20`).
- **RAM**: Minimum 4 GB (8 GB recommended).

---

## 2. Building the Engine

### Option A: Visual Studio IDE (Recommended)

1. Open `CPPsumulations1.slnx` or `CPPsumulations1.vcxproj` in Visual Studio.
2. Select your build configuration in the top toolbar:
   - Configuration: **Release** (optimized performance) or **Debug** (for debugging).
   - Platform: **x64**.
3. Build the solution by pressing **Ctrl + Shift + B** (or menu *Build* $\to$ *Build Solution*).
   - Visual Studio automatically invokes `vcpkg` to restore `glfw3`, `glad`, `glm`, and `imgui` via `vcpkg.json`.
4. Press **F5** or **Ctrl + F5** to launch!

### Option B: Command Line via MSBuild

Open **Developer PowerShell for VS 2022/2026** or standard PowerShell with MSBuild in PATH:

```powershell
# Navigate to project directory
cd D:\CrazyCPPSumulations\CPPsumulations1

# Build Release x64
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" CPPsumulations1.vcxproj /p:Configuration=Release /p:Platform=x64 -m

# Build Debug x64
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" CPPsumulations1.vcxproj /p:Configuration=Debug /p:Platform=x64 -m
```

---

## 3. Running the Application

### Interactive 3D Graphics Mode

Run the compiled executable from terminal or Explorer:
```powershell
.\x64\Release\CPPsumulations1.exe
```

### Headless Automated Test Suite

To verify all 18 analytical physics formulas, JSON serializers, VFS security, and orbit invariants without launching a window:
```powershell
.\x64\Release\CPPsumulations1.exe --test-only
```
If all tests pass, the process exits with status code `0`.

---

## 4. Controls & Navigation

| Input | Action | Description |
| :--- | :--- | :--- |
| **Left Click + Drag** | Orbit Camera | Rotates the view around the currently selected/focused body |
| **Right Click + Drag**| Orbit Camera | Alternative drag for pitch and yaw |
| **Scroll Wheel Up** | Zoom In | Smooth exponential zoom closer to target |
| **Scroll Wheel Down** | Zoom Out | Exponential zoom outward into astronomical space |
| **Left Click on Object**| Select | Click any celestial body in the left Object Explorer to focus it |

---

## 5. User Interface Overview

### Top Toolbar
- **Play / Pause**: Freezes or unfreezes physical time.
- **Step (1 dt)**: Advances the simulation by exactly one numerical timestep ($\Delta t$).
- **Reset**: Reloads the default Solar System scenario.
- **Time Warp**: Accelerates simulation time from `1x Realtime` up to `2,592,000x` (1 month per real second).
- **Preset Scenarios**: Dropdown with 9 pre-built setups (Solar System, Two-Body Kepler Lab, Three-Body Chaos, Binary Star, Planetary Collision, Supermassive Black Hole, Mass vs Gravity, Relativistic Precession, Empty Universe).
- **Scale Mode**:
  - `Lab Scale`: Visually scales planet radii relative to orbital distances so they can be viewed without extreme zooming.
  - `1:1 Physical`: Exact astronomical proportions.
- **+ Add Body**: Opens the procedural body creation dialog.
- **Guided Experiments**: Toggles the automated guided scientific experiment runner.
- **DSO Catalog**: Toggles the Deep-Space Objects Explorer (galaxies, nebulae, clusters).
- **Add-ons**: Inspects discovered add-on packages.

### Scientific Inspector & Mass Lab (Right Panel)
- **Physics Tab**: Readouts of mass, radius, mean density, surface gravity ($m/s^2$ and $g$), escape velocity ($km/s$), and GM parameter.
- **Mass Laboratory**: Live mass multiplier slider (0.1x to 10.0x) and pinned anchor toggle.
- **Surface & Biomes Tab**: Live selector for 6 procedural biomes (Terrestrial, Desert, Ocean World, Ice World, Volcanic, Gas Giant), procedural seed, sea level, cloud coverage, and atmosphere density.
- **Orbit Tab**: Real-time osculating Keplerian orbital elements (semi-major axis $a$, eccentricity $e$, inclination $i$, periapsis, apoapsis, orbital period $T$, and instantaneous speed).

### Diagnostics & Telemetry (Bottom Left Panel)
- Displays FPS, physics update duration ($ms$), total simulated time, kinetic energy ($K$), potential energy ($U$), and total energy ($E$).
- Real-time energy drift graph tracking numerical accuracy ($\Delta E / E_0$).
- **Export Telemetry to CSV**: Writes complete telemetry history to `orbital_telemetry.csv`.

---

## 6. Running Guided Experiments & Tours

1. Click **"Guided Experiments"** on the top toolbar.
2. Choose one of the pre-built educational experiments:
   - **Newton's Gravity & Mass Lab**: Demonstrates linear acceleration scaling $a = GM/r^2$.
   - **Orbital Velocity Transitions**: Demonstrates circular orbit $\to$ prograde boost $\to$ parabolic escape velocity $v_{esc} = \sqrt{2}v_c$.
   - **Relativistic Spacetime & Schwarzschild Metric**: Explores event horizon $r_s$, photon sphere ($1.5 r_s$), ISCO ($3.0 r_s$), and gravitational time dilation $\sqrt{1 - r_s/r}$.
3. Click **"Start"**.
4. Read the floating explanation cards and click **"Continue / Next Step"** to advance through the steps.
5. Review the generated observation notes in the experiment report window.

---

## 7. Creating Custom Scenarios (JSON)

Scenarios are stored in `data/scenarios/<scenario_name>.json`. Example structure:

```json
{
  "schemaVersion": 1,
  "id": "my_custom_system",
  "name": "My Planetary System",
  "initialPause": false,
  "timeStepSeconds": 60.0,
  "timeScale": 86400.0,
  "integrator": "velocity-verlet",
  "bodies": [
    {
      "name": "Primary Star",
      "type": "Star",
      "physics": {
        "massKg": 1.989e30,
        "meanRadiusM": 6.96e8,
        "mode": "pinned-source"
      },
      "initialState": {
        "positionM": [0.0, 0.0, 0.0],
        "velocityMS": [0.0, 0.0, 0.0]
      },
      "appearance": {
        "colorRGB": [1.0, 0.9, 0.6],
        "emission": 1.0
      }
    },
    {
      "name": "Habitable Planet",
      "type": "Planet",
      "physics": {
        "massKg": 5.972e24,
        "meanRadiusM": 6.371e6,
        "mode": "dynamic"
      },
      "initialState": {
        "positionM": [1.496e11, 0.0, 0.0],
        "velocityMS": [0.0, 0.0, 29780.0]
      },
      "appearance": {
        "colorRGB": [0.2, 0.5, 0.9],
        "atmosphereEnabled": true,
        "atmosphereDensity": 1.0
      }
    }
  ]
}
```

---

## 8. Authoring Add-on Packages

To distribute custom packs of scenarios, catalogs, or textures:
1. Create a subfolder inside `addons/`, e.g. `addons/my_pack/`.
2. Place a `manifest.json` file inside it:
   ```json
   {
     "schemaVersion": 1,
     "id": "com.author.my-pack",
     "displayName": "My Custom Addon Pack",
     "version": "1.0.0",
     "minimumEngineVersion": "0.1.0",
     "description": "Custom planetary systems and texture biomes.",
     "license": "CC-BY-4.0",
     "contentRoots": ["catalogs", "scenarios", "textures"]
   }
   ```
3. Place files inside `addons/my_pack/scenarios/` or `addons/my_pack/catalogs/`.
4. When ORBITAL launches, the VFS will automatically discover the package and mount its assets.

---

## 9. Troubleshooting

- **Error: `LNK1104: cannot open file ... CPPsumulations1.exe`**:
  An instance of the simulation is currently running in the background. Open Task Manager or run `taskkill /F /IM CPPsumulations1.exe` in PowerShell, then recompile.
- **Low Framerate**:
  Ensure your laptop/desktop is utilizing the dedicated GPU (NVIDIA / AMD) rather than low-power integrated graphics. Check the console output on startup (`[RENDERER]`) to verify which GPU OpenGL initialized on.
- **vcpkg Dependency Errors**:
  Make sure Visual Studio has the **vcpkg package manager** component enabled in the Visual Studio Installer.
