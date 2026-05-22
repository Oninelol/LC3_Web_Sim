# LC3 Simulator

A cycle-accurate LC-3 simulator with a C++ core and an interactive web frontend. The simulator models the LC-3 ISA at the level of the textbook finite state machine — every instruction is decomposed into its constituent FSM states (FETCH_0, FETCH_1, FETCH_2, DECODE, and per-opcode execute states), and each `step()` advances exactly one state transition. The goal is to make the LC-3 datapath visible: you can watch fetch happen, watch decode dispatch, and see exactly where each instruction's microarchitecture spends its cycles.

## Why I Built This

The inspiration comes from ECE120 and ECE220 at UIUC. Where the LC3 ISA is treated as the foundational assembly language taught to students. The textbook (Patt & Patel) shows the LC-3 as a state machine, but most simulators I'd used (including the course's reference) treat instructions as atomic. You hit "step" and the whole instruction executes in one shot. That hides what I find most interesting about the architecture: the cycle-by-cycle dance between PC, MAR, MDR, IR, and the register file as an instruction works its way through the datapath.

So I wrote one that doesn't hide that. The FSM is the simulator. Stepping is per-cycle. Eventually the web frontend will visualize the FSM diagram with the current state highlighted, so you can see — in real time — your program move through the state graph as it executes.

## Architecture

The simulator is built with three layers:

