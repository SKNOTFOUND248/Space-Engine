---
name: visual-studio-cpp-project
description: Understands, edits, builds, and troubleshoots native C++ projects created with Microsoft Visual Studio on Windows, including .sln/.vcxproj structure, MSBuild configuration, source/header organization, linker errors, and OpenGL projects using GLFW and GLAD. Use when working on a Visual Studio C++ project from Antigravity.
---

# Visual Studio C++ Project Structure Skill

## Mission

Work safely and accurately inside existing Microsoft Visual Studio C++ projects. Treat the `.sln` and `.vcxproj` files as project configuration, not disposable metadata. Preserve the user's current toolchain and project layout unless a change is necessary and explained.

## First: inspect before changing

1. Identify the workspace root and list the project files.
2. Look for:
   - `*.sln` — solution entry point; may contain multiple projects.
   - `*.vcxproj` — MSBuild project configuration.
   - `*.vcxproj.filters` — Visual Studio Solution Explorer display grouping; not the actual filesystem.
   - `*.vcxproj.user` — usually user-specific local debugging settings; avoid changing unless needed.
   - `src/`, `include/`, `headers/`, `assets/`, `external/`, `vendor/`, `lib/`, `bin/`, `build/`, `x64/`, `Debug/`, `Release/`.
   - `CMakeLists.txt` — may indicate a CMake-based Visual Studio project rather than a classic `.vcxproj` project.
3. Read the existing README, build instructions, project properties, and source files before proposing a layout.
4. Determine the actual build system: classic Visual Studio/MSBuild (`.vcxproj`), CMake, or another system. Do not assume all Visual Studio projects are classic MSBuild projects.
5. Check the selected platform (`x64` or `Win32`), configuration (`Debug` or `Release`), C++ language standard, Windows SDK, and compiler toolset when relevant.

## Understand the project files

- `.sln`: groups projects and solution configurations.
- `.vcxproj`: defines source files, include paths, macros, compiler options, linker inputs, and per-configuration settings.
- `.vcxproj.filters`: organizes files in Solution Explorer. It does not move files on disk.
- `.vcxproj.user`: local user settings; normally do not commit or overwrite it.
- `.cpp`: C++ implementation files.
- `.h` / `.hpp`: declarations and class/interface headers.
- `.rc`: Windows resources, if the project uses them.
- `.props` / `.targets`: reusable MSBuild property sheets and build customizations.
- `packages.config`, `vcpkg.json`, or package-manager files: dependency metadata, if present.
- `CMakeLists.txt`: CMake project definition.
- `Debug/`, `Release/`, `x64/`, `build/`, `.vs/`, `ipch/`, and generated intermediates are generally build output or IDE state, not the main source of truth.

## Safe editing rules

- Make the smallest change that solves the request.
- Do not replace the project with a generic template or recreate `.sln`/`.vcxproj` files without a clear reason.
- Do not delete, rename, or move project files without checking all references in the `.vcxproj`, `.vcxproj.filters`, source `#include`s, resource files, and build scripts.
- When adding a `.cpp` or header to a classic MSBuild project, ensure it is included in the project file if the project does not automatically include it. Update `.vcxproj.filters` only if Solution Explorer grouping should be maintained.
- Keep declarations in headers and implementations in `.cpp` files where appropriate. Use include guards or `#pragma once`; avoid defining non-inline functions or global variables in headers in ways that cause multiple-definition linker errors.
- Use relative paths where practical. Avoid hard-coded paths tied to one user's machine.
- Do not commit generated binaries, intermediate files, `.vs/`, or machine-specific settings unless the repository explicitly requires them.
- Never silently change the compiler, Windows SDK, target architecture, C++ standard, or dependency versions.
- Before editing build settings, explain which setting is changing and why if the change could affect other configurations.

## Build and diagnose

1. Find the available build tools. Prefer the Visual Studio Developer PowerShell/Command Prompt or MSBuild installed with Visual Studio.
2. Build the existing solution/project using its current configuration and platform. Example:
   `msbuild MySolution.sln /m /p:Configuration=Debug /p:Platform=x64`
   Replace the filename and platform with values that actually exist in the solution.
