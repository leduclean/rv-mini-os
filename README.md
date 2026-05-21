# RISC-V OS Kernel

A bare-metal operating system kernel targeting **RISC-V 64-bit** (QEMU `virt` machine), written in C and RISC-V assembly.

> **Academic project** — developed at [Ensimag](https://ensimag.grenoble-inp.fr/) (Grenoble INP) under the supervision of **Sébastien Viardot**.

---

## Features

### Hardware & Boot
- Bare-metal RISC-V 64-bit target (no host OS, no standard library)
- UART console driver
- Graphical framebuffer via bochs-display
- PLIC-based external interrupt routing
- Timer interrupt support

### Process Management
- Process lifecycle: `READY → RUNNING → SLEEPING / BLOCKED → TERMINATED → ZOMBIE`
- Priority levels: `HIGH`, `NORMAL`, `LOW`, `IDLE`
- Priority-based round-robin scheduler with per-priority ready queues
- Foreground and background process spawning
- Sleep and wake-up with timer-based deadlines
- Zombie reaping and parent/child process relationships

### Synchronization
- Mutexes
- Semaphores
- Wait queues with optional timeout
- IRQ-protected critical sections

### Memory
- Custom dynamic allocator: first-fit free list with eager splitting and immediate coalescing (`tinyalloc`)
- Statically allocated kernel heap

### Shell
- Interactive shell process with command registry
- Command parser
- Built-in programs (`ps`, ...)

### Libraries
- Minimal libc subset (no host dependency): `printf`, `sprintf`, `string`, `ctype`, `strtoul`
- Circular doubly-linked list (`clist`) with `container_of` intrusive node design
- Font rendering

---

## Project Structure

```
src/
├── arch/riscv/        # RISC-V specific: boot, trap handler, UART, IRQ, linker scripts
├── kernel/
│   ├── core/          # Scheduler, process, shell, sync primitives, time
│   └── init/          # Kernel entry point
├── lib/               # Minimal standard library, clist, allocator
└── include/           # Public headers
tests/                 # Unit tests (Unity framework)
```

---

## Requirements

- `riscv64-unknown-elf-gcc` toolchain
- `qemu-system-riscv64`
- `cmake >= 3.15`

---

## Build & Run

### Kernel (text mode)

```bash
cd src/
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=riscv64-toolchain.cmake
cd build/
make run
```

### Kernel (graphical mode)

```bash
make rung
```

### Debug (GDB over TCP)

```bash
make debug   # text mode + GDB server on :1234
make debugg  # graphical mode + GDB server on :1234
```

Then attach with:

```bash
bash debug.sh   # wrapper around gdb with .gdbinit preset
```

### Unit Tests

```bash
cd tests/
cmake -B build -S .
cd build/
make tests
```

---

## Academic Context

This project was developed as part of the **Operating Systems** course at [Ensimag](https://ensimag.grenoble-inp.fr/) (Grenoble INP — école nationale supérieure d'informatique et de mathématiques appliquées de Grenoble).

**Supervisor:** Sébastien Viardot

The project started from a minimal RISC-V bootstrap and was progressively extended with a scheduler, synchronization primitives, a dynamic allocator, and an interactive shell.
