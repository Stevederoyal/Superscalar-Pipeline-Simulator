# N-Wide Superscalar Pipeline Simulator with Branch Prediction

A cycle-accurate, trace-driven timing simulator for an $N$-wide superscalar processor pipeline implemented in C++. The simulator evaluates execution throughput by modeling structural hazards, Read-After-Write (RAW) data dependencies, data forwarding paths, and dynamic branch prediction architectures.

---

## Architectural Features

* **$N$-Wide Superscalar Execution:** Configurable issue width supporting both scalar ($N=1$) and superscalar ($N \ge 2$) execution across a standard 5-stage pipeline: Instruction Fetch (IF), Instruction Decode (ID), Execute (EX), Memory (MEM), and Writeback (WB).
* **Hazard & Dependency Tracking:** Resolves pipeline stalls due to true data dependencies (RAW), including intra-cycle dependencies between concurrent instructions in the ID stage for superscalar configurations.
* **Data Forwarding Engine:** Implements bypass paths from the EX and MEM stages back to the ID stage. Accurately models load-use delays where data cannot be forwarded until the load completes its MEM stage.
* **Falling-Edge Register File:** Models a split-cycle register file (write on falling edge / first half of cycle, read on second half), removing the need for WB-to-ID data forwarding.
* **Dynamic Branch Prediction:**
  * **Always-Taken Baseline:** Directional branch prediction checked at IF with branch stalls resolved upon completion in the MEM stage.
  * **gshare Predictor:** Combines a 12-bit Global History Register (GHR) XORed with the lower 12 bits of the PC to index a Pattern History Table (PHT) composed of 2-bit saturating counters initialized to weakly taken (`10`)

---

## Directory Structure

```text
├── src/
│   ├── sim.cpp         # Simulator entry point and execution loop
│   ├── pipeline.cpp    # Pipeline stage logic (IF, ID, EX, MEM, WB) and hazard detection
│   ├── pipeline.h      # Latch structures and Pipeline class definitions
│   ├── bpred.cpp       # Branch predictor logic (Always-Taken, gshare)
│   ├── bpred.h         # Branch predictor interface definitions
│   ├── trace.h         # Instruction trace record definitions
│   └── Makefile        # Build configuration
├── traces/             # Benchmark execution traces (.ptr.gz)
└── scripts/
    ├── runall.sh       # Batch benchmark execution script
    └── runtests.sh     # Automated validation script against golden outputs