3. Do not claim a build passed unless a build command was actually run and its exit status/output was checked.
4. Diagnose the first meaningful compiler or linker error first; later errors may be cascading failures.
5. Classify errors before editing:
   - `Cxxxx`: compiler/source or compiler-setting error.
   - `LNKxxxx`: linker/input/library/configuration error.
   - `E1696` or similar IntelliSense-only diagnostics: verify with the actual compiler/build output.
   - Missing header: check include path, dependency presence, spelling, and platform-specific paths.
   - Unresolved external symbol: check implementation is compiled, correct library is linked, architecture matches, and calling conventions/configuration agree.
6. Check both Debug and Release only when the request requires it; do not make broad changes just to silence IntelliSense.
7. Summarize the changed files, the reason for each change, build/test commands run, and any remaining blockers.

## OpenGL project guidance

Use this section when the project uses OpenGL, GLFW, GLAD, GLEW, GLUT/freeglut, or related graphics libraries.

- A Visual Studio **Console App** can create an OpenGL window. A Windows Desktop Application is not required just because the program opens a graphics window.
- Identify which API and loader the project already uses. Do not mix GLAD, GLEW, and other loaders without an explicit reason.
- For a modern OpenGL starter, GLFW handles window/context/input creation and GLAD loads OpenGL function pointers. The OpenGL API library is commonly linked as `opengl32.lib` on Windows; GLFW and any loader must also be configured according to how they were installed.
- Check the architecture of every dependency: an x64 project needs x64 libraries; Win32 needs Win32 libraries.
- Check include directories, library directories, linker input names, runtime DLL availability, and any package-manager setup (such as vcpkg) before changing them.
- Do not copy random `.lib` or `.dll` files into the project or download dependencies from unverified sources. Prefer the project's existing package manager or official project distribution.
- Keep third-party code under the project's established `external/` or `vendor/` convention if source vendoring is already used. Do not move dependency files solely to make the tree look cleaner.
- Keep shaders, textures, and other runtime assets in a clear asset directory when appropriate. Ensure the working directory or asset path configuration lets the executable find them.
- Explain the difference between **Include Directories** (headers), **Library Directories** (where the linker searches), **Additional Dependencies** (library filenames), and **PATH/runtime DLL lookup**.
- A missing `glad/glad.h` is generally an include/dependency configuration issue; an unresolved `gladLoadGLLoader` or similar symbol is generally a source/linkage mismatch; `LNK1104` often means a library path/name is wrong or the file is unavailable. Verify against the actual error and installed dependency version before concluding.
- Do not invent GLAD API names. GLAD 1 and GLAD 2 generated projects may use different include paths and loader function names. Inspect the generated files.

## Recommended structure (only when starting fresh or refactoring is requested)

Do not force this structure on an existing project. Adapt to its current conventions.

```text
MyOpenGLProject/
├── MyOpenGLProject.sln
├── MyOpenGLProject/
│   ├── MyOpenGLProject.vcxproj
│   ├── MyOpenGLProject.vcxproj.filters
│   ├── src/
│   │   └── main.cpp
│   ├── include/
│   │   └── ...
│   ├── shaders/
│   │   ├── basic.vert
│   │   └── basic.frag
│   ├── assets/
│   └── external/       # only if third-party sources are vendored
├── .gitignore
└── README.md
```

Build output should generally stay in the configured output/intermediate directories rather than being mixed with source files.

## How to handle user requests

- For "explain the structure": map the actual files and explain what each does; distinguish filesystem folders from Solution Explorer filters.
- For "add a file/class": create the files in the established source layout, update project metadata if needed, add includes, and build.
- For "fix build errors": reproduce or inspect the exact error, identify its category, make a minimal fix, and rebuild.
- For "set up OpenGL": first inspect existing dependencies and platform/configuration; then give exact Visual Studio property changes or package-manager steps appropriate to the detected setup.
- For "clean up the project": propose a plan before moving or deleting files, preserve project references, and avoid generated directories.
- For "convert to CMake": treat this as a migration, not a routine cleanup. Inventory dependencies and configurations first and explain trade-offs before replacing build metadata.

## Final checklist

Before finishing, verify:
- [ ] The original build system and toolchain were preserved unless a change was requested.
- [ ] Every new source/header file is in the expected project and filesystem location.
- [ ] Project metadata and filters are consistent where applicable.
- [ ] Include and linker settings match the installed dependency and target architecture.
- [ ] No unrelated files or settings were changed.
- [ ] A real build/test was run if tools were available; otherwise state that it was not run.
- [ ] The response names changed files and any follow-up steps clearly.
