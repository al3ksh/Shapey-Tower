# Shapey Tower (raylib)
A tiny vertical arcade platformer built with C++17 and raylib. You jump up through procedurally generated platforms while the camera scrolls upward. This repository exists primarily for learning: game loops, input, simple physics, and basic rendering.

## Features
- Procedural platforms (random width, X, and vertical spacing ~75–120).
- Some platforms move horizontally with a sine pattern.
- Simple 2D physics: gravity, horizontal speed clamp, friction, coyote time, jump buffer.
- One‑way collisions (land from above using a crossing test).
- Up‑only camera with deadzone (Camera2D) — no downward follow.
- Score + combo system (time window + bonus for skipping floors).
- Highscore persistence in `highscore.txt` next to the executable.

## Controls
- Move: A / D
- Jump: Space / W / Up

## Project structure
```
assets/                # audio, shaders, textures etc.
include/               # public headers
src/                   # game sources (entry point in src/main.cpp)
build/                 # CMake build (Release)
build-debug/           # CMake build (Debug) — created on first debug configure
settings.cfg           # runtime settings (if used by the game)
highscore.txt          # saved highscore (created at runtime)
```

## Prerequisites
- CMake 3.20+
- A C++17 compiler (MSYS2/MinGW on Windows, clang/gcc on Linux/macOS)
- raylib (runtime and development files)

Windows (MSYS2/UCRT64) recommended setup:
- Install MSYS2 and open the UCRT64 shell.
- Install toolchain and raylib: `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-raylib`
- Make sure `${MSYS2}/ucrt64/bin` is on PATH while running the game (for `raylib.dll`).

## Quick start in VS Code
This repo includes ready‑to‑use tasks and debug configs.

Build & run (Release):
1) Run Task → “cmake: configure (Release)” (first time only)
2) Run Task → “cmake: build (Release)”
3) Run Task → “Run shapeytower.exe”

Debugging:
- Use the launch config: “Debug ShapeyTower (gdb, Release)” or “Debug ShapeyTower (gdb, Debug)”.
- The Debug profile uses the separate `build-debug/` directory and includes debug symbols.

Assets are copied automatically next to the built executable by a CMake post‑build step.

Removed experimental shadow rendering system (was a multi‑layer projection) to keep code lean; current visuals rely solely on simple rect rendering.

## Build with CMake (CLI)
Release:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Debug:
```
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --config Debug
```

If CMake finds `raylib` via `find_package(raylib)`, it will set includes and link flags. On Windows, the fallback links against `raylib opengl32 winmm gdi32`. Ensure `raylib` is installed and visible to the linker and that `raylib.dll` is available at runtime (next to the `.exe` or in PATH).

## Manual one‑liner (Windows/MinGW)
Equivalent to the classic “just compile it” approach:
```
g++ -O2 -std=c++17 src/*.cpp -Iinclude -o shapeytower.exe -lraylib -lopengl32 -lwinmm -lgdi32
```
Make sure the compiler can find raylib’s headers and libraries, and that `raylib.dll` is discoverable at runtime.

## Acknowledgements
- Built with [raylib](https://www.raylib.com/) (zlib/libpng license).