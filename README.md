# LC3 Simulator

A cycle-accurate LC-3 simulator with a C++ core and an interactive web frontend. The simulator models the LC-3 ISA at the level of the textbook finite state machine — every instruction is decomposed into its constituent FSM states (FETCH_0, FETCH_1, FETCH_2, DECODE, and per-opcode execute states), and each `step()` advances exactly one state transition. The goal is to make the LC-3 datapath visible: you can watch fetch happen, watch decode dispatch, and see exactly where each instruction's microarchitecture spends its cycles.

## Why I Built This

The inspiration comes from ECE120 and ECE220 at UIUC. Where the LC3 ISA is treated as the foundational assembly language taught to students. The textbook (Patt & Patel) shows the LC-3 as a state machine, but most simulators I'd used (including the course's reference) treat instructions as atomic. You hit "step" and the whole instruction executes in one shot. That hides what I find most interesting about the architecture: the cycle-by-cycle dance between PC, MAR, MDR, IR, and the register file as an instruction works its way through the datapath.

So I wrote one that doesn't hide that. The FSM is the simulator. Stepping is per-cycle. Eventually the web frontend will visualize the FSM diagram with the current state highlighted, so you can see — in real time — your program move through the state graph as it executes.

## Structure

```
lc3-simulator/
├── examples/    # Sample LC-3 assembly programs
│   ├── branch_test.asm
│   ├── fibonacci.asm
│   └── loop.asm
├── src/        # CPU, Memory, Simulator Implementation (C++ Core)
│   ├── bindings.cpp
│   ├── cpu.cpp
│   ├── cpu.h
│   ├── cpu_state.h
│   ├── memory.cpp
│   ├── memory.h
│   └── test.cpp
├── www/       # WebAssembly frontend and browser UI
│   ├── index.html
│   ├── lc3.js
│   ├── lc3.wasm
│   ├── script.js
│   └── style.css
├── .gitignore
├── Makefile  # Configuration for Building
└── README.md
```

## Architecture

The simulator is built with three layers:

src/cpu_state.h — The data layer. Defines the struct CPUState that holds all architectural states (R0–R7, PC, IR, MAR, MDR, NZP flags) along with the current FSM state and a cycle counter. This header also defines the FSMState enum (~40 states). Converting LC3 FSM states to names that are more understandable by learners, and RTL (Register Transfer Language) descriptions (e.g., LD_0 → "MAR <- PC+offset9").

src/memory.{h,cpp} — The Memory class. The class holds a 64K array of 16-bit words with read(), write(), clear_memory(), and load_program() methods. 

## Features

- **Block-based program editor.** Drag-and-drop opcodes with fillable register and immediate fields. No assembly syntax to memorize — invalid combinations are unreachable by construction.
- **Per-cycle stepping.** Step one FSM transition at a time, one full instruction at a time, or run to halt with adjustable speed.
- **Live FSM trace.** Every cycle logged with state name, Patt & Patel state number, and RTL description, color-coded by phase (fetch, decode, execute, memory access, halt).
- **Animated datapath.** PC, IR, MAR, MDR, memory, and ALU light up to show which datapath element is active in the current state.
- **Full instruction set.** ADD, AND, NOT, LD, LDR, LDI, ST, STR, STI, LEA, BR (with N/Z/P conditions), JMP, JSR, JSRR, TRAP, HALT.
- **Branch/loop support.** Attach labels to blocks; BR and JSR targets resolve them visually.


## How it works

The inspiration of this simulator comes from a programming tool that probably everyone has used as a kid — Scratch. The visual simulator offers fillable operation blocks based off the LC3 ISA on the right side of the page. Based on what the user wants, the user can build the program using the fillable blocks on the right. Then, instead of directly showing compilation results as a compiler, the user can toggle the step and run commands on the bottom of the page. In which the step shows the FSM instructions once at a time, or fully printed out as a whole. Simultaneously, the user can choose a specific step and see the values of different LC3 CPU signals, such as MAR, MDR, IR, NZP, etc. In this way, the tool can promote better understanding of assembly language for students that are new to these concepts. 

  
## Build and Run Locally

The native simulator builds with any C++17 compiler:

```bash
make native        # produces ./lc3_simulator
make test          # runs the unit tests
```

The web simulator requires Emscripten. One-time setup:

```bash
git clone https://github.com/emscripten-core/emsdk.git
./emsdk/emsdk install latest
./emsdk/emsdk activate latest
source ./emsdk/emsdk_env.sh
```

Then build the WASM and preview the site:

```bash
make wasm          # compiles src/ → www/lc3.js + www/lc3.wasm
make serve         # serves http://localhost:8000/www/
```

Open `http://localhost:8000/www/` in a browser. The site needs HTTP (not `file://`) because the WASM loader uses ES-module imports.


## Acknowledgements

The LC-3 ISA and FSM design are from Introduction to Computing Systems by Yale Patt and Sanjay Patel. State numbers in the trace match the canonical microcontroller in Appendix C of that text.
