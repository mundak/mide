# mIDE

mIDE is a C-focused IDE and toolchain for x64 Windows.

The core idea is straightforward: as C code is written, the IDE should continuously analyze and interpret it, keeping a live view of the generated assembly in sync with the source. Instead of treating compilation, debugging, and machine-code inspection as separate phases, mIDE is intended to keep them connected at all times.

When a program is run, stepping and debugging should be simpler than in a traditional environment because the IDE will own the full compilation pipeline and all of the metadata that describes the program. That control is the main reason this project does not rely on external toolchains.

## Goals

- Provide real-time visibility from C source code to x64 assembly output.
- Make execution, stepping, and debugging tightly integrated with the editor.
- Own the full toolchain so the IDE can expose information that third-party compilers and debuggers typically hide.
- Focus on x64 Windows first so the runtime and development environment stay controlled and practical.

## Why a custom toolchain

mIDE will include its own compiler, assembler, linker, debug symbol format, and C standard library.

This is a deliberate architectural choice. Existing toolchains are optimized for general-purpose builds, portability, and compatibility. mIDE needs different tradeoffs: incremental analysis, real-time assembly feedback, full control over emitted metadata, and debugger behavior designed specifically for this IDE.

By owning the entire pipeline, the project can:

- attach rich source-to-machine-code relationships to every stage of compilation,
- design a debug symbol format around IDE needs rather than external standards,
- simplify stepping and inspection because the runtime model is known end to end,
- evolve the compiler and debugger together instead of working around opaque toolchain behavior.

## Core components

- A native IDE written in C++ for performance and low-level control.
- An immediate-mode user interface built with ImGui.
- A custom C compiler designed for interactive, editor-driven analysis.
- A custom assembler and linker for the x64 Windows target.
- A custom debug symbol format tailored to live inspection and precise stepping.
- A custom C standard library suitable for the project runtime and tooling goals.

## Current prototype

The repository now includes a first ImGui-based shell for the editor UI. It is a standalone desktop app using GLFW, the OpenGL backend, and Dear ImGui from the docking branch.

The current window is intentionally a fake IDE surface: explorer, editor, preview, assembly, watch, console, memory, and profiler panels are placeholder content meant to establish the visual direction and docking behavior.

Docking and multi-viewports are enabled, so any docked tab can be dragged out into a native OS window.

## Build

```powershell
cmake -S . -B build
cmake --build build
```

The executable will be generated in the build directory. The first configure step downloads GLFW and Dear ImGui via CMake FetchContent.
