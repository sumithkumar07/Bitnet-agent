# Single-Bit Neural Simulation Core

An experimental C++ proof-of-concept testing AVX2 constraints and localized array parsing bounds without utilizing external float libraries.

## Status: Experimental / Alpha
This repository is currently a **simplified simulation sandbox**. It explores mathematical optimizations in `[-1, 0, 1]` matrix operations but does not yet represent a fully functioning production transformer architecture. Future developments aim to gradually implement Attention and complete Neural Nodes phase-by-phase.

## Architecture
This engine is built entirely on native C/C++ arrays utilizing direct AVX2 processing. It strictly enforces memory isolation between distinct struct components.

### Mechanics:
- **AVX2 Computation Bounds:** Core integer arrays compute utilizing parallel SIMD instructions mapped through an intrinsic L1 lookup mask. 
- **Array Overflow Sub-Division:** Large string tokenizations that overflow standard array bounds sequentially spawn dynamic clones to distribute the load locally.
- **Local Isolation:** No Network sockets, no local Database overheads, no Float multiplications. Execution is restricted strictly inside local terminal physics.

## Development Verification
A strict 20-Phase Verification Protocol (`docs/RULES.md`) structurally logs development.

## Getting Started
Ensure you have the Microsoft Visual C++ Compiler (`cl.exe`) or `vcvars64.bat` accessible through a typical VS Community installation.

```cmd
# Run the provided Build Script (Requires MSVC)
build.bat
```

Executing `build.bat` automatically resolves MSVC environments dynamically, compiles `src/main.cpp` using Hardware SIMD (`/arch:AVX2`), and executes an immediate loop to successfully verify zero-crashing array boundaries end-to-end.
