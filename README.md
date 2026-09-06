# Superscalar Pipeline Simulator

A trace-driven timing simulator (C++) modeling an N-wide superscalar RISC pipeline, extended with data dependency tracking, operand forwarding, and branch prediction. Built for ECE 4100/6100 (Advanced Computer Architecture) at Georgia Tech.

## Overview

The simulator models a 5-stage pipeline — Instruction Fetch (IF), Instruction Decode (ID), Execute (EX), Memory (MEM), and Writeback (WB) — driven by pre-recorded instruction traces (no functional data values, timing only). Starting from a provided N-wide superscalar skeleton with no dependency tracking, the project implements:

1. **Data dependency tracking and stalling** for a scalar (N=1) machine
2. **Generalized dependency tracking** for an N-wide superscalar machine (tested at N=2), accounting for dependencies from instructions still in the ID stage
3. **Data forwarding** from the EX and MEM stages, including correct handling of load-to-use dependencies (a load's value isn't available until MEM)
4. **Branch prediction**, first with an idealized "AlwaysTaken" predictor, then a **gshare** predictor (12-bit global history register XORed with the PC, indexing a pattern history table of 2-bit saturating counters)

## Architecture

```
sim.cpp          — opens trace files, initializes and runs the pipeline
trace.h          — trace record format (op fields, *_needed flags, cc_read/cc_write)
pipeline.cpp/.h  — Pipeline class; pipe_cycle_IF() ... pipe_cycle_WB() implement
                   per-stage behavior, dependency tracking, stalling, and forwarding
bpred.cpp/.h     — branch predictor interface (get prediction / update predictor)
```

### Pipeline stages modeled

| Stage | Responsibility |
|---|---|
| **IF** | Fetches the next instruction (up to N-wide); consults the branch predictor for conditional branches |
| **ID** | Decodes the instruction; checks for data dependencies against older in-flight instructions (ID, EX, MEM) |
| **EX** | Executes the operation; forwarding sources for downstream dependent instructions |
| **MEM** | Completes memory operations (loads/stores); final point where load results become available |
| **WB** | Commits results — writes on the falling edge, so no forwarding is needed from WB back to ID |

## Part A — Dependency Tracking & Forwarding

- **A.1 — Scalar dependency tracking (N=1):** Detects RAW hazards between an instruction in ID and older instructions still in flight, stalling ID until the dependency clears.
- **A.2 — N-wide superscalar generalization:** Extends dependency checks so an instruction can depend not only on instructions in EX/MEM, but also on **older instructions still in the same ID bundle** — since multiple instructions can be in ID simultaneously in a superscalar machine.
- **A.3 — Data forwarding (EX and MEM):** Adds forwarding paths so dependent instructions don't always need to stall. Load instructions are a special case — their value isn't available until MEM, so a load-dependent instruction can only forward from MEM, never from EX.

## Part B — Branch Prediction

- **B.1 — AlwaysTaken predictor:** A baseline predictor assuming every conditional branch is taken; integrated with the A.3 pipeline (N=2) via an idealized Branch Target Buffer (BTB) that identifies branches and provides the target address at fetch time. On a misprediction, fetch stalls until the branch resolves in MEM.
- **B.2 — gshare predictor:** A 2-level adaptive predictor. The Global History Register (12 bits) is XORed with the low-order bits of the instruction address to index a Pattern History Table of 2-bit saturating counters (initialized to weakly-taken, `10`), giving direction predictions that adapt to per-branch and correlated global history patterns.

## Results

Pipeline performance (in cycles-to-completion) was evaluated across four instruction traces for each configuration:

| Configuration | Description |
|---|---|
| N=1, no forwarding | Scalar, stall-only baseline |
| N=1, forwarding | Scalar with EX/MEM forwarding |
| N=2, forwarding | Superscalar, perfect branch prediction |
| N=2, AlwaysTaken | Superscalar with baseline branch predictor |
| N=2, gshare | Superscalar with adaptive 2-level branch predictor |

*(Add your actual cycle-count / IPC results and any performance comparison charts here once testing is complete.)*

## Tools

- **Language:** C++
- **Build:** `make` (see `Lab_2/src`)
- **Reference environment:** `ece-linlabsrv01.ece.gatech.edu`

## Running the Simulator

```bash
cd Lab_2/src
make
./sim ../traces/mcf.ptr.gz                      # scalar (N=1)
./sim -pipewidth 2 ../traces/mcf.ptr.gz         # 2-wide superscalar
./sim -h                                        # full list of command-line options
```

The `scripts/runall.sh` script runs all configurations (A1, A2, A3, B1, B2) across all provided traces in one pass. `scripts/runtests.sh` compares output against the provided reference results (`ref/refoutput_*.pdf`).

## Repository Structure

```
├── src/
│   ├── sim.cpp
│   ├── trace.h
│   ├── pipeline.cpp
│   ├── pipeline.h
│   ├── bpred.cpp
│   └── bpred.h
├── scripts/
│   ├── runall.sh
│   └── runtests.sh
├── traces/
├── ref/
├── docs/
│   └── gshare_predictor.png
└── README.md
```

## Author

**Stephen Tagoe**
[GitHub](https://github.com/Stevederoyal) | [LinkedIn](https://linkedin.com/in/stephen-tagoe-7588a61b7/)
