# Security Policy — ORBITAL

The ORBITAL engineering team takes software security, memory safety, and simulation sandbox integrity seriously. This document outlines our security policies, vulnerability reporting procedures, and architectural security boundaries.

---

## Supported Versions

Only the latest release branch and `main` receive active security patches.

| Version | Supported | Status |
| :--- | :--- | :--- |
| **0.2.x (Current)** | :white_check_mark: | Active support |
| **0.1.x** | :x: | End of life |
| **< 0.1.0** | :x: | Development snapshots |

---

## Reporting a Vulnerability

If you discover a security vulnerability or exploit in ORBITAL, **please do NOT create a public issue on GitHub.**

Instead, please report it privately:
1. **GitHub Security Advisory**: Navigate to the [Security tab](https://github.com/SKNOTFOUND248/Space-Engine/security/advisories) on GitHub and click **"New draft advisory"**.
2. **Direct Contact**: Email the repository maintainers or security contact with:
   - Type of issue (e.g., VFS directory traversal, memory corruption, shader injection, buffer overflow).
   - Detailed step-by-step instructions or minimal reproduction scenario (`.json` file).
   - Potential impact and affected platforms (Windows x64).

### Our Commitment
- **Initial Response**: We will acknowledge receipt of your vulnerability report within **48 hours**.
- **Assessment**: We will confirm the vulnerability and provide an estimated timeline for remediation within **7 days**.
- **Coordination**: We ask that you observe responsible disclosure practices and allow sufficient time for patch release prior to public disclosure.

---

## Architectural Security Measures

ORBITAL is designed with defensive boundaries to ensure that loading custom scenarios, catalogs, and add-on packs does not compromise the host system:

### 1. Virtual File System (VFS) Sandboxing
- All asset requests (`catalogs://`, `scenarios://`, `textures://`) must resolve within registered content directories.
- Path traversal sequences (`..`, `../`, `..\`) are strictly filtered and rejected at the VFS resolution layer before touching OS system calls.
- Absolute paths pointing outside mounted roots are prohibited.

### 2. Constrained Script Execution (No Eval / No Native Exec)
- The ORBITAL Scenario Script engine (`ScenarioScript` & `ScriptRunner`) is a declarative state-machine dispatcher.
- It **never** invokes system shell commands, native OS processes (`CreateProcess`, `system`), or dynamic library loading (`LoadLibrary`).
- Scenario scripts can only alter registered simulation parameters (`body.setMass`, `camera.setPose`, `simulation.pause`).

### 3. Strict Schema Validation
- All external data (JSON scenarios, catalog files, manifest metadata) is strictly validated against versioned schemas.
- Non-finite numbers (`NaN`, `Inf`), invalid types, and malformed structures are caught during deserialization with non-destructive fallback.

### 4. Supply Chain & Dependency Hygiene
- Dependencies (`glfw3`, `glad`, `glm`, `imgui`) are pinned and managed deterministically via `vcpkg` manifest mode (`vcpkg.json`).
- Pre-built binary blobs from unverified sources are rejected.
