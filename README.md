# Branch Description

This branch showcases the progress made on the OS extension and the overall project architecture.

It includes:

- A minimal shell with background process support
- Dynamic memory allocation
- A modular kernel structure
- Synchronization primitives (mutexes and semaphores)

This branch is intended to demonstrate the architectural evolution of the project.

---

# CI & Evaluation Notice

This version is provided as a demonstration branch.

The original CI environment relied on:

- French command aliases
- A specific Makefile structure

Due to the new modular architecture and the migration to CMake, the legacy CI setup is no longer compatible and may fail.

This branch prioritizes clean architecture, modularity, and extensibility over CI compatibility.

---

# Building the Project

The project now uses **CMake** with a dedicated toolchain file (designed for future expansion and portability).

## Quick Demo (Kernel Build)

```bash
cd src/
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=riscv64-toolchain.cmake
cd build/
make rung
```

## Unit Testing

```bash
cd tests/
cmake -B build -S .
cd build/
make tests
```
